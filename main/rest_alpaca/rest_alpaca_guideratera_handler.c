#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — GuideRateRightAscension
 *
 * Purpose: Returns the RA guide rate in degrees/second.
 *
 * Alpaca usage: N.I.N.A. uses this for autoguiding calibration.
 */
esp_err_t alpaca_guideratera_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float rate = alpaca_bridge_get_guide_rate_ra();
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double)rate);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
