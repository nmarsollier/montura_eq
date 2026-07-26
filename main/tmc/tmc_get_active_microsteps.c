/* TMC - tmc_get_active_microsteps.c
 *
 * Purpose: store the verified microstep cache and the initialisation flag
 * used by tmc2209_is_initialized() for LED error signalling.
 *
 * s_active_microsteps is written by tmc_init.c after successful
 * hardware write + read-back verification.  A non-zero value means
 * both axes were configured and verified.
 */

#include "tmc.h"
#include "tmc_internal.h"

/* Verified microstep cache — 0 until tmc2209_hw_init() succeeds. */
static uint16_t s_active_microsteps = 0;

void tmc2209_set_active_microsteps(uint16_t microsteps) {
    s_active_microsteps = microsteps;
}

bool tmc2209_is_initialized(void) {
    return s_active_microsteps != 0;
}
