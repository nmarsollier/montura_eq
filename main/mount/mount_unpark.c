/* Mount - mount_unpark.c
 *
 * Purpose: return the mount from parked state to normal operation.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

/*
 * Business use case: take the mount out of the parked state.
 *
 * Objective: reopen normal operation so pointing, tracking, and sync
 * commands can be accepted again.
 */
MountResult mount_unpark(void) {
    motors_enable();

    return mount_result_ok();
}
