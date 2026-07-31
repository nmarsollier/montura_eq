/* Mount - mount_stop.c
 *
 * Purpose: stop any active mount movement.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

/*
 * Business use case: stop an active movement operation.
 *
 * Objective: leave the mount ready for the next command after a STOP request.
 */
MountResult mount_stop(void) {
    if (mount_is_motors_error()) {
        return mount_result_motors_error();
    }

    mount_move_axis_reset();
    MotorResultCode rc = motors_stop();
    if (rc != MOTOR_OK) {
        return motors_result_code_error_result(rc);
    }

    return mount_result_ok();
}
