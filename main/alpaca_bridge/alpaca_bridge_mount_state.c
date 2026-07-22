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
    float ra = motors_get_ra_deg();
    float dec = motors_get_dec_deg();
    return s.status == MOTORS_STATUS_READY &&
           fabsf(ra) < 1.0f && fabsf(dec) < 1.0f;
}
