/* Motors - motors_is_pulse_guiding.c
 *
 * Purpose: return the authoritative guiding state from motors_state.
 * Written exclusively by the motion task.
 */
#include "motors.h"
#include "motors_internal.h"

bool motors_is_pulse_guiding(void) {
    return motors_state.guiding;
}
