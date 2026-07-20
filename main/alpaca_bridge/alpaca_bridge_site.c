/* Alpaca bridge — site location
 *
 * Read from and write to the persisted mount settings (NVS).
 * The settings struct is copied, modified, and re-persisted on each
 * write to keep NVS consistent.
 */

#include "alpaca_bridge.h"

#include "mount.h"
#include "mount_internal.h"

float alpaca_bridge_get_site_latitude(void) {
    return mount_internal_state.lat;
}

float alpaca_bridge_get_site_longitude(void) {
    return mount_internal_state.lon;
}

int alpaca_bridge_get_site_elevation(void) {
    return mount_internal_state.elevation;
}

MountResult alpaca_bridge_set_site_latitude(float lat) {
    MountSettings s = mount_internal_state;
    s.lat = lat;
    return mount_settings_update(s);
}

MountResult alpaca_bridge_set_site_longitude(float lon) {
    MountSettings s = mount_internal_state;
    s.lon = lon;
    return mount_settings_update(s);
}

MountResult alpaca_bridge_set_site_elevation(int elevation) {
    MountSettings s = mount_internal_state;
    s.elevation = elevation;
    return mount_settings_update(s);
}
