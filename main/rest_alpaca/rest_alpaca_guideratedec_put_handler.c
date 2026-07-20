#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Property — GuideRateDeclination (PUT)
 *
 * Purpose: Not implemented. Guide rate uses the default 0.5x sidereal.
 *
 * Alpaca usage: Returns NotImplemented (1024). N.I.N.A. uses the GET value as-is.
 */
esp_err_t alpaca_guideratedec_put_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_error(req, 1024, "Not implemented", cid, stx);
    return ESP_OK;
}
