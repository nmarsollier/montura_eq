/* Mount - mount_stop.c
 *
 * Purpose: stop any active mount movement.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"
#include "rest_alpaca_internal.h"

/*
 * Business use case: stop an active movement operation.
 *
 * Objective: leave the mount ready for the next command after a STOP request.
 */
MountResult mount_stop(void) {
    alpaca_moveaxis_reset();
    motors_stop();

    return mount_result_ok();
}
