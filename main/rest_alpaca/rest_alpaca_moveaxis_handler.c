/* Alpaca — Method — MoveAxis
 *
 * Purpose: moves a single axis (0 = RA, 1 = DEC) continuously at the
 * given rate until Rate = 0 stops it.
 *
 * The Rate parameter uses our AxisRates range (1..4, matching the
 * slew speed profiles).  Zero stops.  Sign sets direction.
 */
#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"
#include "motors.h"

#include <math.h>

static float s_ra_rate = 0.0f;
static float s_dec_rate = 0.0f;

esp_err_t alpaca_moveaxis_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    int axis = 0;
    float rate = 0.0f;

    bool got_axis = alpaca_get_form_int(req, "Axis", &axis);
    bool got_rate = alpaca_get_form_float(req, "Rate", &rate);

    if (!got_axis || !got_rate) {
        alpaca_response_error(req, 1025, "Missing Axis or Rate", cid, stx);
        return ESP_OK;
    }

    float hi = motors_get_slewing_speed(4);
    if (axis == 0) {
        s_ra_rate = fmaxf(fminf(rate, hi), 0.0f);
    } else {
        s_dec_rate = fmaxf(fminf(rate, hi), 0.0f);
    }

    MountResult result = mount_set_move_axis_speed(s_ra_rate, s_dec_rate);
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}

void alpaca_moveaxis_reset(void) {
    s_ra_rate = 0.0f;
    s_dec_rate = 0.0f;
    mount_move_axis_reset();
}
