#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — SiteElevation (PUT)
 *
 * Purpose: Updates the site elevation and persists to NVS.
 *
 * Alpaca usage: N.I.N.A. syncs its observatory elevation on connect.
 */
esp_err_t alpaca_siteelevation_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int v;
    if (!alpaca_get_form_int(req, "SiteElevation", &v)) {
        alpaca_response_error(req, 1025, "Missing SiteElevation", cid, stx);
        return ESP_OK;
    }
    MountResult result = alpaca_bridge_set_site_elevation(v);
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
