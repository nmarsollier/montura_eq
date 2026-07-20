/* Motors - motors_motion_task.c
 *
 * Purpose: FreeRTOS motion task — consumes MotionCommands from the queue
 * and drives axis positions via RMT+DMA step pulse generation.
 *
 * The motion task is the SINGLE WRITER of motors_state position,
 * status, and tracking fields — all other code only reads them.
 *
 * Two execution paths, dispatched by command type:
 *   slewing_loop_rmt  — distance-bounded, ramped accel/decel, batched RMT
 *   tracking_loop_rmt — open-ended, constant velocity, fractional accumulator
 *
 * RMT+DMA replaces the previous software GPIO bit-banging. Step pulses
 * are hardware-timed with zero jitter. The CPU sleeps on a semaphore
 * while DMA streams symbols to the RMT peripheral.
 */

#include "motors_internal.h"

#include <math.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "MOTORS_MOTION_TASK";

/*
 * Task stack size — sized to accommodate batch encoding loops,
 * condition checks, and FreeRTOS queue operations.
 */
#define MOTION_TASK_STACK_WORDS 4096
#define MOTION_TASK_PRIORITY    5

/* Step period ceiling — used when velocity is zero or unknown. */
#define MAX_STEP_PERIOD_US (10 * 1000 * 1000)

/*
 * Above this step period vTaskDelay yields the CPU; at or below it
 * a busy-wait with esp_timer keeps microsecond precision (used in
 * the tracking loop fine-wait only).
 */
#define BUSYWAIT_THRESHOLD_US 20000

/* --------------------------------------------------------------------------
 * RMT batch control
 * -------------------------------------------------------------------------- */

/*
 * Target batch duration.  Shorter batches give finer ramp granularity
 * but increase CPU overhead.  20 ms balances both at max slew speed.
 */
#define RMT_BATCH_TARGET_US  20000U

/* Safety cap — never queue more than this many steps in one batch. */
#define RMT_BATCH_MAX_STEPS  48U

/*
 * RMT symbol buffer capacity.  Must not exceed the non-DMA channel's
 * internal memory (SOC_RMT_MEM_WORDS_PER_CHANNEL = 48 on ESP32-S3).
 * The DMA channel (RA) uses this same size for simplicity.
 */
#define RMT_BUFFER_SYMBOLS   48U

static TaskHandle_t s_motion_task_handle = NULL;

/* --------------------------------------------------------------------------
 * Local motion state — active command being executed by the task.
 * -------------------------------------------------------------------------- */
static struct {
    MotionCommandType active_cmd_type;
    float ra_target;
    float dec_target;
    float ra_start; /* position captured at motion start (for ramps) */
    float dec_start;
    bool motion_active;
} s_motion;

/* --------------------------------------------------------------------------
 * Slew acceleration / deceleration
 * -------------------------------------------------------------------------- */

/*
 * Minimum slew velocity in centidegrees/second — floor for ramp curves.
 */
#define MIN_SLEW_CDS 80

/* Distance thresholds in centidegrees for ramp-profile selection. */
#define SHORT_SLEW_CDS   200   /* constant slow speed below this */
#define GENTLE_SLEW_CDS  800   /* cap target speed below this    */
#define FAST_SLEW_CDS   3500   /* aggressive profile above this  */

/*
 * Velocity profiles — 2 rows × 100 columns, each value is the
 * percentage of (target_vel − MIN_SLEW_CDS) added on top of the floor.
 *
 * Row 0 — gentle  (30 % linear accel, 40 % cruise, 30 % linear decel)
 * Row 1 — aggressive (10 % quadratic accel, 60 % cruise, 30 % linear decel)
 */
static const uint8_t VELOCITY_CURVE[2][100] = {
    {
        /* Row 0 — gentle profile */
        0, 3, 7, 10, 14, 17, 21, 24, 28, 31,
        34, 38, 41, 45, 48, 52, 55, 59, 62, 66,
        69, 72, 76, 79, 83, 86, 90, 93, 97, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 97, 93, 90, 86, 83, 79, 76, 72, 69,
        66, 62, 59, 55, 52, 48, 45, 41, 38, 34,
        31, 28, 24, 21, 17, 14, 10, 7, 3, 0,
    },
    {
        /* Row 1 — aggressive profile */
        0, 5, 10, 15, 21, 26, 30, 34, 38, 42, 48, 53, 59, 63, 69, 74, 79, 83, 88, 93,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
        93, 88, 83, 79, 74, 69, 63, 59, 53, 48, 42, 38, 34, 30, 24, 21, 15, 10, 5, 0,
    },
};

