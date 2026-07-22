/* Alpaca bridge — alpaca_bridge_guide_rate.c
 *
 * Guide rate get/set — delegates to the mount layer which stores
 * the authoritative guide rate values (deg/s).
 */
#include "alpaca_bridge.h"
#include "mount.h"

float alpaca_bridge_get_guide_rate_ra(void) {
    return mount_get_guide_rate_ra();
}

float alpaca_bridge_get_guide_rate_dec(void) {
    return mount_get_guide_rate_dec();
}

void alpaca_bridge_set_guide_rate_ra(float rate_dps) {
    mount_set_guide_rate_ra(rate_dps);
}

void alpaca_bridge_set_guide_rate_dec(float rate_dps) {
    mount_set_guide_rate_dec(rate_dps);
}
