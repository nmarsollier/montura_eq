#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

/* Alpaca — Property — TrackingRate (PUT)
 *
 * Purpose: Sets the tracking rate: 0 = sidereal, 1 = lunar, 2 = solar.
 *
 * Alpaca usage: N.I.N.A. allows the user to select the tracking rate for solar/lunar.
 */
esp_err_t alpaca_trackingrate_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int rate = 0;
    alpaca_get_form_int(req, "TrackingRate", &rate);
    MountResult r = alpaca_bridge_set_tracking_rate(rate);
    if (r.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, r.message, cid, stx);
    return ESP_OK;
}
