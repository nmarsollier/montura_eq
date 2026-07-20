/* Motors - motors_current_state.c
 *
 * Purpose: return a copy of the authoritative motors state.
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
