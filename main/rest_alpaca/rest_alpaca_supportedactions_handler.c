#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Device — SupportedActions
 *
 * Purpose: Returns the list of custom action names (empty for Montura EQ).
 *
 * Alpaca usage: Called by clients that support vendor-specific action extensions.
 */
esp_err_t alpaca_supportedactions_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_value(req, "[]", cid, stx);
    return ESP_OK;
}
