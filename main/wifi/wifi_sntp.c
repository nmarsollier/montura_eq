/* Wifi - wifi_sntp.c
 *
 * Purpose: synchronise the system clock via SNTP as soon as WiFi connects.
 *
 * The ESP-IDF SNTP client queries pool.ntp.org on the first successful
 * WiFi connection and periodically thereafter.  The system clock stays
 * in UTC — all local-time / LST conversions happen in the mount
 * coordinates module using the site longitude.
 *
 * This function is called from the IP_EVENT_STA_GOT_IP handler.  Because
 * the event handler runs in the WiFi task, we defer the actual SNTP work
 * to a short-lived background task so we never block the event loop.
 */

#include "wifi.h"

#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WIFI_SNTP";

/*
 * Multiple NTP pools for redundancy.  The ESP-IDF SNTP client cycles
 * through them, so if the first is unreachable the second takes over.
 */
#define SNTP_SERVER_1 "pool.ntp.org"
#define SNTP_SERVER_2 "time.google.com"
#define SNTP_SERVER_3 "time.cloudflare.com"

static void sntp_task(void *arg) {
    (void) arg;

    /*
     * Give the network stack (DNS, DHCP) a couple of seconds to fully
     * settle before attempting the first NTP query.
     */
    vTaskDelay(pdMS_TO_TICKS(2000));

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(3,
                                                                      ESP_SNTP_SERVER_LIST(SNTP_SERVER_1, SNTP_SERVER_2,
                                                                          SNTP_SERVER_3)
    );
    config.start = true;
    config.server_from_dhcp = false;

    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SNTP init failed: %s — time sync unavailable",
                 esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    esp_sntp_set_sync_interval(3600000); /* re-sync every hour (ms) */

    /*
     * Wait up to 30 s for the first sync.  The ESP-IDF SNTP client sends
     * a query every ~60 s by default after start, but the first one should
     * fire within a few seconds.  We poll quickly for responsiveness.
     */
    for (int retry = 0; retry < 300; retry++) {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            time_t now;
            time(&now);
            struct tm timeinfo;
            gmtime_r(&now, &timeinfo);
            ESP_LOGI(TAG, "SNTP sync completed — UTC: %04d-%02d-%02dT%02d:%02d:%02dZ",
                     timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                     timeinfo.tm_mday, timeinfo.tm_hour,
                     timeinfo.tm_min, timeinfo.tm_sec);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}

void wifi_sntp_start(void) {
    xTaskCreate(sntp_task, "sntp_sync", 3072, NULL, 1, NULL);
}
