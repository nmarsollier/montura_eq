#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"

/* Alpaca — Method — Unpark
 *
 * Purpose: Takes the mount out of the parked state, ready for operation.
 *
 * Alpaca usage: Called by N.I.N.A. at session start after connecting.
 */
esp_err_t alpaca_unpark_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    MountResult response = mount_unpark();
    if (response.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, response.message, cid, stx);
    return ESP_OK;
}
