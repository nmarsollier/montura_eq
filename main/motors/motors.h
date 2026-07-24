#pragma once

#include <stdint.h>
#include "esp_err.h"

/*
 * High-level state of the motors subsystem used as the authoritative
 * source of truth for mount activity.
 */
typedef enum {
    /* Ready to accept slews/tracking requests. */
    MOTORS_STATUS_READY,
    /* Performing a user-initiated slew to coordinates. */
    MOTORS_STATUS_SLEWING,
    /* Continuous tracking is active. */
    MOTORS_STATUS_TRACKING,
    /* Parked: mount in safe parked position. */
    MOTORS_STATUS_PARKED,
    /* Disabled: motors unavailable or disabled by user. */
    MOTORS_STATUS_DISABLED
} MotorsStatus;

/*
 * High-level tracking profiles the motors module can apply.
 */
typedef enum TrackingMode {
    /* No automatic tracking. */
    TRACKING_NONE,
    /* Sidereal tracking (stars). */
    TRACKING_SIDEREAL,
    /* Lunar tracking (moon). */
    TRACKING_LUNAR,
    /* Solar tracking (sun). */
    TRACKING_SOLAR
} TrackingMode;

/*
 * Authoritative snapshot of the motors module's view of the mount.
 *
 * Position fields are absolute microstep counters (int64_t) for zero
 * accumulation error.  Use motors_get_ra_deg() / motors_get_dec_deg()
 * to read the current position in degrees.
 */
typedef struct {
    /* Current RA axis position — absolute microstep counter. */
    int64_t ra_steps;
    /* Current DEC axis position — absolute microstep counter. */
    int64_t dec_steps;
    /* High-level motors-derived status. */
    MotorsStatus status;
    /* Current requested tracking mode. */
    TrackingMode tracking;
    /* Current commanded/measured RA axis angular speed (deg/s). */
    float ra_speed;
    /* Current commanded/measured DEC axis angular speed (deg/s). */
    float dec_speed;
    /* True when a PulseGuide is active on either axis. */
    bool guiding;

    /* Operational limits enforced by the motors module. */
    struct {
        float ra_min;
        float ra_max;
        float dec_min;
        float dec_max;
    } limits;
} MotorsState;

/* Numeric result codes returned by motors functions. Mount maps these to MountResult objects. */
typedef enum {
    MOTOR_OK = 0,
    MOTOR_ERR_INVALID_AXIS = 1,
    MOTOR_ERR_OUT_OF_RANGE = 2,
    MOTOR_ERR_NOT_READY = 3,
    MOTOR_ERR_INTERNAL = 99
} MotorResultCode;

/*
 * Initialize the motors subsystem.  Returns ESP_OK on success,
 * otherwise the subsystem is left in DISABLED state.
 */
esp_err_t motors_init(void);

/*
 * Enable motor drivers.
 */
void motors_enable(void);

/*
 * Disable motor drivers.
 */
void motors_disable(void);

/*
 * Return a snapshot copy of the current `MotorsState`.
 */
MotorsState motors_current_state(void);

/*
 * Current axis positions in degrees — derived view over the int64_t
 * microstep counters.  No accumulation error.
 */
float motors_get_ra_deg(void);

float motors_get_dec_deg(void);

/*
 * Stop both axes and return to READY.
 */
void motors_stop(void);

/*
 * Park both axes: stop motion and set status to PARKED.
 */
void motors_park(void);

/*
 * Move the mount to the home position (0, 0).
 */
void motors_home(float lat);

/*
 * Set the current physical position as the new zero reference.
 * Both axis step counters are reset to 0.  Allowed only in READY state.
 */
MotorResultCode motors_set_zero(void);

/*
 * Start continuous tracking according to the chosen `TrackingMode`.
 */
MotorResultCode motors_start_tracking(TrackingMode mode);

/*
 * Move one or both axes continuously at the given rates in deg/s.
 * Positive = forward, negative = reverse, zero = stop that axis.
 * Both zero is equivalent to STOP.  Used by Alpaca MoveAxis and
 * manual controls (joystick).
 */
void motors_set_move_axis_speed(float ra_speed, float dec_speed);

/*
 * Return the slewing angular speed (deg/s) for a given speed_rate profile.
 */
float motors_get_slewing_speed(int speed_rate);

/*
 * Return a comma-separated list of valid axis names ("RA, DEC").
 */
const char *motors_axis_valid_values(void);

/*
 * Canonical status and tracking name helpers.
 */
const char *motors_status_to_string(MotorsStatus status);

const char *motors_tracking_to_string(TrackingMode tracking);

TrackingMode motors_tracking_from_string(const char *value);

const char *motors_tracking_valid_values(void);

/* Move both axes to absolute angles in degrees. */
MotorResultCode motors_slew_to_angle(float ra_deg, float dec_deg, int speed_rate, float lat);

MotorResultCode motors_slew_axis_ra(float degrees, int speed_rate, float lat);

MotorResultCode motors_slew_axis_dec(float degrees, int speed_rate, float lat);

/*
 * Enqueue a PulseGuide command.  The motion task dequeues and executes
 * it — no direct state writes from the caller.  Works whether tracking
 * is active or not.
 *
 * axis: 0 = RA, 1 = DEC
 * offset_dps: signed angular speed offset in deg/s
 * duration_ms: guide pulse duration in milliseconds
 */
void motors_pulse_guide_start(int axis, float offset_dps, uint32_t duration_ms);

/*
 * Return true if a PulseGuide is active on either axis.
 * Reads motors_state.guiding — written exclusively by the motion task.
 */
bool motors_is_pulse_guiding(void);
