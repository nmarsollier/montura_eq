#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — Tracking (PUT)
 *
 * Purpose: Enables (sidereal) or disables tracking.
 *
 * Alpaca usage: N.I.N.A. toggles tracking on/off via this endpoint.
 */
esp_err_t alpaca_tracking_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool enabled = false;
    alpaca_get_form_bool(req, "Tracking", &enabled);
    MountResult r = alpaca_bridge_set_tracking(enabled);
    if (r.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, r.message, cid, stx);
    return ESP_OK;
}
