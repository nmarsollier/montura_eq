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

esp_err_t motors_rmt_init(void);

esp_err_t motors_rmt_deinit(void);

uint32_t motors_rmt_encode_steps(rmt_symbol_word_t *symbols,
                                  uint32_t max_symbols,
                                  uint32_t step_period_us,
                                  uint32_t step_count);

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

/* =========================================================================
 * Task & queue lifecycle (motors_task.c, motors_queue.c).
 * ========================================================================= */

/* Stop the active motion loop from outside the motion task.
 * Safe to call from any task — the loop exits at its next iteration. */
void motors_motion_stop(void);

void motors_motion_task_init(void);

void motors_queue_init(void);
