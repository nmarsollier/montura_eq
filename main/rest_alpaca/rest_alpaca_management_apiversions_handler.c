#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Management — APIVersions
 *
 * Purpose: Returns the list of Alpaca API versions supported by this server.
 *
 * Alpaca usage: Called once by clients after discovery to negotiate the protocol version.
 */
esp_err_t alpaca_management_apiversions_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"Value\":[1],\"ErrorNumber\":0,"
                       "\"ErrorMessage\":\"\",\"ClientTransactionID\":0,"
                       "\"ServerTransactionID\":0}");
    return ESP_OK;
}
