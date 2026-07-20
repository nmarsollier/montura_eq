/* Motors - motors_move_axis_speed.c
 *
 * Purpose: request continuous single-axis speed motion.
 * Positive rate = forward, negative = reverse, zero = stop that axis.
 * Used by Alpaca MoveAxis, joystick, and guiding.
 */
#include "motors.h"
#include "motors_internal.h"

void motors_set_move_axis_speed(float ra_speed, float dec_speed) {
    if ((int) ra_speed == 0 && (int) dec_speed == 0) {
        motors_stop();
        return;
    }

    MotionCommand cmd = {
        .type = MOTION_CMD_MOVE_AXIS,
        .ra_target_deg = 0.0f, /* set by the task from limits */
        .dec_target_deg = 0.0f,
        .ra_speed = ra_speed,
        .dec_speed = dec_speed,
        .tracking_mode = TRACKING_NONE,
    };
    motors_queue_put(&cmd);
}
