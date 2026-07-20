/* Alpaca bridge — slew / abort
 *
 * Thin wrappers that map Alpaca coordinate calls directly to the
 * mount public API. RA is in hours, DEC is in degrees.
 */

#include "alpaca_bridge.h"

#include "mount.h"

MountResult alpaca_bridge_slew_to_coordinates(float ra_hours, float dec_deg) {
    return mount_slew_to_coordinates(ra_hours, dec_deg, 4);
}

MountResult alpaca_bridge_abort_slew(void) {
    return mount_stop();
}
