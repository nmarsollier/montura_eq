/* Motors - motors_stop.c
 *
 * Purpose: stop all motor movement immediately.
 *
 * Clears the command queue, signals the motion task, and updates
 * state.  The motion task handles RMT abort internally.
 */
#include "motors.h"
#include "motors_internal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void motors_stop(void) {
    motors_queue_clear();
    motors_state.status = MOTORS_STATUS_READY;
    motors_state.tracking = TRACKING_NONE;
    motors_state.guiding = false;

    /* Signal the motion task.  It will detect the state change,
     * abort any in-flight RMT, and return to queue-wait. */
    motors_motion_active = false;
    if (motors_motion_task_handle) {
        xTaskNotify(motors_motion_task_handle, 0, eNoAction);
        /* Also abort RMT to wake the task from xSemaphoreTake. */
        motors_rmt_abort_both();
    }
}
