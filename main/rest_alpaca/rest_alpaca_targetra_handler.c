#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdio.h>

/* Alpaca — Property — TargetRightAscension (GET)
 *
 * Purpose: Returns the stored target right ascension in hours.
 *
 * Alpaca usage: N.I.N.A. reads this back after setting it.
 */
esp_err_t alpaca_targetra_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = alpaca_bridge_get_target_ra();
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
