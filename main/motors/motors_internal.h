#pragma once

#include "motors.h"

#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* =========================================================================
 * Motion command queue — thread-safe communication with the motion task.
 *
 * External callers (REST handlers, button poller) send MotionCommand
 * structs to the queue.  The motors task is the sole consumer and the
 * sole writer of motors_state position fields.
 *
 * Only motion-producing commands go through the queue.  Stop / park /
 * disable / enable are handled directly by their callers via
 * motors_motion_stop() + motors_state update — no queue round-trip.
 * ========================================================================= */

typedef enum {
    MOTION_CMD_SLEW = 0,
    MOTION_CMD_TRACK,
    MOTION_CMD_MOVE_AXIS,
    MOTION_CMD_PULSE_GUIDE,
} MotionCommandType;

typedef struct {
    MotionCommandType type;
    float ra_target_deg;
    float dec_target_deg;
    float ra_speed;
    float dec_speed;
    TrackingMode tracking_mode;
    bool relative;
    float ra_delta_deg;
    float dec_delta_deg;
    /* PulseGuide fields (valid when type == MOTION_CMD_PULSE_GUIDE) */
    int guide_axis;        /* 0 = RA, 1 = DEC */
    float guide_offset_dps; /* signed deg/s */
    uint32_t guide_duration_ms;
} MotionCommand;

/* Queue handle — created by motors_init(), shared across the module. */
extern QueueHandle_t motion_cmd_queue;

/* Send a MotionCommand to the back of the queue (FIFO). */
void motors_queue_put(MotionCommand *cmd);

/* Atomically discard every command in the queue. */
void motors_queue_clear(void);

/* Validate axis values against the configured inclusive limits. */
bool motors_is_valid_ra(float value);

bool motors_is_valid_dec(float value);

bool motors_is_valid_ra_steps(int64_t steps);

bool motors_is_valid_dec_steps(int64_t steps);

/* =========================================================================
 * Mechanical constants — hardware configuration.
 * ========================================================================= */
#define MOTOR_STEP_ANGLE_DEG     (1.8f)
#define MOTOR_FULL_STEPS_PER_REV ((int)(360.0f / MOTOR_STEP_ANGLE_DEG))
#define MOTOR_PULLEY_TEETH       (20)
#define AXIS_PULLEY_TEETH        (80)

/*
 * Motion calibration factor.
 *
 * Compensates for discrepancies between configured and actual step
 * resolution.  Adjust until commanded angle equals physical movement:
 *   - Mount moves too little → increase the factor
 *   - Mount moves too much   → decrease the factor
 *
 * factor = commanded_angle / actual_angle
 */
#define MOTION_CALIBRATION_FACTOR 1.0f

/* =========================================================================
 * GPIO pin assignments — single source of truth for the motors module.
 *
 * STEP pins are owned by the RMT peripheral (motors_rmt.c).
 * DIR and ENABLE pins remain under GPIO control (motors_hw.c).
 * ========================================================================= */
#define MOTORS_ENABLE_GPIO GPIO_NUM_14
#define RA_STEP_GPIO       GPIO_NUM_10
#define DEC_STEP_GPIO      GPIO_NUM_15
#define RA_DIR_GPIO        GPIO_NUM_9
#define DEC_DIR_GPIO       GPIO_NUM_7

/* TMC2209 EN pin is active-low. */
#define MOTORS_ENABLE_ACTIVE_LEVEL   0
#define MOTORS_ENABLE_INACTIVE_LEVEL 1

/* =========================================================================
 * Microstep and step resolution — sourced from TMC hardware.
 *
 * The TMC2209 driver is the SINGLE source of truth for microstep count.
 * Use the inline helper below to always read the active value from the
 * TMC module's verified cache (no UART transaction needed on read).
 *
 * Fallback: if TMC not yet initialized, returns TMC_TARGET_MICROSTEPS (128).
 * ========================================================================= */

/*
 * Get the active microstep count from the TMC2209 driver.
 * Reads the cached value verified against hardware registers during init.
 * Falls back to the compile-time default if the TMC is not yet initialised.
 */
static inline uint16_t motors_get_microsteps(void) {
    extern uint16_t tmc2209_get_active_microsteps(void);
    uint16_t ms = tmc2209_get_active_microsteps();
    return (ms > 0) ? ms : 128;
}

/*
 * Angular displacement per microstep at the mount axis.
 * Computed at runtime from the TMC-verified microstep count,
 * gear ratio, and calibration factor — no hardcoded step size.
 */
static inline float motors_get_deg_per_microstep(void) {
    return 360.0f / ((float) MOTOR_FULL_STEPS_PER_REV *
                     (float) motors_get_microsteps() *
                     ((float) AXIS_PULLEY_TEETH / (float) MOTOR_PULLEY_TEETH) *
                     MOTION_CALIBRATION_FACTOR);
}

