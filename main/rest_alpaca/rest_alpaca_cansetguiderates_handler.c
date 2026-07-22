#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Capability — CanSetGuideRates
 *
 * Purpose: Returns true — guide rates are fully configurable.
 *
 * Alpaca usage: N.I.N.A. enables guide rate controls.
 */
esp_err_t alpaca_cansetguiderates_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_value(req, "true", cid, stx);
    return ESP_OK;
}
