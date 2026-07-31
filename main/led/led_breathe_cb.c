/* LED — led_breathe_cb.c
 *
 * Purpose: breathing / heartbeat animations for ERROR and WIFI_WAIT states.
 *
 * Two patterns:
 *
 *   SMOOTH (motor ERROR) — slow sinusoidal-like breathing.
 *     Timer fires every LED_BREATHE_SLOW_MS (1500 ms).
 *     Each tick toggles the fade direction: dim → bright → dim → …
 *     The 1500 ms hardware fade gives a smooth, ominous pulse.
 *
 *   HEARTBEAT (WiFi waiting) — double-pulse with long pause.
 *     Timer fires every LED_HEARTBEAT_TICK_MS (300 ms).
 *     State machine of 8 ticks:
 *       0 → fade to bright   (300 ms)
 *       1 → fade to dim      (300 ms)
 *       2 → fade to bright   (300 ms)   ─┐ double pulse
 *       3 → fade to dim      (300 ms)   ─┘
 *       4-7 → idle at dim    (1200 ms pause)
 *     Visual: ✨__✨__········✨__✨__········
 *     The double-pulse says "alive, but waiting" — clearly distinct
 *     from the slow ERROR breathing.
 */

#include "led_internal.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LED_BREATHE";

static esp_timer_handle_t s_breathe_timer;
static BreathePattern      s_pattern;
static bool                 s_breathe_up;   /* SMOOTH: current fade direction */
static int                  s_beat_step;    /* HEARTBEAT: 0-7 state counter */

/*
 * SMOOTH pattern — simple direction toggle.
 */
static void breathe_tick_smooth(void) {
    if (s_breathe_up) {
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_DIM_DUTY, LED_BREATHE_SLOW_MS,
                                     LEDC_FADE_NO_WAIT);
        s_breathe_up = false;
    } else {
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_BRIGHT_DUTY, LED_BREATHE_SLOW_MS,
                                     LEDC_FADE_NO_WAIT);
        s_breathe_up = true;
    }
}

/*
 * HEARTBEAT pattern — 8-step state machine.
 *   Step 0,2: fade to bright (300 ms)
 *   Step 1,3: fade to dim    (300 ms)
 *   Step 4-7: idle at dim    (300 ms each, 1200 ms total pause)
 */
static void breathe_tick_heartbeat(void) {
    switch (s_beat_step) {
    case 0:
    case 2:
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_BRIGHT_DUTY, LED_HEARTBEAT_TICK_MS,
                                     LEDC_FADE_NO_WAIT);
        break;
    case 1:
    case 3:
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_DIM_DUTY, LED_HEARTBEAT_TICK_MS,
                                     LEDC_FADE_NO_WAIT);
        break;
    default:
        /* Steps 4-7: pause — no fade, just stay at dim. */
        break;
    }
    s_beat_step = (s_beat_step + 1) % 8;
}

static void breathe_tick(void *arg) {
    (void) arg;

    if (s_pattern == BREATHE_PATTERN_HEARTBEAT) {
        breathe_tick_heartbeat();
    } else {
        breathe_tick_smooth();
    }
}

void led_breathe_start(BreathePattern pattern) {
    if (s_breathe_timer) {
        /* Already running with the same pattern — nothing to do. */
        if (s_pattern == pattern) {
            return;
        }
        /* Different pattern — restart. */
        led_breathe_stop();
    }

    s_pattern = pattern;

    uint32_t period_us;
    const char *pattern_name;

    if (pattern == BREATHE_PATTERN_HEARTBEAT) {
        s_beat_step = 0;
        period_us = LED_HEARTBEAT_TICK_MS * 1000UL;
        pattern_name = "heartbeat";

        /* Start with the first pulse — fade to bright immediately. */
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_BRIGHT_DUTY, LED_HEARTBEAT_TICK_MS,
                                     LEDC_FADE_NO_WAIT);
    } else {
        s_breathe_up = true;
        period_us = LED_BREATHE_SLOW_MS * 1000UL;
        pattern_name = "smooth";

        /* Start immediately: fade to bright so the first visible effect is a rise. */
        ledc_set_fade_time_and_start(LED_MODE, LED_CHANNEL,
                                     LED_BRIGHT_DUTY, LED_BREATHE_SLOW_MS,
                                     LEDC_FADE_NO_WAIT);
    }

    esp_timer_create_args_t args = {
        .callback = breathe_tick,
        .arg      = NULL,
        .name     = "led_breathe",
    };
    esp_timer_create(&args, &s_breathe_timer);
    esp_timer_start_periodic(s_breathe_timer, period_us);

    ESP_LOGI(TAG, "%s started (period=%lu us)", pattern_name, (unsigned long) period_us);
}

void led_breathe_stop(void) {
    if (!s_breathe_timer) {
        return;
    }

    esp_timer_stop(s_breathe_timer);
    esp_timer_delete(s_breathe_timer);
    s_breathe_timer = NULL;

    ESP_LOGI(TAG, "stopped");
}
