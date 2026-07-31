/* LED — led_set_state.c
 *
 * Purpose: apply LED state transitions (dim / bright / breathing).
 *
 * This is an internal helper called exclusively from led_update().
 * Other modules must never call this directly — led_update() is the
 * single public entry point that decides the correct state.
 */
#include "led_internal.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "LED_STATE";

/* ── Shared state ──────────────────────────────────────────── */

LedState led_current_state = LED_STATE_NORMAL;

/* ── Helpers ───────────────────────────────────────────────── */

void led_start_fade(uint32_t target_duty, uint32_t time_ms) {
    esp_err_t err = ledc_set_fade_time_and_start(
        LED_MODE, LED_CHANNEL, target_duty, time_ms, LEDC_FADE_NO_WAIT);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "fade start failed: %s", esp_err_to_name(err));
    }
}

static void apply_normal(void) {
    led_start_fade(LED_DIM_DUTY, LED_FADE_MS);
}

static void apply_slewing(void) {
    led_start_fade(LED_BRIGHT_DUTY, LED_FADE_MS);
}

/* ── Internal API ──────────────────────────────────────────── */

void led_set_state(LedState state) {
    if (state == led_current_state) {
        return;
    }

    led_current_state = state;

    switch (state) {
    case LED_STATE_NORMAL:
        led_breathe_stop();
        apply_normal();
        break;
    case LED_STATE_SLEWING:
        led_breathe_stop();
        apply_slewing();
        break;
    case LED_STATE_ERROR:
        led_breathe_stop();
        led_breathe_start(BREATHE_PATTERN_SMOOTH);
        break;
    case LED_STATE_WIFI_WAIT:
        led_breathe_stop();
        led_breathe_start(BREATHE_PATTERN_HEARTBEAT);
        break;
    }
}
