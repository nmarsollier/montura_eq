#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — TrackingRate (GET)
 *
 * Purpose: Returns the current tracking rate as a DriveRates enum (0, 1, or 2).
 *
 * Alpaca usage: N.I.N.A. displays the current tracking rate.
 */
esp_err_t alpaca_trackingrate_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int result = alpaca_bridge_get_tracking_rate();
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
