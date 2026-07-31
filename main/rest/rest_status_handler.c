#include "rest.h"

#include <stdio.h>
#include <time.h>

#include "esp_timer.h"

#include "mount.h"
#include "motors/motors.h"
#include "tmc/tmc.h"

#include "utils/utils.h"
#include "wifi/wifi.h"

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
            "\"is_home\":%s,"
            "\"debug\":{"
            "\"ra_axis_deg\":%.6f,"
            "\"dec_axis_deg\":%.6f,"
            "\"ra_steps\":%lld,"
            "\"dec_steps\":%lld,"
            "\"ra_speed\":%.6f,"
            "\"dec_speed\":%.6f,"
            "\"guiding\":%s,"
            "\"tmc_ok\":%s,"
            "\"microsteps\":%u,"
            "\"limits\":{"
            "\"ra_min\":%.1f,"
            "\"ra_max\":%.1f,"
            "\"dec_min\":%.1f,"
            "\"dec_max\":%.1f"
            "},"
            "\"wifi_ip\":\"%s\","
            "\"uptime_s\":%lu"
            "}"
            "}";

    const char *status = motors_status_to_string(data.status);
    const char *tracking = motors_tracking_to_string(data.tracking);
    char dec_sign = data.dec.sign >= 0 ? '+' : '-';
    bool wifi_ap = wifi_is_setup_ap_started();

    /* Debug: raw motor state for diagnostics. */
    MotorsState ms = motors_current_state();

    /*
     * Fixed-size buffer — the JSON response with debug section fits in 1280 bytes.
     */
    char response[1280];
    snprintf(response, sizeof(response), format,
             status, tracking,
             data.ra.hours, data.ra.minutes, data.ra.seconds,
             dec_sign, data.dec.degrees, data.dec.minutes, data.dec.seconds,
             lst_h, lst_m, lst_s,
             pier_str,
             time_buf,
             data.settings.lat, data.settings.lon, data.settings.elevation,
             wifi_ap ? "true" : "false",
             is_home ? "true" : "false",
             /* debug */
             motors_get_ra_deg(), motors_get_dec_deg(),
             ms.ra_steps, ms.dec_steps,
             ms.ra_speed, ms.dec_speed,
             ms.guiding ? "true" : "false",
             tmc2209_is_initialized() ? "true" : "false",
             TMC_TARGET_MICROSTEPS,
             ms.limits.ra_min, ms.limits.ra_max,
             ms.limits.dec_min, ms.limits.dec_max,
             wifi_ip,
             (unsigned long) (esp_timer_get_time() / 1000000));

    http_response_json(request, response);

    return ESP_OK;
}
