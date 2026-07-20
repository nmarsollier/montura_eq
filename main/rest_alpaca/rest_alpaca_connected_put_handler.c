#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Device — Connected (PUT)
 *
 * Purpose: Accepts the Connected flag. No-op — the device cannot be disconnected.
 *
 * Alpaca usage: N.I.N.A. calls this on connect/disconnect; ignored by hardware.
 */
esp_err_t alpaca_connected_put_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_ok(req, cid, stx);
    return ESP_OK;
}
