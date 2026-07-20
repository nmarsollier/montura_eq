#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Property — ApertureDiameter
 *
 * Purpose: Returns the telescope aperture diameter in metres (0.120 = 120mm).
 *
 * Alpaca usage: N.I.N.A. displays aperture; used for photometry calibration.
 */
esp_err_t alpaca_aperturediameter_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = ALPACA_APERTURE_DIAMETER;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) (result));
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