/* =========================================================================
 * Hardware layer — DIR/ENABLE GPIO control (motors_hw.c).
 *
 * STEP pulse generation is handled by the RMT peripheral (motors_rmt.c).
 * ========================================================================= */

typedef enum {
    MOTOR_DIRECTION_NEGATIVE = 0,
    MOTOR_DIRECTION_POSITIVE = 1,
} MotorDirection;

esp_err_t motors_hw_init(void);

void motors_hw_enable(void);

void motors_hw_disable(void);

void motors_hw_set_direction_ra(MotorDirection direction);

void motors_hw_set_direction_dec(MotorDirection direction);

/* =========================================================================
 * RMT+DMA step pulse generation (motors_rmt.c).
 *
 * Replaces software GPIO bit-banging with hardware-timed RMT symbols
 * streamed via GDMA. Zero jitter, near-zero CPU.
 * ========================================================================= */

/* RMT clock resolution — 2 MHz (0.5 us per tick).
 * Balanced for high-reduction configurations while keeping slow-step
 * idle symbols within SOC_RMT_MEM_WORDS_PER_CHANNEL (48). */
#define RMT_RESOLUTION_HZ 2000000U

/* STEP pulse timing in RMT ticks (2 MHz reference).
 *
 * TMC2209 requires STEP HIGH ≥ 100 ns and STEP LOW ≥ 100 ns.
 * We use 2 µs HIGH (4 ticks) and ≥ 1 µs LOW (2 ticks) for margin.
 * The minimum total period guarantees a valid LOW gap between pulses. */
#define STEP_PULSE_TICKS     4U    /* 2 us HIGH */
#define STEP_MIN_LOW_TICKS   2U    /* 1 us LOW floor */
#define STEP_MIN_PERIOD_TICKS (STEP_PULSE_TICKS + STEP_MIN_LOW_TICKS)  /* 6 ticks = 3 us */

/* =========================================================================
 * Position representation — int64_t absolute microstep counters.
 *
 * Degrees are a derived view over the step counter, computed on demand
 * for API consumers.  All internal position tracking uses integer steps
 * for zero-accumulation-error precision over arbitrarily long sessions.
 * ========================================================================= */

/* Convert between steps and degrees using the active microstep resolution. */
static inline float motors_steps_to_deg(int64_t steps) {
    return (float)steps * motors_get_deg_per_microstep();
}

static inline int64_t motors_deg_to_steps(float degrees) {
    float deg_per_step = motors_get_deg_per_microstep();
    return (int64_t)(degrees / deg_per_step + (degrees >= 0.0f ? 0.5f : -0.5f));
}

esp_err_t motors_rmt_init(void);

esp_err_t motors_rmt_deinit(void);

uint32_t motors_rmt_encode_steps(rmt_symbol_word_t *symbols,
                                  uint32_t max_symbols,
                                  uint32_t step_period_ticks,
                                  uint32_t step_count);

/*
 * Encode a bare STEP pulse (HIGH + minimal LOW).
 * For use by the tracking/guiding loop where inter-step timing
 * is handled by the accumulator + deadline, not by RMT symbols.
 * Always consumes 1 symbol.
 */
uint32_t motors_rmt_encode_pulse(rmt_symbol_word_t *symbols);

esp_err_t motors_rmt_transmit_ra(const rmt_symbol_word_t *symbols,
                                  uint32_t num_symbols);

esp_err_t motors_rmt_transmit_dec(const rmt_symbol_word_t *symbols,
                                   uint32_t num_symbols);

esp_err_t motors_rmt_wait_ra(TickType_t timeout_ticks);

esp_err_t motors_rmt_wait_dec(TickType_t timeout_ticks);

void motors_rmt_abort_ra(void);

void motors_rmt_abort_dec(void);

void motors_rmt_abort_both(void);

/* =========================================================================
 * Module-global state — motors_state is the single source of truth for
 * the motors layer.  External code reads it through motors_current_state().
 * ========================================================================= */
extern MotorsState motors_state;

float motors_get_tracking_speed(TrackingMode mode);

/* Motion task handle — exposed so external code can send notifications. */
extern TaskHandle_t motors_motion_task_handle;

/*
 * Motion-active flag — set false by external code (stop, park) to
 * signal the motion task to abort.  The motion task reads and clears
 * this, and handles RMT abort internally.
 */
extern bool motors_motion_active;

/* RMT abort — called ONLY by the motion task. */
void motors_rmt_abort_ra(void);
void motors_rmt_abort_dec(void);
void motors_rmt_abort_both(void);

/* =========================================================================
 * Task & queue lifecycle (motors_task.c, motors_queue.c).
 * ========================================================================= */

/* Stop the active motion loop from outside the motion task.
 * Safe to call from any task — the loop exits at its next iteration. */
void motors_motion_stop(void);

void motors_motion_task_init(void);

void motors_queue_init(void);
