/* REST - rest_limits_handler.c
 *
 * Purpose: handle axis-limit configuration requests from the UI.
 *
 * Accepts POST /api/limits with JSON body {"action":"<action>"}.
 * Valid actions: set_home, set_ra_left, set_ra_right, set_dec_left,
 * set_dec_right.  Each sets the corresponding limit or home position
 * from the current physical axis position and persists to NVS.
 */
#include "rest.h"

#include "mount.h"
#include "utils/utils.h"

esp_err_t rest_limits_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonStringResult action = json_get_string(body.value, "action");

    if (!action.ok) {
        http_response_bad_request(request,
            "Missing or invalid 'action'. "
            "Valid values: [set_home|set_ra_left|set_ra_right|set_dec_left|set_dec_right]");
        return ESP_OK;
    }

    rest_send_result(request, mount_limits_set(action.value));
    return ESP_OK;
}
