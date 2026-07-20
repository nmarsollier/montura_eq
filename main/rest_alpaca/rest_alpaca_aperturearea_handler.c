#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Property — ApertureArea
 *
 * Purpose: Returns the telescope aperture area in m^2 (0.0113 = 120mm).
 *
 * Alpaca usage: N.I.N.A. uses this for image-scale and flux calculations.
 */
esp_err_t alpaca_aperturearea_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = ALPACA_APERTURE_AREA;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) (result));
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
