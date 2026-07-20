/* Network - network.c
 *
 * Purpose: initialize Wi-Fi for the physical ESP32-S3 board.
 *
 * The device uses ESP-IDF Wi-Fi driver storage for home network credentials.
 * Montura EQ does not store a separate SSID/password in its own NVS namespace.
 *
 * Boot behavior:
 * - If the ESP-IDF Wi-Fi driver already has a station SSID, try to connect.
 * - If no station SSID exists, start the setup AP.
 * - If station connection fails, start the setup AP as fallback.
 *
 * Runtime behavior:
 * - REST can call network_configure_home_wifi() to set the station credentials.
 * - ESP-IDF persists those credentials internally.
 */
#include "network.h"
#include "led.h"

#include <string.h>

#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

static const char *TAG = "NETWORK";

/* Definitions for symbols exported from network.h */
EventGroupHandle_t wifi_event_group;
int wifi_retry_count;
bool setup_ap_started;
bool wifi_started;
char network_ip[17];

static void log_ap_started(void) {
    ESP_LOGW(TAG, "Setup AP started");
    ESP_LOGW(TAG, "SSID: %s", WIFI_SETUP_AP_SSID);
    ESP_LOGW(TAG, "Password: %s", WIFI_SETUP_AP_PASSWORD);
    ESP_LOGW(TAG, "Open: http://192.168.4.1");
}

static bool has_stored_home_wifi(void) {
    wifi_config_t config = {0};
    esp_err_t result = esp_wifi_get_config(WIFI_IF_STA, &config);

    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Could not read stored Wi-Fi STA config: %s", esp_err_to_name(result));
        return false;
    }

    return strlen((const char *) config.sta.ssid) > 0;
}

static void start_setup_ap(void) {
    if (setup_ap_started) {
        return;
    }

    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_SETUP_AP_SSID,
            .ssid_len = strlen(WIFI_SETUP_AP_SSID),
            .password = WIFI_SETUP_AP_PASSWORD,
            .channel = WIFI_SETUP_AP_CHANNEL,
            .max_connection = WIFI_SETUP_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    if (strlen(WIFI_SETUP_AP_PASSWORD) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_mode_t mode = WIFI_MODE_NULL;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));

    if (mode == WIFI_MODE_STA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    } else if (mode != WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));

    if (!wifi_started) {
        ESP_ERROR_CHECK(esp_wifi_start());
        wifi_started = true;
    }

    setup_ap_started = true;
    log_ap_started();
}

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
) {
    (void) arg;
    (void) event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Starting Wi-Fi station connection");
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retry_count < WIFI_MAX_RETRY_COUNT) {
            wifi_retry_count++;
            ESP_LOGW(TAG, "Home Wi-Fi disconnected, retrying %d/%d", wifi_retry_count, WIFI_MAX_RETRY_COUNT);
            esp_wifi_connect();
            return;
        }

        ESP_LOGE(TAG, "Could not connect to home Wi-Fi");
        xEventGroupSetBits(wifi_event_group, WIFI_FAILED_BIT);
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Client connected to setup AP");
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Client disconnected from setup AP");
    }
}

static void ip_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
) {
    (void) arg;
    (void) event_base;

    if (event_id != IP_EVENT_STA_GOT_IP) {
        return;
    }

    const ip_event_got_ip_t *event = (const ip_event_got_ip_t *) event_data;

    wifi_retry_count = 0;
    setup_ap_started = false;

    wifi_config_t config = {0};
    esp_err_t result = esp_wifi_get_config(WIFI_IF_STA, &config);

    ESP_LOGI(TAG, "Connected to home Wi-Fi");
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "SSID: %s", (const char *) config.sta.ssid);
    }
    ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "Open: http://" IPSTR, IP2STR(&event->ip_info.ip));

    sprintf(network_ip, IPSTR, IP2STR(&event->ip_info.ip));

    /* Sync system clock via SNTP on first successful connection. */
    {
        static bool sntp_started = false;
        if (!sntp_started) {
            sntp_started = true;
            network_sntp_start();
        }
    }

    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

    /* If the LED was stuck in wifi-error breathing, clear it now. */
    led_clear_error();
}

static void start_home_wifi(void) {
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    if (!wifi_started) {
        ESP_ERROR_CHECK(esp_wifi_start());
        wifi_started = true;
    } else {
        ESP_ERROR_CHECK(esp_wifi_connect());
    }

    ESP_LOGI(TAG, "Connecting to stored home Wi-Fi");
}

static void start_wifi_driver(void) {
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    /*
     * Keep ESP-IDF Wi-Fi storage enabled so station credentials configured by
     * esp_wifi_set_config(WIFI_IF_STA, ...) survive reboot.
     */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &ip_event_handler,
        NULL,
        NULL));
}

void network_start(void) {
    wifi_event_group = xEventGroupCreate();
    wifi_retry_count = 0;
    setup_ap_started = false;
    wifi_started = false;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    start_wifi_driver();

    if (!has_stored_home_wifi()) {
        ESP_LOGW(TAG, "Home Wi-Fi not configured in ESP-IDF Wi-Fi storage; starting setup AP");
        start_setup_ap();
        return;
    }

    start_home_wifi();

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        /* Disable modem sleep so UDP listeners don't drop packets. */
        esp_wifi_set_ps(WIFI_PS_NONE);
        ESP_LOGI(TAG, "Home Wi-Fi ready");
        return;
    }

    ESP_LOGW(TAG, "Home Wi-Fi unavailable; falling back to setup AP");
    start_setup_ap();
}
