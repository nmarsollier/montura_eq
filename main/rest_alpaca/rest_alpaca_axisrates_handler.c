#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "motors.h"
#include <stdio.h>

/* Alpaca — Property — AxisRates (GET)
 *
 * Purpose: Returns the valid MoveAxis rate range in deg/s for each axis,
 * derived from the configured slew speed profiles.
 *
 * Alpaca usage: N.I.N.A. reads this to configure its MoveAxis speed slider.
 */
esp_err_t alpaca_axisrates_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    float lo = motors_get_slewing_speed(1);
    float hi = motors_get_slewing_speed(4);
    char json[96];
    snprintf(json, sizeof(json),
             "[{\"Minimum\":%.1f,\"Maximum\":%.1f},"
             "{\"Minimum\":%.1f,\"Maximum\":%.1f}]",
             (double) lo, (double) hi, (double) lo, (double) hi);
    alpaca_response_value(req, json, cid, stx);
    return ESP_OK;
}
