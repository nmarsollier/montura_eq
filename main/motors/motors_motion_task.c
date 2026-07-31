/* Motors - motors_motion_task.c
 *
 * Purpose: FreeRTOS motion task — consumes MotionCommands from the queue
 * and drives axis positions via RMT+DMA step pulse generation.
 *
 * The motion task is the sole writer of motors_state position fields
 * (ra_steps, dec_steps) during slews and tracking.
 * Status and tracking fields are updated cooperatively by the motion
 * task, motors_stop(), motors_park(), and motors_enable().
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

/* Step period ceiling in RMT ticks — used when velocity is zero or unknown. */
#define MAX_STEP_PERIOD_TICKS (20U * 1000U * 1000U) /* 10 s at 2 MHz */

/* --------------------------------------------------------------------------
 * RMT batch control
 * -------------------------------------------------------------------------- */

/*
 * Target batch duration in RMT ticks.  Shorter batches give finer ramp
 * granularity but increase CPU overhead.  40k ticks = 20 ms at 2 MHz.
 */
#define RMT_BATCH_TARGET_TICKS  40000U

/*
 * Safety cap — never queue more than this many steps in one batch.
 *
 * On abort / timeout the position is optimistically advanced by the
 * full batch.  Keeping this small caps the worst-case position error.
 * 12 steps × ~0.00028°/step ≈ 0.003° with the current gear ratio.
 */
#define RMT_BATCH_MAX_STEPS  12U

/*
 * RMT symbol buffer capacity.  Must not exceed the non-DMA channel's
 * internal memory (SOC_RMT_MEM_WORDS_PER_CHANNEL = 48 on ESP32-S3).
 * The DMA channel (RA) uses this same size for simplicity.
 */
#define RMT_BUFFER_SYMBOLS   48U

TaskHandle_t motors_motion_task_handle = NULL;

/* --------------------------------------------------------------------------
 * Motion state — active command being executed by the task.
 *
 * All fields are single-writer (motion task only) except `active`, which
 * is set false by motors_motion_stop() from any task to signal abort.
 * -------------------------------------------------------------------------- */
static struct {
    bool active;               /* false → abort at next iteration */
    MotionCommandType active_cmd_type;
    int64_t ra_target;         /* steps */
    int64_t dec_target;        /* steps */
    int64_t ra_start;          /* steps — captured at motion start (for ramps) */
    int64_t dec_start;         /* steps */

    /* PulseGuide state — single-writer (motion task only). */
    int64_t ra_guide_deadline_us;
    int64_t dec_guide_deadline_us;
    float ra_guide_offset_dps;
    float dec_guide_offset_dps;
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
 * target_vel_cds — target velocity in centidegrees/second (user-specified).
 * travelled_steps — steps taken so far in this slew.
 * distance_steps — total step count for this slew.
 * distance_cds   — total angular distance in centidegrees (precomputed).
 *
 * Returns velocity in deg/s (float).
 */
static float ramp_velocity(int target_vel_cds, int64_t travelled_steps,
                           int64_t distance_steps, int distance_cds) {
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

    if (travelled_steps < 0) travelled_steps = -travelled_steps;
    int percent_index = (int) ((int64_t) travelled_steps * 99 / distance_steps);

    int vel = MIN_SLEW_CDS + (capped_vel - MIN_SLEW_CDS) * VELOCITY_CURVE[curve][percent_index] / 100;
    return (float) vel / 100.0f;
}

/* --------------------------------------------------------------------------
 * Step period helpers
 * -------------------------------------------------------------------------- */

/*
 * Convert an angular speed (deg/s) to a step period in RMT ticks.
 * Uses the runtime microstep resolution from the TMC driver.
 * Returns MAX_STEP_PERIOD_TICKS when velocity is effectively zero.
 */
static uint32_t step_period_ticks(float velocity_dps) {
    if (fabsf(velocity_dps) < 1e-9f)
        return MAX_STEP_PERIOD_TICKS;

    float deg_per_step = motors_get_deg_per_microstep();
    float period_s = deg_per_step / fabsf(velocity_dps);
    uint32_t ticks = (uint32_t) (period_s * (float) RMT_RESOLUTION_HZ);
    if (ticks < STEP_MIN_PERIOD_TICKS)
        ticks = STEP_MIN_PERIOD_TICKS;
    return ticks;
}

/* --------------------------------------------------------------------------
 * Stop the active motion loop from outside the motion task.
 * Aborts any in-flight RMT transmission and notifies the task so it
 * wakes immediately and can check s_motion.active.
 *
 * Safe to call from any task.
 * -------------------------------------------------------------------------- */
void motors_motion_stop(void) {
    s_motion.active = false;
    motors_rmt_abort_both();
    if (motors_motion_task_handle) {
        xTaskNotify(motors_motion_task_handle, 0, eNoAction);
    }
}

/* --------------------------------------------------------------------------
 * Motion conditions check — returns false when the current motion should end.
 * -------------------------------------------------------------------------- */
static bool check_motion_conditions(void) {
    /*
     * Motor ERROR — immediate abort of any active motion.
     *
     * If the motors subsystem enters ERROR state while slewing or
     * tracking, kill RMT output and physically disable the drivers
     * on the spot.  This is the emergency cut-off: no deceleration,
     * no position cleanup — just stop NOW.
     */
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        motors_enter_error_state();
        s_motion.active = false;
        return false;
    }

