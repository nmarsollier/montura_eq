#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — SiteLatitude (PUT)
 *
 * Purpose: Updates the site latitude and persists to NVS.
 *
 * Alpaca usage: N.I.N.A. syncs its observatory latitude on connect.
 */
esp_err_t alpaca_sitelatitude_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float v;
    if (!alpaca_get_form_float(req, "SiteLatitude", &v)) {
        alpaca_response_error(req, 1025, "Missing SiteLatitude", cid, stx);
        return ESP_OK;
    }
    MountResult r = alpaca_bridge_set_site_latitude(v);
    if (r.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, r.message, cid, stx);
    return ESP_OK;
}
