/* REST - rest_settings_handler.c
 *
 * Purpose: handle mount settings updates.
 */
#include "rest.h"

#include <string.h>

#include "mount.h"

#include "utils/utils.h"

/*
 * Business use case: expose settings updates via the API.
 *
 * Objective: let clients configure site and session parameters and persist
 * them across restarts.
 */
esp_err_t rest_settings_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    MountSettings settings = {0};
    JsonStringResult time = json_get_string(body.value, "time");
    JsonFloatResult lat = json_get_float(body.value, "lat");
    JsonFloatResult lon = json_get_float(body.value, "lon");
    JsonIntResult elevation = json_get_int(body.value, "elevation");

    if (!lat.ok) {
        http_response_bad_request(request, "Missing or invalid 'lat'. Valid values: [-90<=lat<=90]");
        return ESP_OK;
    }

    if (!lon.ok) {
        http_response_bad_request(request, "Missing or invalid 'lon'. Valid values: [-180<=lon<=180]");
        return ESP_OK;
    }

    if (!elevation.ok) {
        http_response_bad_request(request, "Missing or invalid 'elevation'. Valid values: [int]");
        return ESP_OK;
    }
    /* Update system time first when the client sends a time value. */
    if (time.ok) {
        if (strlen(time.value) >= 128) {
            http_response_bad_request(request, "Invalid 'time'. Value is too long");
            return ESP_OK;
        }

        MountResult tr = mount_set_system_time(time.value);
        if (!tr.ok) {
            rest_send_result(request, tr);
            return ESP_OK;
        }
    }
    settings.lat = lat.value;
    settings.lon = lon.value;
    settings.elevation = elevation.value;

    rest_send_result(request, mount_settings_update(settings));

    return ESP_OK;
}
