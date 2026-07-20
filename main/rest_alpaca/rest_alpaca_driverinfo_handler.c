#include <stdio.h>
#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Device — DriverInfo
 *
 * Purpose: Returns driver identification string.
 *
 * Alpaca usage: Shown in ASCOM diagnostics / ConformU validation.
 */
esp_err_t alpaca_driverinfo_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();

    char *result = ALPACA_DRIVER_INFO;

    char buf[128];
    snprintf(buf, sizeof(buf), "\"%s\"", result);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
