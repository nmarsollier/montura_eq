/* TMC - tmc_get_active_microsteps.c
 *
 * Purpose: provide the rest of the system with the verified microstep count
 * from the TMC2209 driver, without requiring a UART transaction on every read.
 *
 * The single static variable s_active_microsteps is owned by this file and
 * updated by tmc_set_microsteps() (in tmc_init.c) after each successful
 * hardware write + read-back verification.
 *
 * tmc2209_is_initialized() reuses the same cache: a non-zero value means
 * both axes were configured and verified successfully.
 */

#include "tmc.h"
#include "tmc_internal.h"

/* --------------------------------------------------------------------------
 * Active microsteps cache — single source of truth for the rest of the system.
 * -------------------------------------------------------------------------- */

/*
 * The verified microstep count from the most recent successful hardware
 * write + read-back. Initialized to 0 (meaning "not yet configured").
 * Updated by tmc_set_microsteps() after successful verification.
 *
 * Both axes are configured identically, so a single value suffices.
 */
static uint16_t s_active_microsteps = 0;

uint16_t tmc2209_get_active_microsteps(void) {
    return s_active_microsteps;
}

void tmc2209_set_active_microsteps(uint16_t microsteps) {
    s_active_microsteps = microsteps;
}

bool tmc2209_is_initialized(void) {
    return s_active_microsteps != 0;
}
