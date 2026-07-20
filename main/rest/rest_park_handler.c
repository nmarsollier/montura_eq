/* REST - rest_park_handler.c
 *
 * Purpose: handle park requests.
 */
#include "rest.h"

#include "mount.h"

/*
 * Business use case: expose PARK via the API.
 *
 * Objective: let remote clients move the mount to its parked state safely.
 */
esp_err_t rest_park_handler(httpd_req_t *request) {
    rest_send_result(request, mount_park());

    return ESP_OK;
}
