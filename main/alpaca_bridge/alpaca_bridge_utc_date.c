/* Alpaca bridge — UTC date
 *
 * Read the current UTC date as an ISO 8601 string, or set the system
 * clock from an ISO 8601 string.
 */

#include "alpaca_bridge.h"

#include <stdio.h>
#include <time.h>

#include "mount.h"

/*
 * Current UTC date as an ISO 8601 string ("2025-06-05T20:15:00").
 * The returned pointer refers to a static buffer — copy it if needed
 * across multiple calls.
 */
const char *alpaca_bridge_get_utc_date(void) {
    static char buf[64];
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
             utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday,
             utc->tm_hour, utc->tm_min, utc->tm_sec);
    return buf;
}

/*
 * Set the system clock from an ISO 8601 string.
 * Delegates to mount_set_system_time().
 */
MountResult alpaca_bridge_set_utc_date(const char *iso_time) {
    return mount_set_system_time(iso_time);
}
