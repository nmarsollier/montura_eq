/* Wifi - wifi_is_setup_ap_started.c
 *
 * Purpose: query whether the setup AP is currently active.
 */
#include "wifi.h"
#include "wifi_internal.h"

bool wifi_is_setup_ap_started(void) {
    return setup_ap_started;
}
