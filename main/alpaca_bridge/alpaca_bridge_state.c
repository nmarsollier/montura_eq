/* Alpaca bridge — alpaca_bridge_state.c
 *
 * Shared mutable state for the Alpaca bridge layer.
 * Holds transient values that Alpaca clients can read and write.
 */

#include "alpaca_bridge_internal.h"

AlpacaBridgeState alpaca_bridge_state = {
    .target_ra = 0.0f,
    .target_dec = 0.0f,
    .slew_settle_time = 0,
    .park_ra_deg = 0.0f,
    .park_dec_deg = 90.0f,
};
