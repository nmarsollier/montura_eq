/* Motors - motors_enter_error_state.c
 *
 * Purpose: single entry point to put the motors subsystem into the
 * unrecoverable ERROR state.
 *
 * Called when a hardware fault is detected (TMC init failure, RMT init
 * failure, or mid-motion emergency abort).  This function:
 *
 *   1. Aborts any in-flight RMT transmission (stops step pulses).
 *   2. Sets motors_state.status = MOTORS_STATUS_ERROR.
 *   3. Clears the guiding flag.
 *   4. Physically disables the motor drivers (ENABLE pin inactive).
 *
 * Once in ERROR, the motors module rejects all movement commands.
 * Only a full reboot can clear this state.
 */
#include "motors_internal.h"

void motors_enter_error_state(void) {
    motors_rmt_abort_both();
    motors_state.status = MOTORS_STATUS_ERROR;
    motors_state.guiding = false;
    motors_hw_disable();
}
