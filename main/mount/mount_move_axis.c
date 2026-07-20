/* Mount - mount_move_axis.c
 *
 * Purpose: move one axis by a relative amount.
 */
#include "mount.h"
#include "mount_internal.h"
#include "motors.h"

/*
 * Business use case: move a mount axis by a relative amount of degrees.
 *
 * Objective: let operators adjust a single axis without knowing the absolute
 * current position.
 */
MountResult mount_move_axis_ra(float degrees, int speed_rate) {
    if (degrees == 0.0f) {
        return mount_result_error("Degrees cannot be zero");
    }

    MotorResultCode rc = motors_slew_axis_ra(degrees, speed_rate, mount_internal_state.lat);

    if (rc != MOTOR_OK) {
        return motors_result_code_error_result(rc);
    }

    return mount_result_ok();
}

/*
 * Business use case: move a mount axis by a relative amount of degrees.
 *
 * Objective: let operators adjust a single axis without knowing the absolute
 * current position.
 */
MountResult mount_move_axis_dec(float degrees, int speed_rate) {
    if (degrees == 0.0f) {
        return mount_result_error("Degrees cannot be zero");
    }

    MotorResultCode rc = motors_slew_axis_dec(degrees, speed_rate, mount_internal_state.lat);

    if (rc != MOTOR_OK) {
        return motors_result_code_error_result(rc);
    }

    return mount_result_ok();
}
