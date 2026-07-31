/* Motors - motors_home.c
 *
 * Purpose: move the mount to the home position.
 *
 * Always stops any motion in progress before enqueuing the home SLEW.
 * Without the explicit stop, a SLEWING mount would ignore the new command
 * because the motion task does not poll the queue while executing a slew.
 */
#include "motors.h"
#include "motors_internal.h"

MotorResultCode motors_home(void) {
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    motors_stop();
    return motors_slew_to_angle(0.0f, 0.0f, 0);
}
