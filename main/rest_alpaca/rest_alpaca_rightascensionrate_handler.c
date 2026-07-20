#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Property — RightAscensionRate
 *
 * Purpose: Returns the current RA tracking rate (always 0 — rate is internal).
 *
 * Alpaca usage: N.I.N.A. polls this for display purposes.
 */
esp_err_t alpaca_rightascensionrate_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = 0.0f;
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) (result));
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
