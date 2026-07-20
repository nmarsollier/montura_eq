#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Method — SlewToTargetAsync
 *
 * Purpose: Slews to the previously set target coordinates.
 *
 * Alpaca usage: N.I.N.A. sets target coordinates first, then calls this to slew.
 */
esp_err_t alpaca_slewtotargetasync_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    MountResult result = alpaca_bridge_slew_to_target();
    if (result.ok)
        alpaca_response_ok(req, cid, stx);
    else
        alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
