#include "rest.h"

#include <stdio.h>
#include <string.h>

#include "mount.h"

#include "utils/utils.h"

/*
 * Business use case: expose MOVE-AXIS via the API with input validation.
 *
 * Objective: accept an axis, a degree delta, and a speed profile from
 * external clients and turn them into a validated relative move request.
 */
esp_err_t rest_move_axis_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonStringResult axis_str = json_get_string(body.value, "axis");
    JsonFloatResult degrees = json_get_float(body.value, "degrees");
    JsonIntResult speed_rate = json_get_int(body.value, "speed");
    int speed_rate_value = 1;

    if (!axis_str.ok) {
        static const char format[] = "Missing or invalid 'axis'. Valid values: %s";
        const char *valid = motors_axis_valid_values();
        char message[strlen(valid) + sizeof(format)];

        snprintf(message, sizeof(message), format, valid);

        http_response_bad_request(request, message);
        return ESP_OK;
    }

    MotorAxis axis = motors_axis_from_string(axis_str.value);

    if (axis == MOTOR_AXIS_UNKNOWN) {
        static const char format[] = "Invalid 'axis'. Valid values: %s";
        const char *valid = motors_axis_valid_values();
        char message[strlen(valid) + sizeof(format)];

        snprintf(message, sizeof(message), format, valid);

        http_response_bad_request(request, message);
        return ESP_OK;
    }

    if (!degrees.ok) {
        http_response_bad_request(request, "Missing or invalid 'degrees'");
        return ESP_OK;
    }

    bool has_speed_rate = strstr(body.value, "\"speed\"") != NULL;

    if (has_speed_rate && !speed_rate.ok) {
        http_response_bad_request(request, "Invalid 'speed'. Valid values: [1<=speed_rate<=4]");
        return ESP_OK;
    }

    if (has_speed_rate) {
        speed_rate_value = speed_rate.value;
    }

    if (axis == MOTOR_AXIS_RA) {
        rest_send_result(request, mount_move_axis_ra(degrees.value, speed_rate_value));
    } else {
        rest_send_result(request, mount_move_axis_dec(degrees.value, speed_rate_value));
    }

    return ESP_OK;
}
