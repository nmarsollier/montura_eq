/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement immediately.
 *
 * Stops the motion loop and updates state directly — no queue round-trip.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_stop(void) {
    motors_queue_clear();
    motors_motion_stop();
    motors_state.status = MOTORS_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
}