/*
 * Compute the effective velocity for a single axis during a slew.
 *
 * Parameters are in centidegrees / centidegrees-per-second (×100 integers).
 * Only the return value is converted back to deg/s (float).
 */
static float ramp_velocity(int target_vel_cds, int position_cds,
                           int start_position_cds, int distance_cds) {
    if (target_vel_cds == 0)
        return 0.0f;

    if (distance_cds == 0)
        return (float) target_vel_cds / 100.0f;

    if (distance_cds < SHORT_SLEW_CDS)
        return (float) MIN_SLEW_CDS / 100.0f;

    int capped_vel = target_vel_cds;
    if (distance_cds < GENTLE_SLEW_CDS) {
        int speed_limit = MIN_SLEW_CDS * 4;
        if (capped_vel > speed_limit) capped_vel = speed_limit;
    }

    int curve = (distance_cds >= FAST_SLEW_CDS
                 && motors_state.status == MOTORS_STATUS_SLEWING)
                    ? 1
                    : 0;

    int travelled = position_cds - start_position_cds;
    if (travelled < 0) travelled = -travelled;
    int percent_index = (int) ((int64_t) travelled * 99 / distance_cds);

    int vel = MIN_SLEW_CDS + (capped_vel - MIN_SLEW_CDS) * VELOCITY_CURVE[curve][percent_index] / 100;
    return (float) vel / 100.0f;
}

/* --------------------------------------------------------------------------
 * Step period helpers
 * -------------------------------------------------------------------------- */

/*
 * Convert an angular speed (deg/s) to a step period in microseconds.
 * Uses the runtime microstep resolution from the TMC driver.
 * Returns MAX_STEP_PERIOD_US when velocity is effectively zero.
 */
static uint32_t step_period_us(float velocity_dps) {
    if (fabsf(velocity_dps) < 1e-9f)
        return MAX_STEP_PERIOD_US;

    float deg_per_step = motors_get_deg_per_microstep();
    float period_s = deg_per_step / fabsf(velocity_dps);
    uint32_t us = (uint32_t) (period_s * 1e6f);
    return (us == 0) ? 1 : us;
}

/* --------------------------------------------------------------------------
 * Direction helper — constant for the duration of a slew.
 * -------------------------------------------------------------------------- */

static MotorDirection slew_direction(float current, float target) {
    return (target > current) ? MOTOR_DIRECTION_POSITIVE
                              : MOTOR_DIRECTION_NEGATIVE;
}

/* --------------------------------------------------------------------------
 * Stop the active motion loop from outside the motion task.
 * Aborts any in-flight RMT transmission so the motion task wakes
 * immediately and can check s_motion.motion_active.
 *
 * Safe to call from any task.
 * -------------------------------------------------------------------------- */
void motors_motion_stop(void) {
    s_motion.motion_active = false;
    motors_rmt_abort_both();
}

/* --------------------------------------------------------------------------
 * Motion conditions check — returns false when the current motion should end.
 * -------------------------------------------------------------------------- */
static bool check_motion_conditions(float deg_per_step) {
    float half_step = deg_per_step * 0.5f;
    bool ra_has_target =
            fabsf(s_motion.ra_target - motors_state.ra_position) >= half_step;
    bool dec_has_target =
            fabsf(s_motion.dec_target - motors_state.dec_position) >= half_step;

    /*
     * Slew / move-axis completion: both axes at target.
     *
     * Use active_cmd_type rather than motors_state.status because the
     * caller may have already set status to TRACKING (resume-after-slew
     * pattern in motors_slew_to_angle / motors_slew_axis_*) before the
     * motion task reaches the target.  Checking the authoritative
     * s_motion field guarantees completion is always detected.
     */
    if ((s_motion.active_cmd_type == MOTION_CMD_SLEW ||
         s_motion.active_cmd_type == MOTION_CMD_MOVE_AXIS) &&
        !ra_has_target && !dec_has_target) {
        motors_state.status = MOTORS_STATUS_READY;
        motors_state.tracking = TRACKING_NONE;
        s_motion.motion_active = false;

        return false;
    }

    /* External tracking stop. */
    if (motors_state.tracking == TRACKING_NONE &&
        motors_state.status == MOTORS_STATUS_TRACKING) {
        motors_state.status = MOTORS_STATUS_READY;
        s_motion.motion_active = false;

        return false;
    }

    return true;
}

