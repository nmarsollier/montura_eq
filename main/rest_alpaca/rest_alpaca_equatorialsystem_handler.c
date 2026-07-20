#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Property — EquatorialSystem
 *
 * Purpose: Returns equJ2000 (2) — the coordinate system used.
 *
 * Alpaca usage: N.I.N.A. uses this for coordinate epoch calculations.
 */
esp_err_t alpaca_equatorialsystem_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int result = 2;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
