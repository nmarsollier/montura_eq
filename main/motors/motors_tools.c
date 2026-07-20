/* Motors - motors_tools.c
 *
 * Purpose: motors enum/string helpers and state utilities.
 */
#include "motors.h"
#include "motors_internal.h"
#include "utils/utils.h"
#include <string.h>

/*
 * Status string helpers live here so the motors module remains the canonical
 * source for mount state names.
 */
const char *status_to_string(MotorsStatus status) {
    switch (status) {
        case MOTORS_STATUS_READY:
            return "ready";
        case MOTORS_STATUS_TRACKING:
            return "tracking";
        case MOTORS_STATUS_DISABLED:
            return "disabled";
        case MOTORS_STATUS_SLEWING:
            return "slewing";
        case MOTORS_STATUS_PARKED:
            return "parked";
        default:
            return "disabled";
    }
}

/*
 * Tracking string helpers live here so REST and UI layers use the same
 * canonical values as the motors module.
 */
const char *tracking_to_string(TrackingMode tracking) {
    switch (tracking) {
        case TRACKING_NONE:
            return "none";
        case TRACKING_SIDEREAL:
            return "sidereal";
        case TRACKING_LUNAR:
            return "lunar";
        case TRACKING_SOLAR:
            return "solar";
        default:
            return "none";
    }
}

TrackingMode tracking_from_string(const char *value) {
    if (value == NULL) {
        return TRACKING_NONE;
    }

    if (strcmp(value, "none") == 0) {
        return TRACKING_NONE;
    }

    if (strcmp(value, "sidereal") == 0) {
        return TRACKING_SIDEREAL;
    }

    if (strcmp(value, "lunar") == 0) {
        return TRACKING_LUNAR;
    }

    if (strcmp(value, "solar") == 0) {
        return TRACKING_SOLAR;
    }

    return TRACKING_NONE;
}

const char *tracking_valid_values(void) {
    static char buffer[35];
    static bool initialized = false;

    if (initialized) {
        return buffer;
    }

    const char *values[4];
    values[0] = tracking_to_string(TRACKING_NONE);
    values[1] = tracking_to_string(TRACKING_SIDEREAL);
    values[2] = tracking_to_string(TRACKING_LUNAR);
    values[3] = tracking_to_string(TRACKING_SOLAR);

    string_join(buffer, sizeof(buffer), values, 4, "|", "[", "]");
    initialized = true;

    return buffer;
}

/*
 * Validate axis positions against the configured mechanical limits.
 */
bool motors_is_valid_ra(float value) {
    return value >= motors_state.limits.ra_min && value <= motors_state.limits.ra_max;
}

bool motors_is_valid_dec(float value) {
    return value >= motors_state.limits.dec_min && value <= motors_state.limits.dec_max;
}
