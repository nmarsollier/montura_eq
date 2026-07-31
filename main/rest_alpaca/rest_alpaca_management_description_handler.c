#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Management — Description
 *
 * Purpose: Returns server metadata: name, manufacturer, version, location.
 *
 * Alpaca usage: Displayed in N.I.N.A.'s device chooser and ASCOM diagnostics.
 */
esp_err_t alpaca_management_description_handler(httpd_req_t *req) {
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"Value\":{"
             "\"ServerName\":\"%s\","
             "\"Manufacturer\":\"NRM-WO\","
             "\"ManufacturerVersion\":\"%s\","
             "\"Location\":\"Embedded\""
             "},"
             "\"ErrorNumber\":0,\"ErrorMessage\":\"\","
             "\"ClientTransactionID\":0,\"ServerTransactionID\":0}",
             ALPACA_SERVER_NAME, ALPACA_DRIVER_VERSION);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, strlen(buf));
    return ESP_OK;
}
