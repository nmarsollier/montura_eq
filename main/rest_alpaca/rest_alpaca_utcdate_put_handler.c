#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "alpaca_bridge.h"
#include <stdlib.h>

/* Alpaca — Property — UTCDate (PUT)
 *
 * Purpose: Sets the mount system clock from an ISO 8601 string.
 *
 * Alpaca usage: N.I.N.A. syncs the mount clock to the PC clock on connect.
 */
esp_err_t alpaca_utcdate_put_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    char *val = alpaca_get_form_param(req, "UTCDate");
    if (!val) {
        alpaca_response_error(req, 1025, "Missing UTCDate", cid, stx);
        return ESP_OK;
    }
    MountResult response = alpaca_bridge_set_utc_date(val);
    free(val);
    if (response.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, response.message, cid, stx);
    return ESP_OK;
}
