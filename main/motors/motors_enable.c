/* Motors - motors_enable.c
 *
 * Purpose: enable motor drivers and bring the subsystem back
 * to an operational state.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_enable(void) {
    motors_queue_clear();
    motors_motion_stop();
    motors_hw_enable();
    motors_state.status = MOTORS_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
}
