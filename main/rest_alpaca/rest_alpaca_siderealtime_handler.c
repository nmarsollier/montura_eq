#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — SiderealTime
 *
 * Purpose: Returns the local apparent sidereal time in hours.
 *
 * Alpaca usage: N.I.N.A. displays LST; used for coordinate transformations.
 */
esp_err_t alpaca_siderealtime_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = alpaca_bridge_get_sidereal_time();
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) (result));
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
