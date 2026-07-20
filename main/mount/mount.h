#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "motors/motors.h"

/* Use `MotorAxis` from the motors package for mount axis operations. */

typedef struct {
    float lat;
    float lon;
    int elevation;
} MountSettings;

/* MountResult lives here because mount is the API boundary. */
typedef struct {
    bool ok;
    const char *message;
} MountResult;

/* Coordinate types. */
typedef struct {
    float ra_hours;
    float dec_deg;
} EquatorialCoordinates;

typedef struct {
    float ra_axis_deg;
    float dec_axis_deg;
    int pier_side;      /* 0 = pierEast, 1 = pierWest — set by equatorial_to_axis */
} AxisCoordinates;

/* Formatted astronomical coordinate types. */
typedef struct {
    int hours;
    int minutes;
    float seconds;
} RaHMS;

typedef struct {
    int sign; /* +1 or -1 */
    int degrees;
    int minutes;
    float seconds;
} DecDMS;

typedef struct {
    MotorsStatus status;
    TrackingMode tracking;
    RaHMS ra;
    DecDMS dec;
    float lst_hours;    /* Local sidereal time (hours) */
    int pier_side;      /* 0 = pierEast (normal), 1 = pierWest (flipped) */
    MountSettings settings;
} VisibleStatusData;

/* Conversion functions between equatorial and physical axis coordinates. */
bool equatorial_to_axis(EquatorialCoordinates eq, AxisCoordinates current,
                        AxisCoordinates *out);

EquatorialCoordinates axis_to_equatorial(AxisCoordinates axis);

/* Convert decimal RA hours to HMS and DEC degrees to DMS. */
RaHMS ra_hours_to_hms(float ra_hours);

DecDMS dec_deg_to_dms(float dec_deg);

/* Public API */

/*
 * mount_init
 * ----------------
 * Initialize mount module state. Prepares internal structures so the mount
 * can accept high-level commands (slew-to-coordinates, tracking, park/unpark). This is a
 * hardware-agnostic initialization point; it does not perform physical moves.
 */
void mount_init(void);

/*
 * mount_get_visible_status_data
 * -----------------------------
 * Return the current status view that the REST/API layer and UI consume.
 * The structure contains motors-derived status, current tracking mode, last
 * known RA/DEC (in axis-space), timestamp and persisted settings.
 */
VisibleStatusData mount_get_visible_status(void);

/*
 * mount_set_tracking
 * ------------------
 * Set the mount's tracking mode (sidereal, lunar, solar, manual, none).
 * Parameter: `tracking` - desired TrackingMode. Returns a MountResult that
 * indicates whether the request was accepted and includes a short message.
 */
MountResult mount_set_tracking(TrackingMode tracking);

/*
 * mount_slew_to_coordinates
 * -------------------------
 * Start an asynchronous slew to the requested equatorial coordinates.
 * Parameters:
 *  - `ra` (hours): right ascension in hours.
 *  - `dec` (degrees): declination in degrees.
 *  - `speed_rate` (1..4): requested speed profile (higher = faster).
 * Returns a MountResult describing acceptance or rejection.
 */
MountResult mount_slew_to_coordinates(float ra, float dec, int speed_rate);

/*
 * mount_stop
 * ----------
 * Stop any ongoing motion immediately and leave the mount in the READY
 * state. Returns a MountResult indicating success or reason for failure.
 */
MountResult mount_stop(void);

/*
 * mount_park
 * ----------
 * Move the mount to its defined parking position and mark it as PARKED.
 * This is a high-level operation (may be asynchronous); the result reports
 * whether the request was accepted.
 */
MountResult mount_park(void);

/*
 * mount_unpark
 * ------------
 * Take the mount out of the parked state and prepare it for motion. Returns
 * a MountResult describing the outcome.
 */
MountResult mount_unpark(void);

/*
 * mount_update_settings
 * ---------------------
 * Persist and apply user-provided mount settings (location and elevation).
 * Parameter: `settings` - settings structure with latitude, longitude and
 * elevation. Returns success/failure.
 */
MountResult mount_settings_update(MountSettings settings);

MountResult mount_set_system_time(const char *iso_time);

/*
 * Move the mount to its home position.
 */
MountResult mount_home(void);

/*
 * mount_move_axis
 * ---------------
 * Request a small relative move on a single physical axis. Parameters:
 *  - `axis`: which axis to move (RA or DEC).
 *  - `degrees`: delta in degrees to move (positive/negative allowed).
 *  - `speed`: requested speed profile (module may enforce limits).
 * This is the public single-axis move API and returns acceptance info.
 */
MountResult mount_move_axis_ra(float degrees, int speed_rate);

MountResult mount_move_axis_dec(float degrees, int speed_rate);

/*
 * mount_move_axis_speed
 * ------------------------
 * Move one or both axes continuously at the given rates in deg/s until
 * a subsequent call with both rates = 0 (or STOP / PARK) halts motion.
 * Positive = forward, negative = reverse.  Used by Alpaca MoveAxis and
 * manual centering controls.
 */
MountResult mount_set_move_axis_speed(float ra_speed, float dec_speed);

/* Discard saved tracking state when MoveAxis is aborted externally. */
void mount_move_axis_reset(void);

/* Convert RA from HMS struct to decimal hours. */
float mount_get_ra_hours(void);

/* Convert DEC from DMS struct to decimal degrees. */
float mount_get_dec_deg(void);

/* Compute the current Local Sidereal Time for the configured site. */
float mount_get_lst(void);

TrackingMode tracking_from_string(const char *value);
