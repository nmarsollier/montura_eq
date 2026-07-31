/* Mount - mount_set_zero.c
 *
 * Purpose: reset the axis position counters so the current physical
 * orientation becomes the new zero / home reference.
 *
 * Useful for worm-gear mounts where the user manually positions the
 * mount to a known orientation and wants the software to treat that
 * point as the origin.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

MountResult mount_set_zero(void) {
    if (mount_is_motors_error()) {
        return mount_result_motors_error();
    }

    MotorResultCode rc = motors_set_zero();
    return motors_result_code_error_result(rc);
}
