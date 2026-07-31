/* REST - rest_home_handler.c
 *
 * Purpose: expose the HOME action through the API.
 */
#include "rest.h"

esp_err_t rest_home_handler(httpd_req_t *request) {
    rest_send_result(request, mount_home());
    return ESP_OK;
}
