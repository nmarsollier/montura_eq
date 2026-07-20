#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Device — Name
 *
 * Purpose: Returns the telescope device name.
 *
 * Alpaca usage: Shown in N.I.N.A. equipment chooser dropdown.
 */
esp_err_t alpaca_name_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    char buf[128];
    snprintf(buf, sizeof(buf), "\"%s\"", ALPACA_DEVICE_NAME);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
