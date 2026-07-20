#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Property — AlignmentMode
 *
 * Purpose: Returns algGermanPolar (1) — the mount is a German equatorial.
 *
 * Alpaca usage: N.I.N.A. uses this to determine pier-side and meridian flip logic.
 */
esp_err_t alpaca_alignmentmode_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int result = 1;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
