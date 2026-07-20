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

#define LED_FADE_MS      1000  /* normal <-> slewing transition */
#define LED_BREATHE_MS   1500  /* half-period of error breathing */

/* ── Shared state (defined in led_set_state.c) ────────────── */

extern LedState led_current_state;

/* ── Internal helpers ─────────────────────────────────────── */

/* Start a hardware fade to target_duty over time_ms. */
void led_start_fade(uint32_t target_duty, uint32_t time_ms);

/* Start / stop the breathing timer (only for ERROR state). */
void led_breathe_start(void);
void led_breathe_stop(void);
