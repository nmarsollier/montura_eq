#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "motors/motors.h"

/* Alpaca — Capability — CanSetTracking
 *
 * Purpose: Returns true — tracking can be enabled/disabled by the client.
 *
 * Alpaca usage: N.I.N.A. enables the tracking toggle.
 */
esp_err_t alpaca_cansettracking_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = motors_current_state().status != MOTORS_STATUS_ERROR;
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
