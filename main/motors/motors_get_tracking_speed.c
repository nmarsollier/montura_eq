/* Motors - motors_get_tracking_speed.c
 *
 * Purpose: return the RA axis angular speed (deg/s) for a tracking mode.
 * Speeds are defined in motors.h as TRACKING_SPEED_*_DPS.
 */
#include "motors.h"

float motors_get_tracking_speed(TrackingMode mode) {
    switch (mode) {
        case TRACKING_SIDEREAL: return TRACKING_SPEED_SIDEREAL_DPS;
        case TRACKING_SOLAR:    return TRACKING_SPEED_SOLAR_DPS;
        case TRACKING_LUNAR:    return TRACKING_SPEED_LUNAR_DPS;
        default:                return 0.0f;
    }
}
