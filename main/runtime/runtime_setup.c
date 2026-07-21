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
#include "tmc.h"
#include "usb_net.h"

static const char *TAG = "RUNTIME_SETUP";

/*
 * Business use case: prepare the mount for operation.
 *
 * Objective: bring network, core services, and peripherals online so the
 * mount starts in a usable state.
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
     * WiFi error: setup AP running means home wifi didn't connect.
     * Recoverable — a later successful connection calls led_clear_error().
     */
    if (wifi_is_setup_ap_started()) {
        led_set_state(LED_STATE_ERROR);
    }

    /*
     * USB Ethernet (ECM/RNDIS) — non-fatal, mount works without USB.
     * Blocking call with 10 s timeout for host enumeration.
     */
    esp_err_t usb_result = usb_net_init();
    if (usb_result != ESP_OK) {
        ESP_LOGW(TAG, "USB net init skipped: %s", esp_err_to_name(usb_result));
    }

    mount_init();

    motors_init();

    /*
     * UART / TMC error: permanent in practice because no code path
     * calls led_clear_error() for it — only a reboot resets it.
     */
    if (!tmc2209_is_initialized()) {
        led_set_state(LED_STATE_ERROR);
    }

    ESP_LOGI(TAG, "Mount ready");
}
