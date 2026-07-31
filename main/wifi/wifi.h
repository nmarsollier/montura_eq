#pragma once

#include <stdbool.h>

#include "esp_err.h"

void wifi_start(void);

void wifi_sntp_start(void);

extern bool wifi_started;
extern int  wifi_retry_count;

#define WIFI_SETUP_AP_SSID "NRM-WO"
#define WIFI_SETUP_AP_PASSWORD ""
#define WIFI_SETUP_AP_CHANNEL 1
#define WIFI_SETUP_AP_MAX_CONNECTIONS 4

#define WIFI_MAX_RETRY_COUNT 10
#define WIFI_CONNECT_TIMEOUT_MS 15000

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1

bool wifi_is_setup_ap_started(void);

esp_err_t wifi_configure_home_wifi(const char *ssid, const char *password);
