/* Motors - motors_slew_axis.c
 *
 * Purpose: accept relative slew requests for a single axis.
 *
 * The motion task computes the absolute target from the live axis
 * position when the command is dequeued, guaranteeing correct
 * stacking of queued moves.
 */
#include "motors.h"
#include "motors_internal.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_SLEW_AXIS";

static MotorResultCode motors_slew_axis_impl(float ra_delta_deg, float dec_delta_deg, float lat) {
    TrackingMode currTracking = TRACKING_NONE;
    if (motors_state.status == MOTORS_STATUS_TRACKING
        && motors_state.tracking != TRACKING_NONE) {
        currTracking = motors_state.tracking;
        motors_stop();
    }

    MotionCommand cmd = {
        .type = MOTION_CMD_SLEW,
        .ra_speed = motors_state.ra_speed,
        .dec_speed = motors_state.dec_speed,
        .relative = true,
        .ra_delta_deg = ra_delta_deg,
        .dec_delta_deg = dec_delta_deg,
    };
    motors_queue_put(&cmd);

    if (currTracking != TRACKING_NONE) {
        motors_start_tracking(currTracking);
    }
    return MOTOR_OK;
}

/*
 * Move the RA axis by a relative amount in degrees.
 *
 * Positive = forward (west on HA), negative = backward.
 * Validates the future position against axis limits before enqueuing.
 * Pauses and resumes tracking automatically when active.
 */
MotorResultCode motors_slew_axis_ra(float degrees, int speed_rate, float lat) {
    if (!motors_is_valid_ra(motors_state.ra_position + degrees)) {
        ESP_LOGW(TAG, "Rejected RA move: out of range (%.3f)",
                 motors_state.ra_position + degrees);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_state.ra_speed = motors_get_slewing_speed(speed_rate);
    return motors_slew_axis_impl(degrees, 0.0f, lat);
}

/*
 * Move the DEC axis by a relative amount in degrees.
 *
 * Positive = north on declination, negative = south.
 * Validates the future position against axis limits before enqueuing.
 * Pauses and resumes tracking automatically when active.
 */
MotorResultCode motors_slew_axis_dec(float degrees, int speed_rate, float lat) {
    if (!motors_is_valid_dec(motors_state.dec_position + degrees)) {
        ESP_LOGW(TAG, "Rejected DEC move: out of range (%.3f)",
                 motors_state.dec_position + degrees);
        return MOTOR_ERR_OUT_OF_RANGE;
    }
    motors_state.dec_speed = motors_get_slewing_speed(speed_rate);
    return motors_slew_axis_impl(0.0f, degrees, lat);
}
