/* Runtime - runtime_setup.c
 *
 * Purpose: initialize the subsystems needed before the runtime loop starts.
 */
#include "runtime.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "led.h"
#include "motors.h"
#include "mount.h"
#include "wifi.h"
#include "usb_net.h"

static const char *TAG = "RUNTIME_SETUP";

/*
 * Business use case: prepare the mount for operation.
 *
 * Objective: bring network, core services, and peripherals online so the
 * mount starts in a usable state.
 *
 * LED state is managed exclusively by led_update() in the runtime loop —
 * no explicit led_set_state() calls are needed here.  The loop picks up
 * motor status and WiFi status on its first tick.
 */
void setup_init(void) {
    ESP_LOGI(TAG, "Setting up mount");

    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);

    wifi_start();

    led_init();

    /*
     * USB Ethernet (ECM/RNDIS) — non-fatal, mount works without USB.
     * Blocking call with 10 s timeout for host enumeration.
     */
    esp_err_t usb_result = usb_net_init();
    if (usb_result != ESP_OK) {
        ESP_LOGW(TAG, "USB net init skipped: %s", esp_err_to_name(usb_result));
    }

    mount_init();

    esp_err_t motors_err = motors_init();
    if (motors_err != ESP_OK) {
        ESP_LOGE(TAG, "motors_init failed: %s — mount in ERROR state, reboot required",
                 esp_err_to_name(motors_err));
    }

    ESP_LOGI(TAG, "Mount ready");
}
