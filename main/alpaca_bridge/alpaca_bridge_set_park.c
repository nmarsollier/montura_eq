/* Alpaca bridge — set park position
 *
 * Stores the current axis position as the park position.  The actual
 * park/unpark motion is handled by mount_park() / mount_unpark().
 * This just records "where we are right now" as the park target.
 */

#include "alpaca_bridge.h"
#include "alpaca_bridge_internal.h"

#include "mount.h"
#include "mount_internal.h"
#include "motors/motors.h"

MountResult alpaca_bridge_set_park(void) {
    MotorsState s = motors_current_state();
    alpaca_bridge_state.park_ra_deg = s.ra_position;
    alpaca_bridge_state.park_dec_deg = s.dec_position;
    return mount_result_ok();
}
