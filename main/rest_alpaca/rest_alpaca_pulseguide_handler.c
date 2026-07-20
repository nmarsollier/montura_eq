#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Method — PulseGuide
 *
 * Purpose: Not implemented — pulse guiding is not supported.
 *
 * Alpaca usage: N.I.N.A. disables pulse guide when this returns NotImplemented.
 */
esp_err_t alpaca_pulseguide_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_error(req, 1024, "PulseGuide not implemented", cid, stx);
    return ESP_OK;
}
