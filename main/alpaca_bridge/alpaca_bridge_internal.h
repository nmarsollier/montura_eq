#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════
 * Alpaca Bridge — shared mutable state
 *
 * Holds transient values that Alpaca clients can read and write
 * (target coordinates, pier side, slew settle time, park position).
 * These do not need NVS persistence — they reset on reboot.
 * ═══════════════════════════════════════════════════════════════ */

typedef struct {
    float target_ra; /* Target right ascension (hours) */
    float target_dec; /* Target declination (degrees) */
    int slew_settle_time; /* Settle time after slew (seconds) */
    float park_ra_deg; /* Park position — RA axis (degrees) */
    float park_dec_deg; /* Park position — DEC axis (degrees) */
} AlpacaBridgeState;

extern AlpacaBridgeState alpaca_bridge_state;

/* ═══════════════════════════════════════════════════════════════
 * Astronomical helpers (shared by altitude, azimuth, sidereal time)
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Convert a Unix timestamp to Julian Date.
 */
double alpaca_bridge_unix_to_jd(time_t t);

/*
 * Compute Greenwich Mean Sidereal Time (GMST) in hours from a Julian Date.
 * Uses the standard USNO approximation valid to ~1 arcsecond.
 */
double alpaca_bridge_gmst_hours(double jd);
