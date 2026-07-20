/* REST - rest_slew_to_coordinates_handler.c
 *
 * Purpose: handle slew-to-coordinates requests.
 */
#include "rest.h"

#include <stdio.h>
#include <string.h>

#include "mount.h"

#include "utils/utils.h"

esp_err_t rest_slew_to_coordinates_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonFloatResult ra = json_get_float(body.value, "ra");
    JsonFloatResult dec = json_get_float(body.value, "dec");
    JsonIntResult speed_rate = json_get_int(body.value, "speed");
    int speed_rate_value = 1;

    if (!ra.ok) {
        http_response_bad_request(request, "Missing or invalid 'ra'. Valid values: [0<=ra<24]");
        return ESP_OK;
    }

    if (!dec.ok) {
        http_response_bad_request(request, "Missing or invalid 'dec'. Valid values: [-90<=dec<=90]");
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

    rest_send_result(request, mount_slew_to_coordinates(ra.value, dec.value, speed_rate_value));

    return ESP_OK;
}
