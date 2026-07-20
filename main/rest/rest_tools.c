#include "rest.h"

#include <string.h>

#include "utils.h"


/* Axis string helpers live here so REST and UI layers use the same
 * canonical values as the motors module.
 */
const char *motors_axis_to_string(MotorAxis axis) {
    switch (axis) {
        case MOTOR_AXIS_RA:
            return "ra";
        case MOTOR_AXIS_DEC:
            return "dec";
        default:
            return "unknown";
    }
}

/* Canonical string-to-axis mapping shared by the rest of the codebase. */
MotorAxis motors_axis_from_string(const char *value) {
    if (value == NULL)
        return MOTOR_AXIS_UNKNOWN;
    if (strcmp(value, "ra") == 0)
        return MOTOR_AXIS_RA;
    if (strcmp(value, "dec") == 0)
        return MOTOR_AXIS_DEC;
    return MOTOR_AXIS_UNKNOWN;
}

const char *motors_axis_valid_values(void) {
    static char buffer[15];
    static bool initialized = false;

    if (initialized) {
        return buffer;
    }

    const char *values[MOTOR_AXIS_UNKNOWN];

    for (int i = 0; i < MOTOR_AXIS_UNKNOWN; i++) {
        values[i] = motors_axis_to_string((MotorAxis) i);
    }

    string_join(buffer, sizeof(buffer), values, MOTOR_AXIS_UNKNOWN, "|", "[", "]");
    initialized = true;

    return buffer;
}
