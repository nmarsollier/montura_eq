#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "mount.h"

/* ═══════════════════════════════════════════════════════════════
 * Alpaca Bridge — public API for the Alpaca REST layer
 *
 * This module bridges Alpaca protocol concepts (target coordinates,
 * pier side, drive rates, settle time, alt/az, sidereal time) to the
 * mount and motors modules.  It is the only place that calls motors
 * or mount-internal functions on behalf of the Alpaca REST layer.
 * ═══════════════════════════════════════════════════════════════ */

/* Horizontal coordinates (calculated from equatorial + time + site). */
float alpaca_bridge_get_altitude(void);

float alpaca_bridge_get_azimuth(void);

/* Sidereal time in hours at the current site and UTC time. */
float alpaca_bridge_get_sidereal_time(void);

/* UTC date as ISO 8601 string ("2025-06-05T20:15:00"). */
const char *alpaca_bridge_get_utc_date(void);

MountResult alpaca_bridge_set_utc_date(const char *iso_time);

/* State queries. */
bool alpaca_bridge_get_is_slewing(void);

bool alpaca_bridge_get_is_tracking(void);

bool alpaca_bridge_get_is_parked(void);

bool alpaca_bridge_get_is_home(void);

/* Site location (degrees, metres). */
float alpaca_bridge_get_site_latitude(void);

float alpaca_bridge_get_site_longitude(void);

int alpaca_bridge_get_site_elevation(void);

MountResult alpaca_bridge_set_site_latitude(float lat);

MountResult alpaca_bridge_set_site_longitude(float lon);

MountResult alpaca_bridge_set_site_elevation(int elevation);

/* Tracking rate as Alpaca DriveRates enum (0=sidereal, 1=lunar, 2=solar). */
int alpaca_bridge_get_tracking_rate(void);

MountResult alpaca_bridge_set_tracking_rate(int rate);

bool alpaca_bridge_get_tracking(void);

MountResult alpaca_bridge_set_tracking(bool enabled);

void alpaca_bridge_get_tracking_rate_name(int idx, char *buf, size_t len);

/* Slew in equatorial coordinates. */
MountResult alpaca_bridge_slew_to_coordinates(float ra_hours, float dec_deg);

MountResult alpaca_bridge_abort_slew(void);

/* Target coordinates (stored for slewtotarget / synctotarget). */
float alpaca_bridge_get_target_ra(void);

float alpaca_bridge_get_target_dec(void);

void alpaca_bridge_set_target_ra(float ra_hours);

void alpaca_bridge_set_target_dec(float dec_deg);

MountResult alpaca_bridge_slew_to_target(void);

/* Pier side (0=East, 1=West). */
int alpaca_bridge_get_side_of_pier(void);

int alpaca_bridge_get_destination_side_of_pier(float ra_hours, float dec_deg);

MountResult alpaca_bridge_set_side_of_pier(int side);

/* Slew settle time (seconds). */
int alpaca_bridge_get_slew_settle_time(void);

MountResult alpaca_bridge_set_slew_settle_time(int seconds);

/* Park position. */
MountResult alpaca_bridge_set_park(void);
