/* Alpaca bridge — sidereal time
 *
 * Compute Local Apparent Sidereal Time at the configured site.
 */

#include "alpaca_bridge.h"
#include "alpaca_bridge_internal.h"

#include <time.h>

#include "mount.h"
#include "mount_internal.h"

/*
 * Local Apparent Sidereal Time at the configured site (hours, 0–24).
 * Computed as GMST corrected by site longitude.
 */
float alpaca_bridge_get_sidereal_time(void) {
    time_t now = time(NULL);
    double jd = alpaca_bridge_unix_to_jd(now);
    double gmst_h = alpaca_bridge_gmst_hours(jd);
    double lst_h = gmst_h + (double) mount_internal_state.lon / 15.0;
    while (lst_h < 0.0) lst_h += 24.0;
    while (lst_h >= 24.0) lst_h -= 24.0;
    return (float) lst_h;
}
