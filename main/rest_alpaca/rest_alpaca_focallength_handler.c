#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Property — FocalLength
 *
 * Purpose: Returns the telescope focal length in metres (0.900).
 *
 * Alpaca usage: N.I.N.A. uses this to compute plate scale and FOV.
 */
esp_err_t alpaca_focallength_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = ALPACA_FOCAL_LENGTH;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) (result));
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
