/* Alpaca — Property — GuideRateDeclination (PUT)
 *
 * Validates using strtof() with full endptr + isfinite() checks.
 */
#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"

#include <stdlib.h>
#include <math.h>

esp_err_t alpaca_guideratedec_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();

    char *val = alpaca_get_form_param(req, "GuideRateDeclination");
    if (!val) {
        alpaca_response_error(req, 1025, "Missing GuideRateDeclination", cid, stx);
        return ESP_OK;
    }

    char *end = NULL;
    float rate = strtof(val, &end);
    if (end == val || *end != '\0' || !isfinite(rate)) {
        free(val);
        alpaca_response_error(req, 1025, "Invalid GuideRateDeclination", cid, stx);
        return ESP_OK;
    }
    free(val);

    if (rate < 0.0f || rate > 1.0f) {
        alpaca_response_error(req, 1025, "GuideRateDeclination out of range (0-1 deg/s)", cid, stx);
        return ESP_OK;
    }

    alpaca_bridge_set_guide_rate_dec(rate);
    alpaca_response_ok(req, cid, stx);
    return ESP_OK;
}
