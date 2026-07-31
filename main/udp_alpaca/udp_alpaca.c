/* Udp-alpaca - udp_alpaca.c
 *
 * Purpose: ASCOM Alpaca Discovery Protocol — UDP listener on port 32227.
 *
 * Protocol spec: ASCOM Alpaca Discovery API v1.
 * Magic string "alpacadiscovery1" → JSON response with AlpacaPort.
 */

#include "udp_alpaca.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "UDP_ALPACA";

#define ALPACA_DISCOVERY_PORT      32227
#define DISCOVERY_TASK_STACK_WORDS 2048
#define DISCOVERY_TASK_PRIORITY    2

static void udp_alpaca_task(void *arg) {
    (void) arg;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family      = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port        = htons(ALPACA_DISCOVERY_PORT);

    if (bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) < 0) {
        ESP_LOGE(TAG, "bind(:%d) failed: errno=%d", ALPACA_DISCOVERY_PORT, errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Listening on UDP %d", ALPACA_DISCOVERY_PORT);

    char buf[128];
    struct sockaddr_in sender_addr;

    static const char response[] =
        "{"
        "\"AlpacaPort\":11111,"
        "\"ServerName\":\"NRM-WO\","
        "\"Version\":\"v1\","
        "\"InterfaceVersion\":3"
        "}";

    while (true) {
        socklen_t sender_len = sizeof(sender_addr);

        ssize_t len = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                               (struct sockaddr *) &sender_addr, &sender_len);
        if (len < 0) {
            ESP_LOGW(TAG, "recvfrom() errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        buf[len] = '\0';

        ESP_LOGI(TAG, "rx %d bytes from %s:%d: \"%s\"",
                 (int) len,
                 inet_ntoa(sender_addr.sin_addr),
                 ntohs(sender_addr.sin_port),
                 buf);

        if (strstr(buf, "alpacadiscovery1") != NULL) {
            sendto(sock, response, strlen(response), 0,
                   (struct sockaddr *) &sender_addr, sender_len);
            ESP_LOGI(TAG, "discovery response sent");
        }
    }
}

/* ─── Public API ─── */

void udp_alpaca_start(void) {
    xTaskCreate(udp_alpaca_task, "alpaca_disc",
                DISCOVERY_TASK_STACK_WORDS, NULL,
                DISCOVERY_TASK_PRIORITY, NULL);
    ESP_LOGI(TAG, "Task created");
}
