/* Alpaca bridge — slew settle time
 *
 * Time (seconds) the mount should wait after a slew completes before
 * reporting Slewing = false.  Currently stored but not enforced by
 * the motion task — clients may poll slewing until it clears.
 */

#include "alpaca_bridge.h"
#include "alpaca_bridge_internal.h"

#include "mount.h"
#include "mount_internal.h"

int alpaca_bridge_get_slew_settle_time(void) {
    return alpaca_bridge_state.slew_settle_time;
}

MountResult alpaca_bridge_set_slew_settle_time(int seconds) {
    alpaca_bridge_state.slew_settle_time = seconds;
    return mount_result_ok();
}
