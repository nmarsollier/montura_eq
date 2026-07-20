/* Mount - mount_settings.c
 *
 * Purpose: validate and apply mount settings.
 */
#include "mount.h"
#include "mount_internal.h"

/* mount_set_system_time is implemented in mount_time.c. */

MountResult mount_settings_update(MountSettings settings) {
    if (settings.lat < -90.0f || settings.lat > 90.0f) {
        return mount_result_error("Latitude is outside valid range -90 <= lat <= 90");
    }

    if (settings.lon < -180.0f || settings.lon > 180.0f) {
        return mount_result_error("Longitude is outside valid range -180 <= lon <= 180");
    }

    mount_internal_state = settings;

    /* Persist the updated settings. */
    mount_settings_save(&mount_internal_state);

    return mount_result_ok();
}
