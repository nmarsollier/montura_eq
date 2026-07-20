/* Alpaca bridge — destination pier side
 *
 * Computes the pier side that would result from a slew to the given
 * equatorial coordinates at the current time and site.
 *
 * ASCOM convention for a German equatorial mount:
 *   HA >= 0  (west of meridian) →  pierEast  (0) — scope east of pier
 *   HA <  0  (east of meridian) →  pierWest  (1) — scope west of pier
 *
 * This is determined purely by the target's hour angle, not by the
 * specific mechanical solution chosen by equatorial_to_axis().
 */

#include "alpaca_bridge.h"
#include "alpaca_bridge_internal.h"

#include "mount.h"
#include <math.h>
#include <time.h>

static double jd_from_unix(time_t t) {
    return (double) t / 86400.0 + 2440587.5;
}

static double gmst_hours(double jd) {
    double T = (jd - 2451545.0) / 36525.0;
    double gmst_deg = 280.46061837 + 360.98564736629 * (jd - 2451545.0)
                      + 0.000387933 * T * T - (T * T * T) / 38710000.0;
    gmst_deg = fmod(gmst_deg, 360.0);
    if (gmst_deg < 0.0) gmst_deg += 360.0;
    return gmst_deg / 15.0;
}

int alpaca_bridge_get_destination_side_of_pier(float ra_hours, float dec_deg) {
    (void) dec_deg; /* pier side is independent of DEC for a GEM */

    time_t now = time(NULL);
    double jd = jd_from_unix(now);
    double gmst = gmst_hours(jd);
    double lst = gmst + (alpaca_bridge_get_site_longitude() / 15.0);

    /* Hour angle = LST − RA.  Wrap to [−12h, +12h]. */
    float ha_h = (float)(lst - (double) ra_hours);
    while (ha_h > 12.0f)  ha_h -= 24.0f;
    while (ha_h < -12.0f) ha_h += 24.0f;

    /* HA >= 0 → target west of meridian → pierEast.
     * HA <  0 → target east of meridian → pierWest. */
    return (ha_h >= 0.0f) ? 0   /* pierEast */
                          : 1;  /* pierWest */
}
