/* Motors - motors_current_state.c
 *
 * Purpose: return a copy of the authoritative motors state,
 * and provide degree-format accessors for external consumers.
 */
#include "motors.h"
#include "motors_internal.h"

/*
 * Return a snapshot copy of the motors module's authoritative state.
 * External consumers should use this for status and telemetry reads.
 */
MotorsState motors_current_state(void) {
    return motors_state;
}

float motors_get_ra_deg(void) {
    return motors_steps_to_deg(motors_state.ra_steps);
}

float motors_get_dec_deg(void) {
    return motors_steps_to_deg(motors_state.dec_steps);
}
