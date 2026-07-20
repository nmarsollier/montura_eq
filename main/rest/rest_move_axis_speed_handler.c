/* Rest - rest_move_axis_speed_handler.c
 *
 * Purpose: accept continuous axis speed commands for manual slewing.
 *
 * JSON body: {"ra_rate": 0.0, "dec_rate": 0.0}
 * Positive = forward, negative = reverse, zero = stop that axis.
 */
#include "rest.h"

#include "mount.h"
#include "utils/utils.h"

esp_err_t rest_move_axis_speed_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonFloatResult ra  = json_get_float(body.value, "ra_rate");
    JsonFloatResult dec = json_get_float(body.value, "dec_rate");

    float ra_rate  = ra.ok  ? ra.value  : 0.0f;
    float dec_rate = dec.ok ? dec.value : 0.0f;

    MountResult result = mount_set_move_axis_speed(ra_rate, dec_rate);
    rest_send_result(request, result);
    return ESP_OK;
}
