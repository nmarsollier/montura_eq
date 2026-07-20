#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Method — SlewToAltAzAsync
 *
 * Purpose: Not implemented — equatorial mount cannot slew in Alt/Az.
 *
 * Alpaca usage: Returns NotImplemented error (1024) to the client.
 */
esp_err_t alpaca_slewtoaltazasync_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_error(req, 1024, "Alt/Az slew not implemented (EQ mount)",
                          cid, stx);
    return ESP_OK;
}
