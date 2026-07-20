/* LED — led_set_state.c
 *
 * Purpose: LED state machine — brightness transitions and error handling.
 *
 * NORMAL <-> SLEWING transitions use a 1-second hardware fade.
 * ERROR is sticky: once entered, further set_state calls are ignored.
 * ERROR can only be cleared by led_clear_error(), which is called
 * exclusively from the wifi IP handler — so UART errors are de-facto
 * permanent because nothing ever clears them.
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

/* ── Public API ────────────────────────────────────────────── */

void led_set_state(LedState state) {
    /* ERROR is sticky — ignore further transitions. */
    if (led_current_state == LED_STATE_ERROR) {
        return;
    }

    if (state == led_current_state) {
        return;
    }

    ESP_LOGI(TAG, "state: %d -> %d", led_current_state, state);
    led_current_state = state;

    switch (state) {
    case LED_STATE_NORMAL:
        apply_normal();
        break;
    case LED_STATE_SLEWING:
        apply_slewing();
        break;
    case LED_STATE_ERROR:
        led_breathe_stop();
        led_breathe_start();
        break;
    }
}

void led_clear_error(void) {
    if (led_current_state != LED_STATE_ERROR) {
        return;
    }

    ESP_LOGI(TAG, "clearing error -> NORMAL");
    led_breathe_stop();
    led_current_state = LED_STATE_NORMAL;
    apply_normal();
}
