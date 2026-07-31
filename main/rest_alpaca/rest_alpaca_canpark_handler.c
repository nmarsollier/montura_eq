#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "motors/motors.h"

/* Alpaca — Capability — CanPark
 *
 * Purpose: Returns true — the mount can park.
 *
 * Alpaca usage: N.I.N.A. checks this to enable park/unpark in the sequence engine.
 */
esp_err_t alpaca_canpark_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool result = motors_current_state().status != MOTORS_STATUS_ERROR;
    alpaca_response_value(req, result ? "true" : "false", cid, stx);
    return ESP_OK;
}
