/* Motors - motors_get_slewing_speed.c
 *
 * Purpose: return the slewing angular speed for a speed rate profile.
 */
#include "motors.h"
#include "motors_internal.h"

/*
 * Map a speed_rate profile (1..4, higher = faster) to the
 * axis angular speed in degrees per second.
 */
float motors_get_slewing_speed(int speed_rate) {
    switch (speed_rate) {
        case 1: return 1.0f;
        case 2: return 3.0f;
        case 3: return 6.0f;
        default: return MOTORS_MAX_SLEW_SPEED_DPS;
    }
}
