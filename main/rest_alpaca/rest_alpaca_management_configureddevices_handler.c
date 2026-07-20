#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include <stdio.h>

/* Alpaca — Management — ConfiguredDevices
 *
 * Purpose: Returns the list of ASCOM devices exposed by this server (one telescope).
 *
 * Alpaca usage: Called at connection time to discover device numbers and types.
 */
esp_err_t alpaca_management_configureddevices_handler(httpd_req_t *req) {
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"Value\":[{"
             "\"DeviceName\":\"%s\","
             "\"DeviceType\":\"%s\","
             "\"DeviceNumber\":%d,"
             "\"UniqueID\":\"%s\""
             "}],"
             "\"ErrorNumber\":0,\"ErrorMessage\":\"\","
             "\"ClientTransactionID\":0,\"ServerTransactionID\":0}",
             ALPACA_DEVICE_NAME, ALPACA_DEVICE_TYPE,
             ALPACA_DEVICE_NUMBER, ALPACA_UNIQUE_ID);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, strlen(buf));
    return ESP_OK;
}
