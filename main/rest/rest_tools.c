#include "rest.h"

#include <string.h>

#include "utils/utils.h"

/*
 * Axis string helpers — canonical conversion between MotorAxis and "ra"/"dec".
 */
static const char *rest_axis_to_string(MotorAxis axis) {
    switch (axis) {
        case MOTOR_AXIS_RA:
            return "ra";
        case MOTOR_AXIS_DEC:
            return "dec";
        default:
            return "unknown";
    }
}

MotorAxis rest_axis_from_string(const char *value) {
    if (value == NULL)
        return MOTOR_AXIS_UNKNOWN;
    if (strcmp(value, "ra") == 0)
        return MOTOR_AXIS_RA;
    if (strcmp(value, "dec") == 0)
        return MOTOR_AXIS_DEC;
    return MOTOR_AXIS_UNKNOWN;
}

const char *rest_axis_valid_values(void) {
    static char buffer[15];
    static bool initialized = false;

    if (initialized) {
        return buffer;
    }

    const char *values[MOTOR_AXIS_UNKNOWN];

    for (int i = 0; i < MOTOR_AXIS_UNKNOWN; i++) {
        values[i] = rest_axis_to_string((MotorAxis) i);
    }

    string_join(buffer, sizeof(buffer), values, MOTOR_AXIS_UNKNOWN, "|", "[", "]");
    initialized = true;

    return buffer;
}
