/* Motors - motors_park.c
 *
 * Purpose: park both axes immediately.
 *
 * Stops the motion loop and updates state directly — no queue round-trip.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_park(void) {
    motors_queue_clear();
    motors_motion_stop();
    motors_state.status = MOTORS_STATUS_PARKED;
    motors_state.tracking = TRACKING_NONE;
}
