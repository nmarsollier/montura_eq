#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Device — Connected (GET)
 *
 * Purpose: Always returns true — the embedded device is always connected.
 *
 * Alpaca usage: Polled by N.I.N.A. to verify the device is reachable.
 */
esp_err_t alpaca_connected_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    const char *result = "true";
    alpaca_response_value(req, result, cid, stx);
    return ESP_OK;
}
