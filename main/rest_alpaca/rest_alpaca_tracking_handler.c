#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — Tracking (GET)
 *
 * Purpose: Returns true when any automatic tracking mode is active.
 *
 * Alpaca usage: N.I.N.A. polls this to show tracking state in the mount panel.
 */
esp_err_t alpaca_tracking_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();

    bool response = alpaca_bridge_get_tracking();

    alpaca_response_value(req, response ? "true" : "false", cid, stx);
    return ESP_OK;
}
