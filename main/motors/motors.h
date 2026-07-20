#pragma once

#include <stdint.h>

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

/* NOTE: `MountStatus` alias removed - use `MotorsStatus` throughout the codebase. */

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
 */
typedef struct {
    /* Current RA axis angle in degrees. */
    float ra_position;
    /* Current DEC axis angle in degrees. */
    float dec_position;
    /* High-level motors-derived status. */
    MotorsStatus status;
    /* Current requested tracking mode. */
    TrackingMode tracking;
    /* Current commanded/measured RA axis angular speed (deg/s). */
    float ra_speed;
    /* Current commanded/measured DEC axis angular speed (deg/s). */
    float dec_speed;

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
 * Initialize the motors subsystem.
 */
void motors_init(void);

/*
 * Enable or disable motor drivers.
 */
void motors_enable(void);

void motors_disable(void);

/*
 * Return a snapshot copy of the current `MotorsState`.
 */
MotorsState motors_current_state(void);

/*
 * Stop both axes and return to READY.
 */
void motors_stop(void);

void motors_park(void);

/* Convenience high-level home action. */
void motors_home(float lat);

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

const char *motors_axis_valid_values(void);

/* Canonical status and tracking name helpers — implemented in motors_tools.c. */
const char *status_to_string(MotorsStatus status);

const char *tracking_to_string(TrackingMode tracking);

TrackingMode tracking_from_string(const char *value);

const char *tracking_valid_values(void);

/* Move both axes to absolute angles in degrees. */
MotorResultCode motors_slew_to_angle(float ra_deg, float dec_deg, int speed_rate, float lat);

MotorResultCode motors_slew_axis_ra(float degrees, int speed_rate, float lat);

MotorResultCode motors_slew_axis_dec(float degrees, int speed_rate, float lat);
