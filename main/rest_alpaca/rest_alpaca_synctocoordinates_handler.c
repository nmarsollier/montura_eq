#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"

/* Alpaca — Method — SyncToCoordinates
 *
 * Purpose: Returns NotImplemented — sync is not supported by this mount.
 *
 * Alpaca usage: Returns error 1024 (NotImplemented).
 */
esp_err_t alpaca_synctocoordinates_handler(httpd_req_t *req) {
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();
    alpaca_response_error(req, 1024, "Sync not supported by this mount",
                          cid, stx);
    return ESP_OK;
}
