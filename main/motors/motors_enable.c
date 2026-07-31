/* Motors - motors_enable.c
 *
 * Purpose: enable motor drivers and bring the subsystem back
 * to an operational state.
 */
#include "motors.h"
#include "motors_internal.h"

MotorResultCode motors_enable(void) {
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    motors_queue_clear();
    motors_motion_stop();
    motors_hw_enable();
    motors_state.status = MOTORS_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
    return MOTOR_OK;
}
