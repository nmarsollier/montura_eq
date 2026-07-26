/* Mount - mount_home.c
 *
 * Purpose: send the mount to its home position.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

MountResult mount_home(void) {
    motors_home();
    return mount_result_ok();
}
