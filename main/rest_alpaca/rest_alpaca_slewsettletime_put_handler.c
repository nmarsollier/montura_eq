#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — SlewSettleTime (PUT)
 *
 * Purpose: Sets the post-slew settle time in seconds.
 *
 * Alpaca usage: N.I.N.A. writes this from the equipment settings.
 */
esp_err_t alpaca_slewsettletime_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int v;
    if (!alpaca_get_form_int(req, "SlewSettleTime", &v)) {
        alpaca_response_error(req, 1025, "Missing SlewSettleTime", cid, stx);
        return ESP_OK;
    }
    MountResult r = alpaca_bridge_set_slew_settle_time(v);
    if (r.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, r.message, cid, stx);
    return ESP_OK;
}
