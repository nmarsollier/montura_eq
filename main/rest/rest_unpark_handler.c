#include "rest.h"

#include "mount.h"

/*
 * Business use case: expose UNPARK via the API.
 *
 * Objective: let remote clients return the mount to normal operation after
 * parking.
 */
esp_err_t rest_unpark_handler(httpd_req_t *request) {
    rest_send_result(request, mount_unpark());

    return ESP_OK;
}
