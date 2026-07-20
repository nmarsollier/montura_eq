#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — SlewSettleTime (GET)
 *
 * Purpose: Returns the configured post-slew settle time in seconds.
 *
 * Alpaca usage: N.I.N.A. reads this to wait after slews before imaging.
 */
esp_err_t alpaca_slewsettletime_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int result = alpaca_bridge_get_slew_settle_time();
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
