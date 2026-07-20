/* Mount - mount_get_lst.c
 *
 * Purpose: compute the current Local Sidereal Time for the configured site.
 */
#include "mount.h"
#include "mount_internal.h"

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

float mount_get_lst(void) {
    time_t now = time(NULL);
    double jd = jd_from_unix(now);
    double gmst_h = gmst_hours(jd);
    double lst_h = gmst_h + (mount_internal_state.lon / 15.0);

    /* Normalize to [0, 24). */
    while (lst_h < 0.0)  lst_h += 24.0;
    while (lst_h >= 24.0) lst_h -= 24.0;

    return (float) lst_h;
}
