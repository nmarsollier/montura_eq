/* REST - rest_tracking_handler.c
 *
 * Purpose: handle tracking mode changes.
 */
#include "rest.h"

#include <stdio.h>
#include <string.h>

#include "mount.h"

#include "utils/utils.h"

/*
 * Business use case: expose tracking changes via the API.
 *
 * Objective: let clients select the tracking mode for the current target and
 * observing strategy.
 */
esp_err_t rest_tracking_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonStringResult tracking_text = json_get_string(body.value, "tracking");
    TrackingMode tracking = tracking_from_string(tracking_text.value);

    if (!tracking_text.ok) {
        static const char format[] = "Missing or invalid 'tracking'. Valid values: %s";
        const char *valid = tracking_valid_values();

        char message[strlen(valid) + sizeof(format)];
        snprintf(message, sizeof(message), format, valid);

        http_response_bad_request(request, message);
        return ESP_OK;
    }

    rest_send_result(request, mount_set_tracking(tracking));

    return ESP_OK;
}
