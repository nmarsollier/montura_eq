#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Property — DoesRefraction
 *
 * Purpose: Returns false — atmospheric refraction correction is not applied.
 *
 * Alpaca usage: N.I.N.A. may apply its own refraction correction.
 */
esp_err_t alpaca_doesrefraction_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = false;
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
