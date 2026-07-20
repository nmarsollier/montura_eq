/* Motors - motors_get_tracking_speed.c
 *
 * Purpose: return the RA axis angular speed (deg/s) for a tracking mode.
 *
 * Precomputed from:
 *   SIDEREAL  = 360° / 86164.0905308 s  (sidereal day)
 *   SOLAR     = SIDEREAL − 360° / (365.256363004 × 86400)  (solar day minus annual motion)
 *   LUNAR     = SIDEREAL − 360° / (27.321661   × 86400)  (sidereal month)
 */
#include "motors.h"

float motors_get_tracking_speed(TrackingMode mode) {
    switch (mode) {
        case TRACKING_SIDEREAL: return 0.004178074f;
        case TRACKING_SOLAR: return 0.004166667f;
        case TRACKING_LUNAR: return 0.004025576f;
        default: return 0.0f;
    }
}
