/* Main - main.c
 *
 * Purpose: start the runtime subsystems, HTTP server, and main loop.
 */
#include "esp_log.h"
#include "rest.h"
#include "rest_alpaca.h"

#include "udp_alpaca.h"
#include "runtime.h"

static const char *TAG = "MAIN";

void app_main(void) {
    setup_init();

    rest_server_start();
    rest_alpaca_server_start();
    udp_alpaca_start();

    setup_runtime_start();

    ESP_LOGI(TAG, "Started");
}
