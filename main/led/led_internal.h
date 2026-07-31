#pragma once

#include "led.h"
#include "driver/ledc.h"

/* ── LEDC hardware configuration ──────────────────────────── */

#define LED_MODE        LEDC_LOW_SPEED_MODE
#define LED_TIMER       LEDC_TIMER_0
#define LED_CHANNEL     LEDC_CHANNEL_0
#define LED_GPIO        4
#define LED_FREQ_HZ     5000
#define LED_DUTY_RES    LEDC_TIMER_13_BIT
#define LED_DUTY_MAX    8191

/* ── Brightness levels (13-bit range: 0 – 8191) ──────────── */

#define LED_DIM_DUTY     820   /* ~10 % */
#define LED_BRIGHT_DUTY  8191  /* 100 % */

/* ── Timing ───────────────────────────────────────────────── */

#define LED_FADE_MS           1000  /* normal <-> slewing transition */
#define LED_BREATHE_SLOW_MS   1500  /* half-period of error breathing */
#define LED_HEARTBEAT_TICK_MS  300  /* heartbeat state-machine tick */

/* ── Breathing patterns ───────────────────────────────────── */

typedef enum {
    BREATHE_PATTERN_SMOOTH,    /* slow fade up/down — motor ERROR */
    BREATHE_PATTERN_HEARTBEAT  /* double-pulse, long pause — WiFi waiting */
} BreathePattern;

/* ── Shared state (defined in led_set_state.c) ────────────── */

extern LedState led_current_state;

/* ── Internal helpers ─────────────────────────────────────── */

/* Start a hardware fade to target_duty over time_ms. */
void led_start_fade(uint32_t target_duty, uint32_t time_ms);

/* Start / stop the breathing animation with the given pattern. */
void led_breathe_start(BreathePattern pattern);
void led_breathe_stop(void);

/*
 * Internal: apply a state transition (dim / bright / breathing).
 * Only called from led_update() — not part of the public API.
 */
void led_set_state(LedState state);
