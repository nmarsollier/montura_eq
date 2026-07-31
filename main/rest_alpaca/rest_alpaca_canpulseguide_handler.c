#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "motors/motors.h"

/* Alpaca — Capability — CanPulseGuide
 *
 * Purpose: Returns true — pulse guiding is fully supported.
 *
 * Alpaca usage: N.I.N.A. enables the pulse guide controls.
 */
esp_err_t alpaca_canpulseguide_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_value(req, "true", cid, stx);
    return ESP_OK;
}
