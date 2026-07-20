#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Property — DeclinationRate
 *
 * Purpose: Returns the current DEC tracking rate (always 0 for EQ mount).
 *
 * Alpaca usage: N.I.N.A. polls this to show DEC drift rate.
 */
esp_err_t alpaca_declinationrate_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = 0.0f;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) (result));
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