/* --------------------------------------------------------------------------
 * Command processing — handle one MotionCommand and set up motion state.
 *
 * Only motion-producing commands (SLEW, TRACK, MOVE_AXIS) go through
 * the queue.  Stop / park / disable / enable are handled directly
 * by their callers via motors_motion_stop() + motors_state update.
 * -------------------------------------------------------------------------- */
static void process_command(MotionCommand cmd) {
    s_motion.active_cmd_type = cmd.type;

    switch (cmd.type) {
        case MOTION_CMD_SLEW:
            motors_state.ra_speed = cmd.ra_speed;
            motors_state.dec_speed = cmd.dec_speed;
            motors_state.status = MOTORS_STATUS_SLEWING;
            motors_state.tracking = TRACKING_NONE;

            if (cmd.relative) {
                s_motion.ra_target = motors_state.ra_position + cmd.ra_delta_deg;
                s_motion.dec_target = motors_state.dec_position + cmd.dec_delta_deg;
            } else {
                s_motion.ra_target = cmd.ra_target_deg;
                s_motion.dec_target = cmd.dec_target_deg;
            }
            s_motion.ra_start = motors_state.ra_position;
            s_motion.dec_start = motors_state.dec_position;
            s_motion.motion_active = true;

            break;

        case MOTION_CMD_TRACK:
            motors_state.ra_speed = cmd.ra_speed;
            motors_state.dec_speed = 0.0f;
            motors_state.status = MOTORS_STATUS_TRACKING;
            motors_state.tracking = cmd.tracking_mode;

            /*
             * Tracking runs open-ended: target is set to the axis limit so
             * the loop never completes on its own — it only stops when an
             * external status change (STOP, PARK) is detected.
             *
             * Hemisphere selection: positive velocity → ra_max (northern),
             * negative velocity → ra_min (southern).  The sign is set by
             * motors_start_tracking based on site latitude.
             */
            s_motion.ra_target = (cmd.ra_speed >= 0.0f)
                                     ? motors_state.limits.ra_max
                                     : motors_state.limits.ra_min;
            s_motion.dec_target = motors_state.dec_position;
            s_motion.ra_start = motors_state.ra_position;
            s_motion.dec_start = motors_state.dec_position;
            s_motion.motion_active = true;

            break;

        case MOTION_CMD_MOVE_AXIS:
            motors_state.ra_speed = fabsf(cmd.ra_speed);
            motors_state.dec_speed = fabsf(cmd.dec_speed);
            motors_state.status = MOTORS_STATUS_SLEWING;
            motors_state.tracking = TRACKING_NONE;

            s_motion.ra_target = (cmd.ra_speed > 0.0f)
                                     ? motors_state.limits.ra_max
                                     : (cmd.ra_speed < 0.0f)
                                           ? motors_state.limits.ra_min
                                           : motors_state.ra_position;
            s_motion.dec_target = (cmd.dec_speed > 0.0f)
                                      ? motors_state.limits.dec_max
                                      : (cmd.dec_speed < 0.0f)
                                            ? motors_state.limits.dec_min
                                            : motors_state.dec_position;

            s_motion.ra_start = motors_state.ra_position;
            s_motion.dec_start = motors_state.dec_position;
            s_motion.motion_active = true;

            break;
    }
}

/* --------------------------------------------------------------------------
 * Slewing & move-axis motion loop — RMT batch scheduling.
 *
 * Replaces software busy-wait with DMA-driven step generation:
 *
 *   1. Determine direction (constant for the entire slew) and set DIR pins.
 *   2. Every ~5 ms recalculate velocity via the ramp curve.
 *   3. Every ~20 ms compute a batch of steps at the current velocity,
 *      encode them as RMT symbols, and transmit via DMA.
 *   4. Block on a semaphore while the RMT peripheral + DMA handle the
 *      step timing entirely in hardware — zero CPU, zero jitter.
 *   5. On wake-up, update positions by the full batch count and loop.
 *
 * MOVE_AXIS (joystick / NINA centering) skips the ramp — constant
 * velocity from the first microstep so the client's time × rate
 * distance calculations are accurate.
 * -------------------------------------------------------------------------- */
