#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"

/* Alpaca — Method — Park
 *
 * Purpose: Moves the mount to the park position and sets AtPark = true.
 *
 * Alpaca usage: Called by N.I.N.A. at session end or during safety sequences.
 */
esp_err_t alpaca_park_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    MountResult result = mount_park();
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
