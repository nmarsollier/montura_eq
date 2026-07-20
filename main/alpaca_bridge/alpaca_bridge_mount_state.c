/* Alpaca bridge — mount state queries
 *
 * Each function reads the authoritative MotorsState from the motors
 * module and returns a boolean matching the Alpaca property.
 */

#include "alpaca_bridge.h"

#include <math.h>

#include "motors/motors.h"

bool alpaca_bridge_get_is_slewing(void) {
    MotorsState s = motors_current_state();
    return s.status == MOTORS_STATUS_SLEWING;
}

bool alpaca_bridge_get_is_tracking(void) {
    MotorsState s = motors_current_state();
    return s.status == MOTORS_STATUS_TRACKING;
}

bool alpaca_bridge_get_is_parked(void) {
    MotorsState s = motors_current_state();
    return s.status == MOTORS_STATUS_PARKED;
}

/*
 * Home is an approximation: the mount is considered "at home" when
 * it is READY and both axes are within 1° of the origin (0, 0).
 */
bool alpaca_bridge_get_is_home(void) {
    MotorsState s = motors_current_state();
    return s.status == MOTORS_STATUS_READY &&
           fabsf(s.ra_position) < 1.0f &&
           fabsf(s.dec_position) < 1.0f;
}
