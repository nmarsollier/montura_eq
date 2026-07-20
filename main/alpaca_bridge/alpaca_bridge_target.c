/* Alpaca bridge — target coordinates
 *
 * Stored in AlpacaBridgeState. The Alpaca protocol separates "set target"
 * from "slew to target" — clients set TargetRightAscension /
 * TargetDeclination first, then call slewtotarget / synctotarget.
 */

#include "alpaca_bridge.h"
#include "alpaca_bridge_internal.h"

#include "mount.h"
#include "mount_internal.h"

float alpaca_bridge_get_target_ra(void) { return alpaca_bridge_state.target_ra; }
float alpaca_bridge_get_target_dec(void) { return alpaca_bridge_state.target_dec; }

void alpaca_bridge_set_target_ra(float ra_hours) { alpaca_bridge_state.target_ra = ra_hours; }
void alpaca_bridge_set_target_dec(float dec_deg) { alpaca_bridge_state.target_dec = dec_deg; }

MountResult alpaca_bridge_slew_to_target(void) {
    if (alpaca_bridge_state.target_ra == 0.0f && alpaca_bridge_state.target_dec == 0.0f)
        return mount_result_error("Target not set");
    return mount_slew_to_coordinates(alpaca_bridge_state.target_ra, alpaca_bridge_state.target_dec, 2);
}
