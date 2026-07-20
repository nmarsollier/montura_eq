#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * LED states for the external indicator on GPIO 4.
 *
 * NORMAL  — dim brightness, mount idle.
 * SLEWING — full brightness, mount in motion.
 * ERROR   — breathing animation, permanent except for wifi recovery.
 */
typedef enum {
    LED_STATE_NORMAL,
    LED_STATE_SLEWING,
    LED_STATE_ERROR
} LedState;

/* Initialise LEDC PWM on GPIO 4 and start in NORMAL (dim). */
void led_init(void);

/*
 * Request a state change.
 *
 * NORMAL <-> SLEWING transitions use a smooth 1-second hardware fade.
 * Once ERROR is set the LED ignores further set_state calls.
 */
void led_set_state(LedState state);

/*
 * Clear ERROR and return to NORMAL with a fade.
 * Called when wifi recovers after a connection failure.
 * UART errors are never cleared (no code path calls this for them).
 */
void led_clear_error(void);

/*
 * Periodic update, call every ~50 ms from the runtime loop.
 * Reads mount status to switch between NORMAL and SLEWING when not in ERROR.
 */
void led_update(void);
