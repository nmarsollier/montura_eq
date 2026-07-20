/* Mount - mount_tracking.c
 *
 * Purpose: change the active tracking mode.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

/*
 * Business use case: change the mount's active tracking mode.
 *
 * Objective: let clients choose how the mount compensates for the sky's
 * apparent motion during observations while respecting state rules.
 */
MountResult mount_set_tracking(TrackingMode tracking) {
    MotorResultCode rc = motors_start_tracking(tracking);
    if (rc != MOTOR_OK) {
        switch (rc) {
            case MOTOR_ERR_NOT_READY:
                return mount_result_error("Motors not ready");
            default:
                return mount_result_error("Motor error");
        }
    }

    return mount_result_ok();
}
