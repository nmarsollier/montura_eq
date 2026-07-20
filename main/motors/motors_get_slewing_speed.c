/* Motors - motors_get_slewing_speed.c
 *
 * Purpose: return the slewing angular speed for a speed rate profile.
 */
#include "motors.h"

/*
 * Map a speed_rate profile (1..4, higher = faster) to the
 * axis angular speed in degrees per second.
 */
float motors_get_slewing_speed(int speed_rate) {
    switch (speed_rate) {
        case 1: return 2.0f;
        case 2: return 8.0f;
        case 3: return 16.0f;
        default: return 24.0f;
    }
}