static void slewing_loop_rmt(void) {
    float deg_per_step = motors_get_deg_per_microstep();

    /* Precompute total distances in centidegrees. */
    int distance_ra_cds = (int) (fabsf(s_motion.ra_target - s_motion.ra_start) * 100.0f);
    int distance_dec_cds = (int) (fabsf(s_motion.dec_target - s_motion.dec_start) * 100.0f);

    /* Total steps per axis. */
    uint32_t total_ra_steps = (uint32_t) (fabsf(s_motion.ra_target - s_motion.ra_start)
                                          / deg_per_step + 0.5f);
    uint32_t total_dec_steps = (uint32_t) (fabsf(s_motion.dec_target - s_motion.dec_start)
                                           / deg_per_step + 0.5f);
    uint32_t ra_steps_done = 0;
    uint32_t dec_steps_done = 0;

    /*
     * Direction is constant for a slew — set DIR pins once.
     * TMC2209 requires DIR stable ≥ 200 ns before STEP rising edge.
     * The GPIO write here precedes the first rmt_transmit() by
     * at least several microseconds (function call + DMA setup),
     * providing ample margin.
     */
    MotorDirection ra_dir = slew_direction(motors_state.ra_position,
                                            s_motion.ra_target);
    MotorDirection dec_dir = slew_direction(motors_state.dec_position,
                                             s_motion.dec_target);
    motors_hw_set_direction_ra(ra_dir);
    motors_hw_set_direction_dec(dec_dir);

    float ra_sign = (ra_dir == MOTOR_DIRECTION_POSITIVE) ? 1.0f : -1.0f;
    float dec_sign = (dec_dir == MOTOR_DIRECTION_POSITIVE) ? 1.0f : -1.0f;

    /* RMT symbol buffers — stack-allocated, DMA-safe on ESP32-S3. */
    rmt_symbol_word_t ra_symbols[RMT_BUFFER_SYMBOLS];
    rmt_symbol_word_t dec_symbols[RMT_BUFFER_SYMBOLS];

    uint32_t ra_period = MAX_STEP_PERIOD_US;
    uint32_t dec_period = MAX_STEP_PERIOD_US;
    int64_t last_ramp_recalc_us = 0;

    while (s_motion.motion_active) {
        /*
         * 1. Throttled motion-conditions check — exit if target reached
         *    or tracking was stopped externally.
         */
        if (!check_motion_conditions(deg_per_step)) break;
        if (!s_motion.motion_active) break;

        int64_t now = esp_timer_get_time();

        /*
         * 2. Recalculate velocities every ~5 ms (same cadence as the
         *    original software loop).
         *
         *    MOVE_AXIS skips the ramp — constant commanded speed.
         */
        if (now - last_ramp_recalc_us > 5000) {
            if (s_motion.active_cmd_type == MOTION_CMD_MOVE_AXIS) {
                ra_period = step_period_us(motors_state.ra_speed);
                dec_period = step_period_us(motors_state.dec_speed);
            } else {
                int target_ra = (int) (motors_state.ra_speed * 100.0f);
                int target_dec = (int) (motors_state.dec_speed * 100.0f);
                int pos_ra = (int) (motors_state.ra_position * 100.0f);
                int pos_dec = (int) (motors_state.dec_position * 100.0f);

                float ra_vel = ramp_velocity(target_ra, pos_ra,
                                             (int) (s_motion.ra_start * 100.0f),
                                             distance_ra_cds);
                float dec_vel = ramp_velocity(target_dec, pos_dec,
                                              (int) (s_motion.dec_start * 100.0f),
                                              distance_dec_cds);
                ra_period = step_period_us(ra_vel);
                dec_period = step_period_us(dec_vel);
            }
            last_ramp_recalc_us = now;
        }

        /*
         * 3. Compute batch sizes — target ~20 ms of motion per batch.
         */
        uint32_t ra_batch = (ra_period < RMT_BATCH_TARGET_US)
                                ? (RMT_BATCH_TARGET_US / ra_period)
                                : 1;
        uint32_t dec_batch = (dec_period < RMT_BATCH_TARGET_US)
                                 ? (RMT_BATCH_TARGET_US / dec_period)
                                 : 1;

        /* Clamp to remaining steps. */
        uint32_t ra_remaining = total_ra_steps - ra_steps_done;
        uint32_t dec_remaining = total_dec_steps - dec_steps_done;
        if (ra_batch > ra_remaining) ra_batch = ra_remaining;
        if (dec_batch > dec_remaining) dec_batch = dec_remaining;
        if (ra_batch > RMT_BATCH_MAX_STEPS) ra_batch = RMT_BATCH_MAX_STEPS;
        if (dec_batch > RMT_BATCH_MAX_STEPS) dec_batch = RMT_BATCH_MAX_STEPS;

        if (ra_batch == 0 && dec_batch == 0) break;

        /*
         * 4. Encode step batches as RMT symbols.
         *
         *    For MOVE_AXIS (constant velocity) the entire batch uses
         *    a single period — one encode call per axis.
         *
         *    For SLEW with ramps, velocity is recalculated per-step
         *    within the batch to preserve the ramp curve's precision
         *    (equivalent to the original per-step software loop).
         */
        uint32_t ra_num_sym = 0;
        uint32_t dec_num_sym = 0;

        if (ra_batch > 0) {
            if (s_motion.active_cmd_type == MOTION_CMD_MOVE_AXIS) {
                ra_num_sym = motors_rmt_encode_steps(ra_symbols,
                                                      RMT_BUFFER_SYMBOLS,
                                                      ra_period, ra_batch);
            } else {
                uint32_t remaining_sym = RMT_BUFFER_SYMBOLS;
                rmt_symbol_word_t *sym = ra_symbols;
                for (uint32_t i = 0; i < ra_batch && remaining_sym > 0; i++) {
                    /* Project position for this step within the batch. */
                    float step_pos = s_motion.ra_start
                                     + ra_sign * deg_per_step
                                           * (float) (ra_steps_done + i);
                    int pos_cds = (int) (step_pos * 100.0f);
                    float vel = ramp_velocity(
                        (int) (motors_state.ra_speed * 100.0f),
                        pos_cds,
                        (int) (s_motion.ra_start * 100.0f),
                        distance_ra_cds);
                    uint32_t period = step_period_us(vel);
                    uint32_t n = motors_rmt_encode_steps(sym, remaining_sym,
                                                          period, 1);
                    sym += n;
                    remaining_sym -= n;
                    ra_num_sym += n;
                }
            }
        }

        if (dec_batch > 0) {
            if (s_motion.active_cmd_type == MOTION_CMD_MOVE_AXIS) {
                dec_num_sym = motors_rmt_encode_steps(dec_symbols,
                                                       RMT_BUFFER_SYMBOLS,
                                                       dec_period, dec_batch);
            } else {
                uint32_t remaining_sym = RMT_BUFFER_SYMBOLS;
                rmt_symbol_word_t *sym = dec_symbols;
                for (uint32_t i = 0; i < dec_batch && remaining_sym > 0; i++) {
                    float step_pos = s_motion.dec_start
                                     + dec_sign * deg_per_step
                                           * (float) (dec_steps_done + i);
                    int pos_cds = (int) (step_pos * 100.0f);
                    float vel = ramp_velocity(
                        (int) (motors_state.dec_speed * 100.0f),
                        pos_cds,
                        (int) (s_motion.dec_start * 100.0f),
                        distance_dec_cds);
                    uint32_t period = step_period_us(vel);
                    uint32_t n = motors_rmt_encode_steps(sym, remaining_sym,
                                                          period, 1);
                    sym += n;
                    remaining_sym -= n;
                    dec_num_sym += n;
                }
            }
        }

        /*
         * 5. Transmit both axes in parallel (non-blocking).
         */
        if (ra_batch > 0) {
            motors_rmt_transmit_ra(ra_symbols, ra_num_sym);
        }
        if (dec_batch > 0) {
            motors_rmt_transmit_dec(dec_symbols, dec_num_sym);
        }

        /*
         * 6. Block until both transmissions complete.
         *    The RMT ISR gives the semaphore on completion.
         *    A STOP / PARK / DISABLE aborts the channel and gives
         *    the semaphore immediately, so we check motion_active below.
         */
        if (ra_batch > 0) {
            motors_rmt_wait_ra(pdMS_TO_TICKS(500));
        }
        if (dec_batch > 0) {
            motors_rmt_wait_dec(pdMS_TO_TICKS(500));
        }

        /* 7. External stop preemption. */
        if (!s_motion.motion_active) break;

        /*
         * 8. Update positions — the full batch completed successfully.
         */
        motors_state.ra_position += ra_sign * deg_per_step * (float) ra_batch;
        motors_state.dec_position += dec_sign * deg_per_step * (float) dec_batch;
        ra_steps_done += ra_batch;
        dec_steps_done += dec_batch;

        /*
         * 9. Target reached?  check_motion_conditions at the top of the
         *    loop will catch this, but an early exit here avoids one
         *    unnecessary batch encode.
         */
        if (ra_steps_done >= total_ra_steps && dec_steps_done >= total_dec_steps) {
            motors_state.status = MOTORS_STATUS_READY;
            motors_state.tracking = TRACKING_NONE;
            s_motion.motion_active = false;
            break;
        }
    }
}

