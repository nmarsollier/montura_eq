/* Mount - mount_tools.c
 *
 * Purpose: mount result helpers and command validation support.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

/*
 * Current RA in decimal hours, read directly from the authoritative
 * motor position (axis degrees).  RA hour = axis_deg / 15.
 */
float mount_get_ra_hours(void) {
    MotorsState s = motors_current_state();
    AxisCoordinates axis = {.ra_axis_deg = s.ra_position, .dec_axis_deg = s.dec_position};
    return axis_to_equatorial(axis).ra_hours;
}

float mount_get_dec_deg(void) {
    MotorsState s = motors_current_state();
    AxisCoordinates axis = {.ra_axis_deg = s.ra_position, .dec_axis_deg = s.dec_position};
    return axis_to_equatorial(axis).dec_deg;
}

MountResult mount_result_error(const char *message) {
    MountResult result = {
        .ok = false,
        .message = message
    };

    return result;
}

/*
 * Business use case: normalize mount command responses.
 *
 * Objective: provide a uniform success/error contract for REST and UI
 * clients.
 */
MountResult mount_result_ok() {
    return (MountResult){.ok = true, .message = "OK"};
}

MountResult motors_result_code_error_result(MotorResultCode rc) {
    switch (rc) {
        case MOTOR_ERR_INVALID_AXIS:
            return mount_result_error("Invalid axis");
        case MOTOR_ERR_OUT_OF_RANGE:
            return mount_result_error("Target out of range");
        case MOTOR_ERR_NOT_READY:
            return mount_result_error("Motors not ready");
        case MOTOR_ERR_INTERNAL:
        default:
            return mount_result_error("Motor error");
    }
}
