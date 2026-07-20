/* LED — led_breathe_cb.c
 *
 * Purpose: breathing animation for ERROR state.
 *
 * Uses a periodic esp_timer that fires every LED_BREATHE_MS.
 * Each tick toggles the fade direction: dim -> bright -> dim -> ...
 * The 1.5-second hardware fade gives a smooth sinusoidal-like
 * breathing effect with zero CPU overhead during the fade ramp.
 */
#include "led_internal.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LED_BREATHE";

static esp_timer_handle_t s_breathe_timer;
static bool               s_breathe_up;   /* true = currently fading towards bright */

static void breathe_tick(void *arg) {
    (void) arg;

    if (s_breathe_up) {
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_DIM_DUTY, LED_BREATHE_MS,
                                     LEDC_FADE_NO_WAIT);
        s_breathe_up = false;
    } else {
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_BRIGHT_DUTY, LED_BREATHE_MS,
                                     LEDC_FADE_NO_WAIT);
        s_breathe_up = true;
    }
}

void led_breathe_start(void) {
    if (s_breathe_timer) {
        return;   /* already running */
    }

    s_breathe_up = true;

    /* Start immediately: fade to bright so the first visible effect is a rise. */
    ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                 LED_BRIGHT_DUTY, LED_BREATHE_MS,
                                 LEDC_FADE_NO_WAIT);

    esp_timer_create_args_t args = {
        .callback = breathe_tick,
        .arg      = NULL,
        .name     = "led_breathe",
    };
    esp_timer_create(&args, &s_breathe_timer);

    /* First timer tick after LED_BREATHE_MS to reverse direction. */
    esp_timer_start_periodic(s_breathe_timer, LED_BREATHE_MS * 1000UL);

    ESP_LOGI(TAG, "breathing started (period=%u ms)", LED_BREATHE_MS);
}

void led_breathe_stop(void) {
    if (!s_breathe_timer) {
        return;
    }

    esp_timer_stop(s_breathe_timer);
    esp_timer_delete(s_breathe_timer);
    s_breathe_timer = NULL;

    ESP_LOGI(TAG, "breathing stopped");
}