/* --------------------------------------------------------------------------
 * Tracking motion loop — absolute-time fractional accumulator + RMT pulse.
 *
 * Designed for continuous open-ended tracking (sidereal, solar, lunar)
 * where timing precision must hold over arbitrarily long sessions.
 *
 * Scheduling strategy (hybrid sleep + fine-wait → RMT):
 *
 *   dt = now - last_time
 *   accumulator += dt / period_us
 *   while (accumulator >= 1.0):  encode 1 step, rmt_transmit, wait, acc -= 1.0
 *
 *   deadline = now + (1.0 - accumulator) * period_us    (µs-exact)
 *   if deadline - now > 2 ms:
 *       vTaskDelay most of it (capped 50 ms, yields CPU → near-zero consumption)
 *   fine-wait remaining margin with busy-wait → µs precision
 *
 * The actual STEP pulse is generated by the RMT peripheral with zero
 * jitter.  The fine-wait determines *when* the pulse begins; the RMT
 * determines the pulse *shape*.  Long idle periods (tracking ≈ 841 ms)
 * are streamed as idle-only RMT symbols via DMA — the CPU sleeps
 * through the entire step period.
 *
 * Only RA is stepped during tracking; DEC velocity is always zero.
 * -------------------------------------------------------------------------- */
static void tracking_loop_rmt(void) {
    float deg_per_step = motors_get_deg_per_microstep();
    uint32_t period_us = step_period_us(motors_state.ra_speed);

    /*
     * Fine-wait margin: sleep via vTaskDelay until this many µs before
     * the deadline, then busy-wait the remainder for µs precision.
     */
    const int64_t FINE_MARGIN_US = 2000; /* 2 ms */

    /* Fractional-step accumulator (double avoids single-precision drift). */
    double accumulator = 0.0;
    int64_t last_time_us = esp_timer_get_time();
    int64_t last_check_us = last_time_us;

    /* Direction and sign for RA tracking. */
    float ra_sign = (motors_state.ra_speed >= 0.0f) ? 1.0f : -1.0f;
    MotorDirection ra_dir = (ra_sign > 0.0f)
                            ? MOTOR_DIRECTION_POSITIVE
                            : MOTOR_DIRECTION_NEGATIVE;
    motors_hw_set_direction_ra(ra_dir);

    /* Single-step RMT symbol buffer. */
    rmt_symbol_word_t ra_symbols[RMT_BUFFER_SYMBOLS];

    while (s_motion.motion_active) {
        int64_t now = esp_timer_get_time();
        int64_t dt_us = now - last_time_us;
        last_time_us = now;

        /* Accumulate fractional microsteps since last iteration. */
        if (dt_us > 0) {
            accumulator += (double) dt_us / (double) period_us;
        }

        /* ------------------------------------------------------------------
         * Throttled conditions check (every ~500 µs).
         * ------------------------------------------------------------------ */
        if (now - last_check_us >= 500) {
            last_check_us = now;

            if (!check_motion_conditions(deg_per_step)) break;

            if (motors_state.status != MOTORS_STATUS_TRACKING ||
                motors_state.tracking == TRACKING_NONE) {
                break;
            }
        }

        /* ------------------------------------------------------------------
         * Emit accumulated whole steps via RMT.
         *
         * Each step is encoded, transmitted, and waited-on.  For tracking
         * speeds (~841 ms period), the wait blocks the task but the RMT
         * abort path (motors_rmt_abort_ra) wakes it immediately on STOP.
         * ------------------------------------------------------------------ */
        while (accumulator >= 1.0) {
            /* Position validation. */
            float next_pos = motors_state.ra_position
                             + (deg_per_step * ra_sign);
            if (!motors_is_valid_ra(next_pos)) {
                s_motion.motion_active = false;
                motors_state.status = MOTORS_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                return;
            }

            /* Encode one step, transmit, wait for hardware completion. */
            uint32_t n = motors_rmt_encode_steps(ra_symbols,
                                                  RMT_BUFFER_SYMBOLS,
                                                  period_us, 1);
            motors_rmt_transmit_ra(ra_symbols, n);
            motors_rmt_wait_ra(pdMS_TO_TICKS(2000));

            if (!s_motion.motion_active) return;

            motors_state.ra_position = next_pos;
            accumulator -= 1.0;
        }

        /* ------------------------------------------------------------------
         * Hybrid sleep + fine-wait.
         *
         * Compute the exact deadline of the next whole step, sleep most of
         * the interval yielding the CPU, then fine-wait the final margin
         * for µs-precise step timing.
         *
         * If the deadline is still far away after one sleep chunk (e.g.
         * because the period is huge or the sleep was capped at 50 ms),
         * loop back to the outer while — re-accumulate dt, re-check
         * conditions, and re-sleep.  This prevents unbounded CPU spin
         * when velocity is near zero (degenerate tracking).
         * ------------------------------------------------------------------ */
        int64_t deadline = now + (int64_t) ((1.0 - accumulator) * (double) period_us);
        int64_t wait_us = deadline - esp_timer_get_time();

        if (wait_us > FINE_MARGIN_US) {
            /*
             * Sleep the bulk of the wait via vTaskDelay, capped at 50 ms
             * to keep the command queue responsive, then loop back.
             */
            int64_t sleep_us = wait_us - FINE_MARGIN_US;
            uint32_t sleep_ms = (sleep_us / 1000 > 50)
                                    ? 50
                                    : (uint32_t) (sleep_us / 1000);
            if (sleep_ms < 1) sleep_ms = 1;
            vTaskDelay(pdMS_TO_TICKS(sleep_ms));
            continue;
        }

        /*
         * Fine-wait the remaining margin with busy-wait.
         * We only reach here when wait_us <= FINE_MARGIN_US (~2 ms),
         * so the spin is bounded and safe.
         */
        while (esp_timer_get_time() < deadline) {
            if ((esp_timer_get_time() & 0x1FF) == 0) {
                taskYIELD(); /* reset task WDT, let other tasks run */
            }
        }
    }
}

