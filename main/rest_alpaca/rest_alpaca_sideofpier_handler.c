#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — SideOfPier (GET)
 *
 * Purpose: Returns the pier side: 0 = East, 1 = West.
 *
 * Alpaca usage: N.I.N.A. uses this for meridian flip decisions.
 */
esp_err_t alpaca_sideofpier_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int result = alpaca_bridge_get_side_of_pier();
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
