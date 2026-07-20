/* Motors - motors_slew_to_angle.c
 *
 * Purpose: move both axes to absolute axis angles (degrees).
 *
 * Pauses and resumes tracking automatically.  Target validation
 * happens here; the motion task only executes the commanded move.
 */
#include "motors.h"
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_SLEW_TO_ANGLE";

/*
 * Move both axes to absolute axis angles in degrees.
 *
 * Validates both targets against axis limits before enqueuing the SLEW
 * command.  If tracking is active it is paused for the slew and
 * automatically resumed afterwards — the caller does not need to
 * manage tracking state.
 *
 * Parameters:
 *   ra_deg   — target RA axis angle in degrees (validated against limits)
 *   dec_deg  — target DEC axis angle in degrees
 *   speed_rate — slew profile (1=2°/s, 2=8°/s, 3=16°/s, default=24°/s)
 *   lat      — site latitude (used to restore tracking direction)
 */
MotorResultCode motors_slew_to_angle(float ra_deg, float dec_deg, int speed_rate, float lat) {
    TrackingMode currTracking = TRACKING_NONE;
    if (motors_state.status == MOTORS_STATUS_TRACKING
        && motors_state.tracking != TRACKING_NONE) {
        currTracking = motors_state.tracking;
        motors_stop();
    }

    if (!motors_is_valid_ra(ra_deg)) {
        ESP_LOGW(TAG, "Rejected slew: RA out of range (%.3f)", ra_deg);
        return MOTOR_ERR_OUT_OF_RANGE;
    }

    if (!motors_is_valid_dec(dec_deg)) {
        ESP_LOGW(TAG, "Rejected slew: DEC out of range (%.3f)", dec_deg);
        return MOTOR_ERR_OUT_OF_RANGE;
    }

    float speed = motors_get_slewing_speed(speed_rate);

    motors_state.ra_speed = speed;
    motors_state.dec_speed = speed;

    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_target_deg = ra_deg,
        .dec_target_deg = dec_deg,
        .ra_speed = speed,
        .dec_speed = speed,
    };
    motors_queue_put(&cmd);

    if (currTracking != TRACKING_NONE) {
        motors_start_tracking(currTracking);
    }

    return MOTOR_OK;
}
