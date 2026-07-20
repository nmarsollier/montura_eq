#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Device — Description
 *
 * Purpose: Returns the telescope device description string.
 *
 * Alpaca usage: Shown in N.I.N.A. equipment panel and ASCOM driver info.
 */
esp_err_t alpaca_description_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    const char *result = ALPACA_SERVER_DESCRIPTION;
    char buf[128];
    snprintf(buf, sizeof(buf), "\"%s\"", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
