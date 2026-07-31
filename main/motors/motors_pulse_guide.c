/* Motors - motors_pulse_guide.c
 *
 * Purpose: enqueue a PulseGuide command for the motion task.
 * The motion task is the single writer — this function only validates
 * and queues.  Works whether tracking is active or not.
 */
#include "motors.h"
#include "motors_internal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

MotorResultCode motors_pulse_guide_start(int axis, float offset_dps, uint32_t duration_ms) {
    if (motors_state.status == MOTORS_STATUS_ERROR) {
        return MOTOR_ERR_HARDWARE_ERROR;
    }

    MotionCommand cmd = {
        .type = MOTION_CMD_PULSE_GUIDE,
        .guide_axis = axis,
        .guide_offset_dps = offset_dps,
        .guide_duration_ms = duration_ms,
    };
    motors_queue_put(&cmd);

    /* Wake the motion task so it processes the command immediately. */
    if (motors_motion_task_handle) {
        xTaskNotify(motors_motion_task_handle, 0, eNoAction);
    }
    return MOTOR_OK;
}
