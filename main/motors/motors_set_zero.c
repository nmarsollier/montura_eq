/* Motors - motors_set_zero.c
 *
 * Purpose: reset both axis step counters to zero, treating the current
 * physical position as the new reference origin.
 *
 * Always stops any active motion before zeroing — safe to call from
 * any state.  After this call the mount is READY at (0, 0).
 */
#include "motors.h"
#include "motors_internal.h"

MotorResultCode motors_set_zero(void) {
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    MotorResultCode rc = motors_stop();
    if (rc != MOTOR_OK) {
        return rc;
    }

    motors_state.ra_steps = 0;
    motors_state.dec_steps = 0;

    return MOTOR_OK;
}
