/* Alpaca — Method — PulseGuide
 *
 * Purpose: execute a guide pulse.  Direction is an integer per the
 * ASCOM Alpaca spec: 0=North, 1=South, 2=East, 3=West.
 * Falls back to string parsing for non-conforming clients.
 */
#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "mount.h"

#include <stdlib.h>
#include <string.h>

esp_err_t alpaca_pulseguide_handler(httpd_req_t *req) {
    alpaca_read_body(req);
    uint32_t cid = alpaca_get_client_id(req);
    uint32_t stx = alpaca_next_server_tx();

    int dir_int = -1;
    int duration = 0;
    bool got_dir = alpaca_get_form_int(req, "Direction", &dir_int);
    bool got_dur = alpaca_get_form_int(req, "Duration", &duration);

    /* Fallback: try string Direction (non-standard but tolerant). */
    if (got_dir && (dir_int < 0 || dir_int > 3)) {
        char *dir_str = alpaca_get_form_param(req, "Direction");
        if (dir_str) {
            if (strcasecmp(dir_str, "North") == 0)      dir_int = 0;
            else if (strcasecmp(dir_str, "South") == 0) dir_int = 1;
            else if (strcasecmp(dir_str, "East") == 0)  dir_int = 2;
            else if (strcasecmp(dir_str, "West") == 0)  dir_int = 3;
            free(dir_str);
        }
    }

    if (!got_dir || !got_dur || dir_int < 0 || dir_int > 3) {
        alpaca_response_error(req, 1025, "Missing or invalid Direction (0-3) or Duration", cid, stx);
        return ESP_OK;
    }
    if (duration <= 0 || duration > 30000) {
        alpaca_response_error(req, 1025, "Duration out of range (1-30000 ms)", cid, stx);
        return ESP_OK;
    }

    /* Map ASCOM integer to internal GuideDirection enum. */
    GuideDirection dir_map[] = {
        GUIDE_DIRECTION_NORTH, GUIDE_DIRECTION_SOUTH,
        GUIDE_DIRECTION_EAST,  GUIDE_DIRECTION_WEST,
    };

    MountResult result = mount_pulse_guide(dir_map[dir_int], (uint32_t)duration);
    if (result.ok) alpaca_response_ok(req, cid, stx);
    else alpaca_response_error(req, 1025, result.message, cid, stx);
    return ESP_OK;
}
