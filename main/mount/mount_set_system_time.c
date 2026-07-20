/* Mount - mount_time.c
 *
 * Purpose: parse time values and update the system clock.
 */
#include "mount.h"
#include "mount_internal.h"

#include "esp_log.h"
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "MOUNT_SET_SYSTEM_TIME";

static bool parse_iso8601_to_time(const char *s, time_t *out) {
    if (s == NULL || out == NULL)
        return false;

    int Y, M, D, h, mi, sec;
    if (sscanf(s, "%4d-%2d-%2dT%2d:%2d:%2d", &Y, &M, &D, &h, &mi, &sec) != 6) {
        return false;
    }

    int off_sign = 0, off_h = 0, off_m = 0;
    const char *tzpos = strchr(s, 'Z');
    if (tzpos == NULL) {
        const char *tpos = strchr(s, 'T');
        const char *p = tpos;
        const char *tzsign = NULL;
        while (p && *p) {
            if (*p == '+' || *p == '-')
                tzsign = p;
            p++;
        }
        if (tzsign) {
            off_sign = (*tzsign == '-') ? -1 : 1;
            if (sscanf(tzsign + 1, "%2d:%2d", &off_h, &off_m) < 1) {
                return false;
            }
        }
    }

    struct tm tm = {0};
    tm.tm_year = Y - 1900;
    tm.tm_mon = M - 1;
    tm.tm_mday = D;
    tm.tm_hour = h;
    tm.tm_min = mi;
    tm.tm_sec = sec;

    time_t t;
#if defined(__GLIBC__) || defined(_GNU_SOURCE) || defined(__USE_GNU)
    extern time_t timegm(struct tm *tm);
    t = timegm(&tm);
#else
    char old_tz_buf[128] = {0};
    char *old_tz = getenv("TZ");
    if (old_tz) {
        strncpy(old_tz_buf, old_tz, sizeof(old_tz_buf) - 1);
    }
    setenv("TZ", "UTC0", 1);
    tzset();
    t = mktime(&tm);
    if (old_tz) {
        setenv("TZ", old_tz_buf, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
#endif

    int offset_seconds = off_sign * (off_h * 3600 + off_m * 60);
    t -= offset_seconds;

    *out = t;
    return true;
}

MountResult mount_set_system_time(const char *iso_time) {
    if (iso_time == NULL || iso_time[0] == '\0') {
        return mount_result_error("Missing time");
    }

    time_t new_time = 0;
    if (!parse_iso8601_to_time(iso_time, &new_time)) {
        return mount_result_error("Invalid time format");
    }

    struct timeval tv = {.tv_sec = new_time, .tv_usec = 0};
    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGW(TAG, "Failed to set system time");
        return mount_result_error("Failed to set system time");
    }
    return mount_result_ok();
}
