#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — DestinationSideOfPier (GET)
 *
 * Purpose: Returns the pier side the mount WOULD be on after a slew
 * to the given RA/Dec, computed from the current time and site.
 *
 * N.I.N.A. calls this to decide whether a slew will cause a meridian flip:
 *   GET /api/v1/telescope/0/destinationsideofpier?RightAscension=18.5&Declination=-24.0
 *
 * Response: 0 = pierEast, 1 = pierWest.
 */
esp_err_t alpaca_destinationsideofpier_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();

    float ra = 0.0f, dec = 0.0f;
    if (!alpaca_get_form_float(req, "RightAscension", &ra) ||
        !alpaca_get_form_float(req, "Declination", &dec)) {
        alpaca_response_error(req, 1025,
                              "Missing RightAscension or Declination", cid, stx);
        return ESP_OK;
    }

    int result = alpaca_bridge_get_destination_side_of_pier(ra, dec);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
