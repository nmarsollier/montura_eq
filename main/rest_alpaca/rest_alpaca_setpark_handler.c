#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Method — SetPark
 *
 * Purpose: Stores the current axis position as the new park position.
 *
 * Alpaca usage: Called by N.I.N.A. when the user defines a custom park position.
 */
esp_err_t alpaca_setpark_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    MountResult result = alpaca_bridge_set_park();
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
