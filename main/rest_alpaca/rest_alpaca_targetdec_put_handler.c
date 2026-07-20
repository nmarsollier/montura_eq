#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — TargetDeclination (PUT)
 *
 * Purpose: Stores the target declination for later slew/sync.
 *
 * Alpaca usage: N.I.N.A. sets this before calling SlewToTarget or SyncToTarget.
 */
esp_err_t alpaca_targetdec_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float v;
    if (!alpaca_get_form_float(req, "TargetDeclination", &v)) {
        alpaca_response_error(req, 1025, "Missing TargetDeclination", cid, stx);
        return ESP_OK;
    }
    alpaca_bridge_set_target_dec(v);
    alpaca_response_ok(req, cid, stx);
    return ESP_OK;
}
