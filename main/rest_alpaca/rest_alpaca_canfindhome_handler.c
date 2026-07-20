#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Capability — CanFindHome
 *
 * Purpose: Returns true — the mount supports finding its home position.
 *
 * Alpaca usage: N.I.N.A. checks this to enable the Find Home button.
 */
esp_err_t alpaca_canfindhome_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = true;
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
