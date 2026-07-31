/* Mount - mount_limits.c
 *
 * Purpose: bridge the REST API to the motors limits subsystem.
 *
 * Validates mount state (rejects if motors are in ERROR) and maps
 * action strings to the corresponding motors_limits setters.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors/motors.h"

#include <string.h>

MountResult mount_limits_set(const char *action) {
    if (mount_is_motors_error()) {
        return mount_result_motors_error();
    }

    if (action == NULL) {
        return mount_result_error("Missing action");
    }

    if (strcmp(action, "set_home") == 0) {
        motors_set_current_as_home();
    } else if (strcmp(action, "set_ra_left") == 0) {
        motors_set_current_as_ra_left_limit();
    } else if (strcmp(action, "set_ra_right") == 0) {
        motors_set_current_as_ra_right_limit();
    } else if (strcmp(action, "set_dec_left") == 0) {
        motors_set_current_as_dec_left_limit();
    } else if (strcmp(action, "set_dec_right") == 0) {
        motors_set_current_as_dec_right_limit();
    } else if (strcmp(action, "clear_limits") == 0) {
        motors_limits_reset();
    } else {
        return mount_result_error("Unknown action");
    }

    return mount_result_ok();
}
