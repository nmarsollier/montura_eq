#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Capability — CanSetGuideRates
 *
 * Purpose: Returns false — guide rates are not configurable.
 *
 * Alpaca usage: N.I.N.A. uses default guide rates.
 */
esp_err_t alpaca_cansetguiderates_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = false;
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
