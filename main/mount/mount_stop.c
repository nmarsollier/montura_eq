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
    mount_move_axis_reset();
    motors_stop();

    return mount_result_ok();
}
