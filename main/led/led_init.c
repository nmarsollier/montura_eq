/* LED — led_init.c
 *
 * Purpose: initialise LEDC PWM on GPIO 4 for brightness control.
 *
 * Configures timer 0 at 5 kHz, 13-bit resolution (8192 steps),
 * binds channel 0 to GPIO 4, and starts with the dim (NORMAL) duty.
 */
#include "led_internal.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "LED_INIT";

void led_init(void) {
    /* ── Timer ─────────────────────────────────────────── */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LED_MODE,
        .duty_resolution = LED_DUTY_RES,
        .timer_num       = LED_TIMER,
        .freq_hz         = LED_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
        .deconfigure     = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* ── Channel ────────────────────────────────────────── */
    ledc_channel_config_t chan_conf = {
        .gpio_num       = LED_GPIO,
        .speed_mode     = LED_MODE,
        .channel        = LED_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LED_TIMER,
        .duty           = LED_DIM_DUTY,
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags          = { .output_invert = 0 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&chan_conf));

    /* ── Fade service (needed for ledc_set_fade_time_and_start) ── */
    ESP_ERROR_CHECK(ledc_fade_func_install(0));

    ESP_LOGI(TAG, "LEDC PWM ready on GPIO %d, dim=%u, bright=%u",
             LED_GPIO, LED_DIM_DUTY, LED_BRIGHT_DUTY);
}
