/* Motors - motors_init.c
 *
 * Purpose: initialize the motors module — state, hardware, queue, and motion task.
 */
#include "motors.h"
#include "motors_internal.h"

#include "esp_log.h"
#include "esp_err.h"

/*
 * Default motors state — positions in microsteps, home = 0.
 * Axis limits are configured via .limits (see MotorsState).
 */
MotorsState motors_state = {
    .ra_steps = 0,
    .dec_steps = 0,
    .status = MOTORS_STATUS_READY,
    .tracking = TRACKING_NONE,
    .ra_speed = 0.0f,
    .dec_speed = 0.0f,
    .limits = {
        .ra_min = -100.0f,
        .ra_max = 100.0f,
        .dec_min = -150.0f,
        .dec_max = 150.0f,
    },
};

esp_err_t motors_init(void) {
    esp_err_t err;

    err = motors_hw_init();
    if (err != ESP_OK) {
        ESP_LOGE("MOTORS_INIT", "motors_hw_init: %s", esp_err_to_name(err));
        motors_state.status = MOTORS_STATUS_DISABLED;
        return err;
    }

    err = motors_rmt_init();
    if (err != ESP_OK) {
        ESP_LOGE("MOTORS_INIT", "motors_rmt_init: %s", esp_err_to_name(err));
        motors_state.status = MOTORS_STATUS_DISABLED;
        return err;
    }

    motors_queue_init();
    motors_motion_task_init();
    return ESP_OK;
}
