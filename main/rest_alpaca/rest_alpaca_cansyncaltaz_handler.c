#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Capability — CanSyncAltAz
 *
 * Purpose: Returns false — Alt/Az sync is not supported by this mount.
 *
 * Alpaca usage: N.I.N.A. checks this capability.
 */
esp_err_t alpaca_cansyncaltaz_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_value(req, "false", cid, stx);
    return ESP_OK;
}
