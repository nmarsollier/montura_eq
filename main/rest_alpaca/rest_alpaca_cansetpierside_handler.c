#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Capability — CanSetPierSide
 *
 * Purpose: Returns true — pier side can be set by the client.
 *
 * Alpaca usage: N.I.N.A. may set pier side after a meridian flip.
 */
esp_err_t alpaca_cansetpierside_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = true;
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
