/* LED — led_update.c
 *
 * Purpose: periodic LED state sync called from the runtime loop.
 *
 * Reads the mount status to switch between NORMAL and SLEWING
 * when the LED is not in the permanent ERROR state.
 * Breathing animation for ERROR is driven by an esp_timer,
 * so this function does nothing extra for that state.
 */
#include "led_internal.h"

#include "motors.h"

void led_update(void) {
    if (led_current_state == LED_STATE_ERROR) {
        return;
    }

    MotorsState ms = motors_current_state();

    if (ms.status == MOTORS_STATUS_SLEWING) {
        led_set_state(LED_STATE_SLEWING);
    } else {
        led_set_state(LED_STATE_NORMAL);
    }
}
