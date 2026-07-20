#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — AtHome
 *
 * Purpose: Returns true when the mount is at the home position.
 *
 * Alpaca usage: N.I.N.A. polls this after a FindHome command.
 */
esp_err_t alpaca_athome_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = alpaca_bridge_get_is_home();
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
