/* REST - rest_zero_handler.c
 *
 * Purpose: expose the set-zero-position action through the API.
 */
#include "rest.h"

esp_err_t rest_zero_handler(httpd_req_t *request) {
    MountResult res = mount_set_zero();
    rest_send_result(request, res);

    return ESP_OK;
}
