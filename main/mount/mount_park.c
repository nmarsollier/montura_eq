/* Mount - mount_park.c
 *
 * Purpose: park the mount safely.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

/*
 * Business use case: park the mount.
 *
 * Objective: leave the equipment in a safe rest state with tracking disabled.
 */
MountResult mount_park(void) {
    if (mount_is_motors_error()) {
        return mount_result_motors_error();
    }

    if (motors_current_state().status == MOTORS_STATUS_PARKED) {
        return mount_result_ok();
    }

    MotorResultCode rc = motors_park();
    return motors_result_code_error_result(rc);
}
