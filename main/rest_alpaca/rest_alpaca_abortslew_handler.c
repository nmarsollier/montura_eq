#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Method — AbortSlew
 *
 * Purpose: Immediately stops any ongoing slew.
 *
 * Alpaca usage: Called by N.I.N.A. when the user presses Stop or a safety limit triggers.
 */
esp_err_t alpaca_abortslew_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    MountResult r = alpaca_bridge_abort_slew();
    if (r.ok)
        alpaca_response_ok(req, cid, stx);
    else
        alpaca_response_error(req, 1025, r.message, cid, stx);
    return ESP_OK;
}
