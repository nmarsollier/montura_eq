/* REST - rest_screen_handler.c
 *
 * Purpose: serve the embedded screen view.
 */
#include "rest.h"
#include "utils/utils.h"

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

/*
 * Business use case: expose the mounted UI as static HTML.
 *
 * Objective: let clients and operators load the embedded interface directly
 * from the device.
 */
esp_err_t rest_html_handler(httpd_req_t *request) {
    http_response_html(request, index_html_start, index_html_end - index_html_start);

    return ESP_OK;
}
