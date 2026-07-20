/* Motors - motors_disable.c
 *
 * Purpose: disable motor drivers immediately.
 *
 * Kills hardware, stops the motion loop, and updates state directly.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_disable(void) {
    motors_queue_clear();
    motors_motion_stop();
    motors_hw_disable();
    motors_state.status = MOTORS_STATUS_DISABLED;
    motors_state.tracking = TRACKING_NONE;
}
