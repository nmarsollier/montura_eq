/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement immediately.
 *
 * Clears the command queue, resets state, and delegates hardware stop
 * and task notification to motors_motion_stop().
 */
#include "motors.h"
#include "motors_internal.h"

MotorResultCode motors_stop(void) {
    /* ERROR is unrecoverable — never transition out of it. */
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    motors_queue_clear();
    motors_state.status = MOTORS_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
    motors_state.guiding = false;
    motors_motion_stop();
    return MOTOR_OK;
}
