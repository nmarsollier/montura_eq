#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Property — DeclinationRate (PUT)
 *
 * Purpose: Not implemented. DEC rate is managed internally.
 *
 * Alpaca usage: Returns NotImplemented (1024) to the client.
 */
esp_err_t alpaca_declinationrate_put_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_error(req, 1024, "Not implemented", cid, stx);
    return ESP_OK;
}
