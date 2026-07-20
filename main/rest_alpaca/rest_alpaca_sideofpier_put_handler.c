#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — SideOfPier (PUT)
 *
 * Purpose: Sets the pier side (client-reported after a flip).
 *
 * Alpaca usage: N.I.N.A. may update this after performing a meridian flip.
 */
esp_err_t alpaca_sideofpier_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int v;
    if (!alpaca_get_form_int(req, "SideOfPier", &v)) {
        alpaca_response_error(req, 1025, "Missing SideOfPier", cid, stx);
        return ESP_OK;
    }
    MountResult result = alpaca_bridge_set_side_of_pier(v);
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
