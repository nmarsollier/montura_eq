/* Alpaca bridge — current pier side
 *
 * ASCOM convention: 0 = pierEast, 1 = pierWest.
 *
 * With this mount's mechanical model, the pier side is reliably encoded
 * in the sign of dec_axis (DEC is linear, not normalised):
 *
 *   dec_axis >= 0  ->  pierEast  (0)
 *   dec_axis <  0  ->  pierWest  (1)
 */

#include "alpaca_bridge.h"

#include "mount.h"

int alpaca_bridge_get_side_of_pier(void) {
    MotorsState s = motors_current_state();
    return (s.dec_position >= 0.0f) ? 0 : 1;
}

MountResult alpaca_bridge_set_side_of_pier(int side) {
    (void) side;
    return (MountResult){.ok = true, .message = ""};
}
