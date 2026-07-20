/*
 * tmc.h — TMC2209 driver public API.
 *
 * This module is the SINGLE source of truth for microstep configuration.
 * All other layers (motors, motion, etc.) MUST query microstep values
 * through this API rather than defining their own constants.
 *
 * Initialization sequence (expected call order):
 *   1. tmc2209_hw_init()       — UART init, configure both axes
 *   2. tmc2209_get_microsteps() — query the active (verified) microstep count
 */

#ifndef TMC2209_HW_H
#define TMC2209_HW_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/*
 * Initialize UART, configure GCONF/IHOLD_IRUN/CHOPCONF on both RA and DEC
 * axes, and verify that the hardware latched the requested microsteps.
 *
 * Must be called before any step/direction motion is started.
 * Called internally from motors_motion_hw_init().
 */
esp_err_t tmc2209_hw_init(void);

/*
 * Get the confirmed active microstep count from the most recent init/set.
 *
 * This value is cached after tmc2209_hw_init() completes successfully,
 * so it always reflects the hardware state without needing a UART
 * transaction on every read.
 *
 * Both axes (RA and DEC) are configured identically, so this returns a
 * single value that applies to the entire mount.
 *
 * Returns 0 if the TMC module has not been initialized yet.
 */
uint16_t tmc2209_get_active_microsteps(void);

/*
 * Query whether the TMC2209 UART and both axes were initialised
 * successfully.  Used for error-state LED signalling.
 */
bool tmc2209_is_initialized(void);

#endif
