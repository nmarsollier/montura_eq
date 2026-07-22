#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "motors.h"

/* Alpaca — Property — IsPulseGuiding
 *
 * Purpose: Returns the real PulseGuide state from the motors layer.
 *
 * Alpaca usage: N.I.N.A. polls this during autoguiding sessions.
 */
esp_err_t alpaca_ispulseguiding_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    bool active = motors_is_pulse_guiding();
    alpaca_response_value(req, active ? "true" : "false", cid, stx);
    return ESP_OK;
}
