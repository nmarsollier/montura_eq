#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"
#include <stdio.h>

/* Alpaca — Property — Declination
 *
 * Purpose: Returns the current declination in degrees.
 *
 * Alpaca usage: N.I.N.A. shows this in the main mount display; core coordinate.
 */
esp_err_t alpaca_declination_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float result = mount_get_dec_deg();
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", (double) (result));
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
