#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Device — DriverVersion
 *
 * Purpose: Returns the driver version as a string.
 *
 * Alpaca usage: Used by clients for compatibility checks.
 */
esp_err_t alpaca_driverversion_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    char buf[128];
    snprintf(buf, sizeof(buf), "\"%s\"", ALPACA_DRIVER_VERSION);
    alpaca_response_value(req, buf, cid, stx);
    return ESP_OK;
}
