#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — TrackingRates
 *
 * Purpose: Returns the array [0, 1, 2] of supported DriveRates values.
 *
 * Alpaca usage: N.I.N.A. uses this to populate the tracking rate dropdown.
 */
esp_err_t alpaca_trackingrates_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int count = 3;
    char buf[256];
    int off = snprintf(buf, sizeof(buf), "[");
    for (int i = 0; i < count; i++) {
        char name[32];
        alpaca_bridge_get_tracking_rate_name(i, name, sizeof(name));
        off += snprintf(buf + off, sizeof(buf) - off, "%s%d", i > 0 ? "," : "", i);
    }
    snprintf(buf + off, sizeof(buf) - off, "]");
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