    int64_t ra_diff = s_motion.ra_target - motors_state.ra_steps;
    if (ra_diff < 0) ra_diff = -ra_diff;
    int64_t dec_diff = s_motion.dec_target - motors_state.dec_steps;
    if (dec_diff < 0) dec_diff = -dec_diff;

    bool ra_has_target = (ra_diff > 0);
    bool dec_has_target = (dec_diff > 0);

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
        s_motion.active = false;

        return false;
    }

    /* External tracking stop. */
    if (motors_state.tracking == TRACKING_NONE &&
        motors_state.status == MOTORS_STATUS_TRACKING) {
        motors_state.status = MOTORS_STATUS_READY;
        s_motion.active = false;

        return false;
    }

    return true;
}

/* --------------------------------------------------------------------------
 * Command processing — handle one MotionCommand and set up motion state.
 *
 * Only motion-producing commands (SLEW, TRACK, MOVE_AXIS) go through
 * the queue.  Stop / park / enable are handled directly by their
 * callers via motors_motion_stop() + motors_state update.
 * -------------------------------------------------------------------------- */
static void process_command(MotionCommand cmd) {
    /*
     * If motors entered ERROR while this command was queued, reject it
     * without touching state.  The mount layer already guards against
     * enqueuing commands in ERROR, but a race between enqueue and
     * error-detection could still deliver a stale command here.
     */
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        return;
    }

    s_motion.active_cmd_type = cmd.type;

    switch (cmd.type) {
        case MOTION_CMD_SLEW:
            motors_state.ra_speed = cmd.ra_speed;
            motors_state.dec_speed = cmd.dec_speed;
            motors_state.status = MOTORS_STATUS_SLEWING;
            motors_state.tracking = TRACKING_NONE;

            if (cmd.relative) {
                s_motion.ra_target = motors_state.ra_steps
                                     + motors_deg_to_steps(cmd.ra_delta_deg);
                s_motion.dec_target = motors_state.dec_steps
                                      + motors_deg_to_steps(cmd.dec_delta_deg);
            } else {
                s_motion.ra_target = motors_deg_to_steps(cmd.ra_target_deg);
                s_motion.dec_target = motors_deg_to_steps(cmd.dec_target_deg);
            }
            s_motion.ra_start = motors_state.ra_steps;
            s_motion.dec_start = motors_state.dec_steps;
            s_motion.active = true;

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
                                     ? motors_deg_to_steps(motors_state.limits.ra_max)
                                     : motors_deg_to_steps(motors_state.limits.ra_min);
            s_motion.dec_target = motors_state.dec_steps;
            s_motion.ra_start = motors_state.ra_steps;
            s_motion.dec_start = motors_state.dec_steps;
            s_motion.active = true;

            break;

        case MOTION_CMD_MOVE_AXIS:
            motors_state.ra_speed = fabsf(cmd.ra_speed);
            motors_state.dec_speed = fabsf(cmd.dec_speed);
            motors_state.status = MOTORS_STATUS_SLEWING;
            motors_state.tracking = TRACKING_NONE;

            s_motion.ra_target = (cmd.ra_speed > 0.0f)
                                     ? motors_deg_to_steps(motors_state.limits.ra_max)
                                     : (cmd.ra_speed < 0.0f)
                                           ? motors_deg_to_steps(motors_state.limits.ra_min)
                                           : motors_state.ra_steps;
            s_motion.dec_target = (cmd.dec_speed > 0.0f)
                                      ? motors_deg_to_steps(motors_state.limits.dec_max)
                                      : (cmd.dec_speed < 0.0f)
                                            ? motors_deg_to_steps(motors_state.limits.dec_min)
                                            : motors_state.dec_steps;

            s_motion.ra_start = motors_state.ra_steps;
            s_motion.dec_start = motors_state.dec_steps;
            s_motion.active = true;

            break;

        case MOTION_CMD_PULSE_GUIDE:
            /* Set or extend the guide for one axis.  Collision policy:
             * same dir extends deadline, opposite dir replaces. */
            if (cmd.guide_axis == 0) {
                if (s_motion.ra_guide_deadline_us
                    && s_motion.ra_guide_offset_dps == cmd.guide_offset_dps) {
                    int64_t dl = esp_timer_get_time()
                               + (int64_t)cmd.guide_duration_ms * 1000;
                    if (dl > s_motion.ra_guide_deadline_us)
                        s_motion.ra_guide_deadline_us = dl;
                } else {
                    s_motion.ra_guide_deadline_us = esp_timer_get_time()
                        + (int64_t)cmd.guide_duration_ms * 1000;
                    s_motion.ra_guide_offset_dps = cmd.guide_offset_dps;
                }
            } else {
                if (s_motion.dec_guide_deadline_us
                    && s_motion.dec_guide_offset_dps == cmd.guide_offset_dps) {
                    int64_t dl = esp_timer_get_time()
                               + (int64_t)cmd.guide_duration_ms * 1000;
                    if (dl > s_motion.dec_guide_deadline_us)
                        s_motion.dec_guide_deadline_us = dl;
                } else {
                    s_motion.dec_guide_deadline_us = esp_timer_get_time()
                        + (int64_t)cmd.guide_duration_ms * 1000;
                    s_motion.dec_guide_offset_dps = cmd.guide_offset_dps;
                }
            }
            motors_state.guiding = true;
            /* Don't set s_motion.active — the existing loop handles it. */
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
    /* Total step counts — exact from int64_t targets. */
    int64_t ra_dist = s_motion.ra_target - s_motion.ra_start;
    int64_t dec_dist = s_motion.dec_target - s_motion.dec_start;
    if (ra_dist < 0) ra_dist = -ra_dist;
    if (dec_dist < 0) dec_dist = -dec_dist;
    uint32_t total_ra_steps = (uint32_t) ra_dist;
    uint32_t total_dec_steps = (uint32_t) dec_dist;
    uint32_t ra_steps_done = 0;
    uint32_t dec_steps_done = 0;

    /* Distances in centidegrees for ramp threshold comparisons. */
    int distance_ra_cds = (int)(motors_steps_to_deg(ra_dist) * 100.0f);
    int distance_dec_cds = (int)(motors_steps_to_deg(dec_dist) * 100.0f);

    /*
     * Direction is constant for a slew — set DIR pins once.
     * TMC2209 requires DIR stable ≥ 200 ns before STEP rising edge.
     * The GPIO write here precedes the first rmt_transmit() by
     * at least several microseconds (function call + DMA setup),
     * providing ample margin.
     */
    MotorDirection ra_dir = (s_motion.ra_target > motors_state.ra_steps)
                                ? MOTOR_DIRECTION_POSITIVE
                                : MOTOR_DIRECTION_NEGATIVE;
    MotorDirection dec_dir = (s_motion.dec_target > motors_state.dec_steps)
                                ? MOTOR_DIRECTION_POSITIVE
                                : MOTOR_DIRECTION_NEGATIVE;
    motors_hw_set_direction_ra(ra_dir);
    motors_hw_set_direction_dec(dec_dir);

    int ra_sign = (ra_dir == MOTOR_DIRECTION_POSITIVE) ? 1 : -1;
    int dec_sign = (dec_dir == MOTOR_DIRECTION_POSITIVE) ? 1 : -1;

    /* RMT symbol buffers — stack-allocated, DMA-safe on ESP32-S3. */
    rmt_symbol_word_t ra_symbols[RMT_BUFFER_SYMBOLS];
    rmt_symbol_word_t dec_symbols[RMT_BUFFER_SYMBOLS];

    uint32_t ra_period = MAX_STEP_PERIOD_TICKS;
    uint32_t dec_period = MAX_STEP_PERIOD_TICKS;
    int64_t last_ramp_recalc_us = 0;

    while (s_motion.active) {
        /*
         * 1. Throttled motion-conditions check — exit if target reached
         *    or tracking was stopped externally.
         */
        if (!check_motion_conditions()) break;
        if (!s_motion.active) break;

        int64_t now = esp_timer_get_time();

        /*
         * 2. Recalculate velocities every ~5 ms (same cadence as the
         *    original software loop).
         *
         *    MOVE_AXIS skips the ramp — constant commanded speed.
         */
        if (now - last_ramp_recalc_us > 5000) {
            if (s_motion.active_cmd_type == MOTION_CMD_MOVE_AXIS) {
                ra_period = step_period_ticks(motors_state.ra_speed);
                dec_period = step_period_ticks(motors_state.dec_speed);
            } else {
                int target_ra = (int) (motors_state.ra_speed * 100.0f);
                int target_dec = (int) (motors_state.dec_speed * 100.0f);
                int64_t ra_travelled = (int64_t)ra_sign * (motors_state.ra_steps - s_motion.ra_start);
                int64_t dec_travelled = (int64_t)dec_sign * (motors_state.dec_steps - s_motion.dec_start);

                float ra_vel = ramp_velocity(target_ra, ra_travelled,
                                             ra_dist, distance_ra_cds);
                float dec_vel = ramp_velocity(target_dec, dec_travelled,
                                              dec_dist, distance_dec_cds);
                ra_period = step_period_ticks(ra_vel);
                dec_period = step_period_ticks(dec_vel);
            }
            last_ramp_recalc_us = now;
        }

        /*
         * 3. Compute batch sizes — target ~20 ms of motion per batch.
         */
        uint32_t ra_batch = (ra_period < RMT_BATCH_TARGET_TICKS)
                                ? (RMT_BATCH_TARGET_TICKS / ra_period)
                                : 1;
        uint32_t dec_batch = (dec_period < RMT_BATCH_TARGET_TICKS)
                                 ? (RMT_BATCH_TARGET_TICKS / dec_period)
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
         *    within the batch to preserve the ramp curve's precision.
         */
        uint32_t ra_num_sym = 0;
        uint32_t dec_num_sym = 0;

        /* Travelled steps at batch start (signed, for ramp position). */
        int64_t ra_base_travelled = (int64_t)ra_sign
                                    * (motors_state.ra_steps - s_motion.ra_start);
        int64_t dec_base_travelled = (int64_t)dec_sign
                                     * (motors_state.dec_steps - s_motion.dec_start);

        if (ra_batch > 0) {
            if (s_motion.active_cmd_type == MOTION_CMD_MOVE_AXIS) {
                ra_num_sym = motors_rmt_encode_steps(ra_symbols,
                                                      RMT_BUFFER_SYMBOLS,
                                                      ra_period, ra_batch);
            } else {
                uint32_t remaining_sym = RMT_BUFFER_SYMBOLS;
                rmt_symbol_word_t *sym = ra_symbols;
                for (uint32_t i = 0; i < ra_batch && remaining_sym > 0; i++) {
                    int64_t travelled = ra_base_travelled + (int64_t)i;
                    float vel = ramp_velocity(
                        (int) (motors_state.ra_speed * 100.0f),
                        travelled, ra_dist, distance_ra_cds);
                    uint32_t period = step_period_ticks(vel);
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
                    int64_t travelled = dec_base_travelled + (int64_t)i;
                    float vel = ramp_velocity(
                        (int) (motors_state.dec_speed * 100.0f),
                        travelled, dec_dist, distance_dec_cds);
                    uint32_t period = step_period_ticks(vel);
                    uint32_t n = motors_rmt_encode_steps(sym, remaining_sym,
                                                          period, 1);
                    sym += n;
                    remaining_sym -= n;
                    dec_num_sym += n;
                }
            }
        }

        /*
         * 5. Update positions optimistically BEFORE transmit.
         *    If the transmit itself fails we roll back (no steps sent).
         *    On abort / timeout the position stays — conservative
         *    worst-case estimate, at most RMT_BATCH_MAX_STEPS ahead.
         */
        int64_t prev_ra = motors_state.ra_steps;
        int64_t prev_dec = motors_state.dec_steps;
        motors_state.ra_steps += (int64_t)ra_sign * (int64_t)ra_batch;
        motors_state.dec_steps += (int64_t)dec_sign * (int64_t)dec_batch;

        /*
         * 5b. Check for external stop BEFORE touching the RMT hardware.
         *     motors_motion_stop() from another task calls abort, which
         *     re-enables the channels.  Without this check we could
         *     start an unintended transmission on those re-enabled
         *     channels, physically moving the motor after the STOP.
         */
        if (!s_motion.active) {
            ESP_LOGW(TAG, "Motion aborted before transmit — position may be off by ≤%u steps",
                     RMT_BATCH_MAX_STEPS);
            break;
        }

        /*
         * 6. Transmit both axes in parallel (non-blocking).
         */
        esp_err_t ra_tx_err = ESP_OK;
        esp_err_t dec_tx_err = ESP_OK;

        if (ra_batch > 0) {
            ra_tx_err = motors_rmt_transmit_ra(ra_symbols, ra_num_sym);
        }
        if (dec_batch > 0) {
            dec_tx_err = motors_rmt_transmit_dec(dec_symbols, dec_num_sym);
        }

        /* Transmit failure → roll back (no steps were physically sent). */
        if (ra_tx_err != ESP_OK) motors_state.ra_steps = prev_ra;
        if (dec_tx_err != ESP_OK) motors_state.dec_steps = prev_dec;

        if (ra_tx_err != ESP_OK || dec_tx_err != ESP_OK) {
            ESP_LOGE(TAG, "RMT transmit failed (RA: %s, DEC: %s)",
                     esp_err_to_name(ra_tx_err),
                     esp_err_to_name(dec_tx_err));
            motors_rmt_abort_both();
            s_motion.active = false;
            motors_state.status = MOTORS_STATUS_READY;
            motors_state.tracking = TRACKING_NONE;
            break;
        }

        /*
         * 7. Block until both transmissions complete.
         *    The RMT ISR gives the semaphore on completion.
         *    A STOP / PARK / DISABLE aborts the channel, gives the
         *    semaphore, and sets motion_active = false.
         */
        esp_err_t ra_wait_err = ESP_OK;
        esp_err_t dec_wait_err = ESP_OK;

        if (ra_batch > 0) {
            ra_wait_err = motors_rmt_wait_ra(pdMS_TO_TICKS(500));
        }
        if (dec_batch > 0) {
            dec_wait_err = motors_rmt_wait_dec(pdMS_TO_TICKS(500));
        }

        if (ra_wait_err != ESP_OK || dec_wait_err != ESP_OK) {
            ESP_LOGW(TAG, "RMT wait %s — position may be off by ≤%u steps",
                     (ra_wait_err == ESP_ERR_TIMEOUT) ? "timeout" : "error",
                     RMT_BATCH_MAX_STEPS);
            motors_rmt_abort_both();
            s_motion.active = false;
            motors_state.status = MOTORS_STATUS_READY;
            motors_state.tracking = TRACKING_NONE;
            break;
        }

        /* 8. External stop preemption — position already updated. */
        if (!s_motion.active) {
            ESP_LOGW(TAG, "Motion aborted — position may be off by ≤%u steps",
                     RMT_BATCH_MAX_STEPS);
            break;
        }

        /*
         * 9. Batch confirmed — mark steps as done.
         */
        ra_steps_done += ra_batch;
        dec_steps_done += dec_batch;

        /*
         * 10. Target reached?  check_motion_conditions at the top of the
         *    loop will catch this, but an early exit here avoids one
         *    unnecessary batch encode.
         */
        if (ra_steps_done >= total_ra_steps && dec_steps_done >= total_dec_steps) {
            motors_state.status = MOTORS_STATUS_READY;
            motors_state.tracking = TRACKING_NONE;
            s_motion.active = false;
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
 *   accumulator += dt / period_us     (period_us converted from RMT ticks)
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
/*
 * Recompute the effective velocity for one axis, folding in any active
 * PulseGuide offset.  Returns the RMT step period in ticks.
 */
static void tracking_loop_rmt(void) {
    const int64_t FINE_MARGIN_US = 2000;
    const float deg_per_step = motors_get_deg_per_microstep();

    /* Signed phase accumulators in microsteps. */
    double ra_phase = 0.0;
    double dec_phase = 0.0;
    int64_t last_time_us = esp_timer_get_time();
    int64_t last_check_us = last_time_us;
    int64_t last_guide_check_us = last_time_us;

    int ra_sign = (motors_state.ra_speed >= 0.0f) ? 1 : -1;
    motors_hw_set_direction_ra((ra_sign > 0)
                                ? MOTOR_DIRECTION_POSITIVE
                                : MOTOR_DIRECTION_NEGATIVE);

    int dec_dir_set = 0;
    rmt_symbol_word_t ra_sym[RMT_BUFFER_SYMBOLS];
    rmt_symbol_word_t dec_sym[RMT_BUFFER_SYMBOLS];

    while (s_motion.active) {
        int64_t now = esp_timer_get_time();
        double dt_s = (double)(now - last_time_us) / 1000000.0;
        last_time_us = now;

        /* Guide expiry (every 10 ms). */
        if (now - last_guide_check_us >= 10000) {
            last_guide_check_us = now;
            if (s_motion.ra_guide_deadline_us
                && now >= s_motion.ra_guide_deadline_us) {
                s_motion.ra_guide_deadline_us = 0;
                s_motion.ra_guide_offset_dps = 0.0f;
            }
            if (s_motion.dec_guide_deadline_us
                && now >= s_motion.dec_guide_deadline_us) {
                s_motion.dec_guide_deadline_us = 0;
                s_motion.dec_guide_offset_dps = 0.0f;
                dec_dir_set = 0;
            }
            motors_state.guiding = (s_motion.ra_guide_deadline_us != 0
                                    || s_motion.dec_guide_deadline_us != 0);
        }

        /* Effective velocities (deg/s). */
        float ra_vel = motors_state.ra_speed;
        if (s_motion.ra_guide_deadline_us)
            ra_vel += s_motion.ra_guide_offset_dps;
        float dec_vel = 0.0f;
        if (s_motion.dec_guide_deadline_us)
            dec_vel = s_motion.dec_guide_offset_dps;

        /* Accumulate signed phase (microsteps). */
        ra_phase += (double)ra_vel * dt_s / (double)deg_per_step;
        dec_phase += (double)dec_vel * dt_s / (double)deg_per_step;

        /* Conditions check (every ~500 us). */
        if (now - last_check_us >= 500) {
            last_check_us = now;
            if (!check_motion_conditions()) break;
            if (motors_state.status != MOTORS_STATUS_TRACKING
                && !motors_state.guiding) {
                break;
            }
        }

        /* ── Emit RA steps ─────────────────────────────────── */
        while (ra_phase >= 1.0) {
            int64_t next = motors_state.ra_steps + (int64_t)ra_sign;
            if (!motors_is_valid_ra_steps(next)) {
                ESP_LOGW(TAG, "RA limit at %.3f deg",
                         (double)motors_steps_to_deg(motors_state.ra_steps));
                s_motion.active = false;
                motors_state.status = MOTORS_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                motors_state.guiding = false;
                return;
            }
            if (!s_motion.active) { ra_phase = 0.0; return; }

            motors_rmt_encode_pulse(ra_sym);
            esp_err_t tx_err = motors_rmt_transmit_ra(ra_sym, 1);
            if (tx_err != ESP_OK) {
                ESP_LOGE(TAG, "RMT RA tx fail: %s", esp_err_to_name(tx_err));
                motors_rmt_abort_ra();
                s_motion.active = false;
                motors_state.status = MOTORS_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                motors_state.guiding = false;
                return;
            }
            esp_err_t wait_err = motors_rmt_wait_ra(pdMS_TO_TICKS(2000));
            if (wait_err != ESP_OK) {
                ESP_LOGW(TAG, "RMT RA wait %s",
                         (wait_err == ESP_ERR_TIMEOUT) ? "timeout" : "error");
                motors_rmt_abort_ra();
                s_motion.active = false;
                motors_state.status = MOTORS_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                motors_state.guiding = false;
                return;
            }
            if (!s_motion.active) { ra_phase = 0.0; return; }

            motors_state.ra_steps = next;
            ra_phase -= 1.0;
        }
        while (ra_phase <= -1.0) {
            int rs = -1;
            int64_t next = motors_state.ra_steps + (int64_t)rs;
            if (!motors_is_valid_ra_steps(next)) {
                ESP_LOGW(TAG, "RA limit at %.3f deg",
                         (double)motors_steps_to_deg(motors_state.ra_steps));
                s_motion.active = false;
                motors_state.status = MOTORS_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                motors_state.guiding = false;
                return;
            }
            if (!s_motion.active) { ra_phase = 0.0; return; }

            motors_rmt_encode_pulse(ra_sym);
            esp_err_t tx_err = motors_rmt_transmit_ra(ra_sym, 1);
            if (tx_err != ESP_OK) {
                ESP_LOGE(TAG, "RMT RA tx fail: %s", esp_err_to_name(tx_err));
                motors_rmt_abort_ra();
                s_motion.active = false;
                motors_state.status = MOTORS_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                motors_state.guiding = false;
                return;
            }
            esp_err_t wait_err = motors_rmt_wait_ra(pdMS_TO_TICKS(2000));
            if (wait_err != ESP_OK) {
                ESP_LOGW(TAG, "RMT RA wait %s",
                         (wait_err == ESP_ERR_TIMEOUT) ? "timeout" : "error");
                motors_rmt_abort_ra();
                s_motion.active = false;
                motors_state.status = MOTORS_STATUS_READY;
                motors_state.tracking = TRACKING_NONE;
                motors_state.guiding = false;
                return;
            }
            if (!s_motion.active) { ra_phase = 0.0; return; }

            motors_state.ra_steps = next;
            ra_phase += 1.0;
        }

        /* ── Emit DEC steps ─────────────────────────────────── */
        while (dec_phase >= 1.0) {
            if (dec_dir_set != 1) {
                motors_hw_set_direction_dec(MOTOR_DIRECTION_POSITIVE);
                dec_dir_set = 1;
            }
            int64_t next = motors_state.dec_steps + 1;
            if (!motors_is_valid_dec_steps(next)) {
                ESP_LOGW(TAG, "DEC limit at %.3f deg",
                         (double)motors_steps_to_deg(motors_state.dec_steps));
                s_motion.dec_guide_deadline_us = 0;
                s_motion.dec_guide_offset_dps = 0.0f;
                dec_dir_set = 0;
                dec_phase = 0.0;
                break;
            }
            if (!s_motion.active) { dec_phase = 0.0; return; }

            motors_rmt_encode_pulse(dec_sym);
            esp_err_t tx_err = motors_rmt_transmit_dec(dec_sym, 1);
            if (tx_err != ESP_OK) {
                ESP_LOGE(TAG, "RMT DEC tx fail: %s", esp_err_to_name(tx_err));
                motors_rmt_abort_dec();
                s_motion.dec_guide_deadline_us = 0;
                s_motion.dec_guide_offset_dps = 0.0f;
                dec_dir_set = 0;
                dec_phase = 0.0;
                break;
            }
            esp_err_t wait_err = motors_rmt_wait_dec(pdMS_TO_TICKS(2000));
            if (wait_err != ESP_OK) {
                ESP_LOGW(TAG, "RMT DEC wait %s",
                         (wait_err == ESP_ERR_TIMEOUT) ? "timeout" : "error");
                motors_rmt_abort_dec();
                s_motion.dec_guide_deadline_us = 0;
                s_motion.dec_guide_offset_dps = 0.0f;
                dec_dir_set = 0;
                dec_phase = 0.0;
                break;
            }
            if (!s_motion.active) { dec_phase = 0.0; return; }

            motors_state.dec_steps = next;
            dec_phase -= 1.0;
        }
        while (dec_phase <= -1.0) {
            if (dec_dir_set != -1) {
                motors_hw_set_direction_dec(MOTOR_DIRECTION_NEGATIVE);
                dec_dir_set = -1;
            }
            int64_t next = motors_state.dec_steps - 1;
            if (!motors_is_valid_dec_steps(next)) {
                ESP_LOGW(TAG, "DEC limit at %.3f deg",
                         (double)motors_steps_to_deg(motors_state.dec_steps));
                s_motion.dec_guide_deadline_us = 0;
                s_motion.dec_guide_offset_dps = 0.0f;
                dec_dir_set = 0;
                dec_phase = 0.0;
                break;
            }
            if (!s_motion.active) { dec_phase = 0.0; return; }

            motors_rmt_encode_pulse(dec_sym);
            esp_err_t tx_err = motors_rmt_transmit_dec(dec_sym, 1);
            if (tx_err != ESP_OK) {
                ESP_LOGE(TAG, "RMT DEC tx fail: %s", esp_err_to_name(tx_err));
                motors_rmt_abort_dec();
                s_motion.dec_guide_deadline_us = 0;
                s_motion.dec_guide_offset_dps = 0.0f;
                dec_dir_set = 0;
                dec_phase = 0.0;
                break;
            }
            esp_err_t wait_err = motors_rmt_wait_dec(pdMS_TO_TICKS(2000));
            if (wait_err != ESP_OK) {
                ESP_LOGW(TAG, "RMT DEC wait %s",
                         (wait_err == ESP_ERR_TIMEOUT) ? "timeout" : "error");
                motors_rmt_abort_dec();
                s_motion.dec_guide_deadline_us = 0;
                s_motion.dec_guide_offset_dps = 0.0f;
                dec_dir_set = 0;
                dec_phase = 0.0;
                break;
            }
            if (!s_motion.active) { dec_phase = 0.0; return; }

            motors_state.dec_steps = next;
            dec_phase += 1.0;
        }

        /* ── Sleep ─────────────────────────────────────────── */
        double ra_s = (ra_vel != 0.0) ? (1.0 - ra_phase) * deg_per_step / fabs((double)ra_vel) : 1e9;
        double dec_s = (dec_vel != 0.0) ? (1.0 - dec_phase) * deg_per_step / fabs((double)dec_vel) : 1e9;
        int64_t w_us = (int64_t)((ra_s < dec_s ? ra_s : dec_s) * 1000000.0);
        if (w_us < 0) w_us = 0;

        if (w_us > FINE_MARGIN_US) {
            uint32_t sm = (uint32_t)((w_us - FINE_MARGIN_US) / 1000);
            if (sm > 50) sm = 50;
            if (sm < 1) sm = 1;
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(sm));
            continue;
        }
        if (w_us > 0) {
            int64_t dl = esp_timer_get_time() + w_us;
            while (esp_timer_get_time() < dl) {
                if ((esp_timer_get_time() & 0x1FF) == 0) taskYIELD();
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
    /* Motor ERROR — refuse to enter any motion loop. */
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        return;
    }

    /* Standalone guide pulse (no tracking) — use the tracking loop
     * with zero base rate.  It handles DEC-only and RA-only guiding. */
    if ((motors_state.status == MOTORS_STATUS_TRACKING
         && motors_state.tracking != TRACKING_NONE)
        || motors_state.guiding) {
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
 * Stop / park / enable are handled directly by their callers via
 * motors_motion_stop() + motors_state update.
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
        /* PULSE_GUIDE: the tracking loop picks up guide state on each
         * iteration.  If tracking is NOT active, the guide pulse runs
         * via a minimal single-step loop below. */
        if (cmd.type == MOTION_CMD_PULSE_GUIDE
            && motors_state.status != MOTORS_STATUS_TRACKING) {
            /* Standalone guiding — run a short motion loop just for
             * the guide pulse.  DEC-only: no tracking base rate. */
            motion_loop();
        }

        /* Clear guiding flag when both axes are idle. */
        bool ra_active = (s_motion.ra_guide_deadline_us != 0)
                      && (esp_timer_get_time() < s_motion.ra_guide_deadline_us);
        bool dec_active = (s_motion.dec_guide_deadline_us != 0)
                       && (esp_timer_get_time() < s_motion.dec_guide_deadline_us);
        motors_state.guiding = (ra_active || dec_active);
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
        &motors_motion_task_handle);

    /* Report stack high-water mark for diagnostics. */
    if (motors_motion_task_handle != NULL) {
        UBaseType_t high_water = uxTaskGetStackHighWaterMark(motors_motion_task_handle);
        ESP_LOGI(TAG, "Stack high-water mark: %lu words (total %d)",
                 (unsigned long) high_water, MOTION_TASK_STACK_WORDS);
    }
}
