/* tmc_internal.h — internal API shared among TMC module files
 *
 * Functions and variables exposed here are for use ONLY within the
 * main/tmc/ module. No file outside tmc/ should include this header.
 */

#pragma once

#include <stdint.h>

/*
 * Update the cached active microstep count.
 * Called by tmc_set_microsteps() after verifying the hardware latch.
 */
void tmc2209_set_active_microsteps(uint16_t microsteps);
