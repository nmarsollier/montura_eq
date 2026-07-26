/*
 * tmc.h — TMC2209 driver public API.
 *
 * This module is the SINGLE source of truth for microstep configuration.
 * All other layers MUST reference TMC_TARGET_MICROSTEPS rather than
 * defining their own constants.
 */

#ifndef TMC2209_HW_H
#define TMC2209_HW_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------------
 * Microstep configuration — single source of truth for the entire system.
 *
 * All other layers (motors, motion) MUST reference this value rather than
 * defining their own constants.
 * -------------------------------------------------------------------------- */
#define TMC_TARGET_MICROSTEPS  32

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/*
 * Initialize UART, configure GCONF/IHOLD_IRUN/CHOPCONF on both RA and DEC
 * axes, and verify that the hardware latched the requested microsteps.
 *
 * Must be called before any step/direction motion is started.
 * Called internally from motors_hw_init().
 */
esp_err_t tmc2209_hw_init(void);

/*
 * Query whether the TMC2209 UART and both axes were initialised
 * successfully.  Used for error-state LED signalling.
 */
bool tmc2209_is_initialized(void);

#endif