/* --------------------------------------------------------------------------
 * Motion loop dispatcher.
 *
 * Routes to the appropriate execution path based on mount status:
 *   - TRACKING (sidereal / solar / lunar) → tracking_loop_rmt()
 *     (absolute-time fractional accumulator, zero cumulative error)
 *   - Everything else (SLEWING, MOVE_AXIS, etc.) → slewing_loop_rmt()
 *     (batched RMT with ramps)
 * -------------------------------------------------------------------------- */
static void motion_loop(void) {
    if (motors_state.status == MOTORS_STATUS_TRACKING
        && motors_state.tracking != TRACKING_NONE) {
        tracking_loop_rmt();
    } else {
        slewing_loop_rmt();
    }
}

/* --------------------------------------------------------------------------
 * Motion task entry point.
 *
 * Blocks on the command queue when idle. When a motion-producing command
 * arrives (SLEW, TRACK, or MOVE_AXIS), enters motion_loop() which dispatches
 * to the appropriate RMT-driven execution path.
 * Stop / park / disable / enable are handled directly by their
 * callers via motors_motion_stop() + motors_state update.
 * -------------------------------------------------------------------------- */
static void motors_motion_task_run(void *arg) {
    (void) arg;

    while (true) {
        MotionCommand cmd;
        if (xQueueReceive(motion_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE)
            continue;

        process_command(cmd);

        if (cmd.type == MOTION_CMD_SLEW || cmd.type == MOTION_CMD_TRACK ||
            cmd.type == MOTION_CMD_MOVE_AXIS) {
            motion_loop();
        }
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void motors_motion_task_init(void) {
    xTaskCreate(
        motors_motion_task_run,
        "motors_motion",
        MOTION_TASK_STACK_WORDS,
        NULL,
        MOTION_TASK_PRIORITY,
        &s_motion_task_handle);

    /* Report stack high-water mark for diagnostics. */
    if (s_motion_task_handle != NULL) {
        UBaseType_t high_water = uxTaskGetStackHighWaterMark(s_motion_task_handle);
        ESP_LOGI(TAG, "Stack high-water mark: %lu words (total %d)",
                 (unsigned long) high_water, MOTION_TASK_STACK_WORDS);
    }
}
