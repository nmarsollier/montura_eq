#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Device — InterfaceVersion
 *
 * Purpose: Returns the ASCOM interface version number (3 = ITelescopeV3).
 *
 * Alpaca usage: Checked by clients to determine available features.
 */
esp_err_t alpaca_interfaceversion_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int result = (int) (ALPACA_INTERFACE_VERSION);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
