#include "rest.h"

#include <stdio.h>
#include <time.h>

#include "mount.h"

#include "utils/utils.h"
#include "network/network.h"

/*
 * Business use case: expose the mount's current status via the API.
 *
 * Objective: provide a consistent operational snapshot for UI, monitoring,
 * and external automation.
 */
esp_err_t rest_status_handler(httpd_req_t *request) {
    VisibleStatusData data = mount_get_visible_status();

    /* Format current mount time as ISO 8601 for the UI. */
    time_t now = time(NULL);
    struct tm tm = {0};
    gmtime_r(&now, &tm);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", &tm);

    bool is_home = (data.status == MOTORS_STATUS_READY
                    && data.ra.hours == 0 && data.ra.minutes == 0
                    && data.dec.degrees == 0 && data.dec.minutes == 0);

    /* Format LST as HH:MM:SS.s from decimal hours. */
    int lst_h = (int) data.lst_hours;
    int lst_m = (int) ((data.lst_hours - (float) lst_h) * 60.0f);
    float lst_s = (data.lst_hours - (float) lst_h - (float) lst_m / 60.0f) * 3600.0f;
    if (lst_s < 0.0f) lst_s = 0.0f;

    const char *pier_str = (data.pier_side == 0) ? "East" : "West";

    static const char format[] =
            "{"
            "\"status\":\"%s\","
            "\"tracking\":\"%s\","
            "\"ra\":\"%02d:%02d:%05.2f\","
            "\"dec\":\"%c%02d:%02d:%05.2f\","
            "\"lst\":\"%02d:%02d:%05.2f\","
            "\"pier_side\":\"%s\","
            "\"time\":\"%s\","
            "\"settings\":{"
            "\"lat\":%.6f,"
            "\"lon\":%.6f,"
            "\"elevation\":%d"
            "},"
            "\"wifi_ap\":%s,"
            "\"is_home\":%s"
            "}";

    const char *status = status_to_string(data.status);
    const char *tracking = tracking_to_string(data.tracking);
    char dec_sign = data.dec.sign >= 0 ? '+' : '-';
    bool wifi_ap = network_is_setup_ap_started();

    /*
     * Fixed-size buffer — the JSON response with LST + pier_side fits in 640 bytes.
     */
    char response[640];
    snprintf(response, sizeof(response), format,
             status, tracking,
             data.ra.hours, data.ra.minutes, data.ra.seconds,
             dec_sign, data.dec.degrees, data.dec.minutes, data.dec.seconds,
             lst_h, lst_m, lst_s,
             pier_str,
             time_buf,
             data.settings.lat, data.settings.lon, data.settings.elevation,
             wifi_ap ? "true" : "false",
             is_home ? "true" : "false");

    http_response_json(request, response);

    return ESP_OK;
}
