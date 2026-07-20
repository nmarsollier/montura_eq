/* Alpaca bridge — tracking rate
 *
 * Maps between the mount's internal TrackingMode enum and the
 * ASCOM DriveRates integer values (0 = sidereal, 1 = lunar,
 * 2 = solar, 3 = king — not implemented).
 */

#include "alpaca_bridge.h"

#include <stdio.h>

#include "mount.h"
#include "motors/motors.h"

/*
 * Return the current tracking mode as an ASCOM DriveRates value.
 */
int alpaca_bridge_get_tracking_rate(void) {
    MotorsState s = motors_current_state();
    switch (s.tracking) {
        case TRACKING_SIDEREAL: return 0;
        case TRACKING_LUNAR: return 1;
        case TRACKING_SOLAR: return 2;
        default: return 0;
    }
}

/*
 * True if any automatic tracking mode is active (sidereal, lunar, or solar).
 * Manual and None are considered "not tracking".
 */
bool alpaca_bridge_get_tracking(void) {
    MotorsState s = motors_current_state();
    return s.tracking != TRACKING_NONE;
}

/*
 * Enable or disable tracking without changing the current rate.
 * When enabling, the previously-set rate (via TrackingRate) is preserved.
 * Defaults to sidereal only if no rate was ever set.
 */
MountResult alpaca_bridge_set_tracking(bool enabled) {
    if (!enabled)
        return mount_set_tracking(TRACKING_NONE);

    MotorsState s = motors_current_state();
    TrackingMode mode = (s.tracking != TRACKING_NONE) ? s.tracking : TRACKING_SIDEREAL;
    return mount_set_tracking(mode);
}

/*
 * Set tracking to a specific ASCOM DriveRates value.
 */
MountResult alpaca_bridge_set_tracking_rate(int rate) {
    TrackingMode mode;
    switch (rate) {
        case 0: mode = TRACKING_SIDEREAL;
            break;
        case 1: mode = TRACKING_LUNAR;
            break;
        case 2: mode = TRACKING_SOLAR;
            break;
        default: mode = TRACKING_SIDEREAL;
            break;
    }
    return mount_set_tracking(mode);
}

/*
 * Fill `buf` with the ASCOM name for tracking rate `idx`
 * (0 = driveSidereal, 1 = driveLunar, 2 = driveSolar).
 */
void alpaca_bridge_get_tracking_rate_name(int idx, char *buf, size_t len) {
    switch (idx) {
        case 0: snprintf(buf, len, "driveSidereal");
            break;
        case 1: snprintf(buf, len, "driveLunar");
            break;
        case 2: snprintf(buf, len, "driveSolar");
            break;
        default: buf[0] = '\0';
            break;
    }
}
