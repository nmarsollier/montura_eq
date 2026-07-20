/* Mount - mount_settings_storage.c
 *
 * Purpose: persist mount settings in NVS.
 */
#include "mount_internal.h"
#include "utils/utils.h"

#include "nvs.h"

#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MOUNT_SETTINGS_LOAD";
static const char *NVS_NAMESPACE = "mount";

static void mount_settings_storage_set_default_settings(MountSettings *settings) {
    settings->lat = -32.8908f;
    settings->lon = -68.8272f;
    settings->elevation = 750;
}

/*
 * Business use case: load persisted mount configuration.
 *
 * Objective: start with valid operational parameters, using saved values when
 * available and defaults otherwise.
 */
void mount_settings_load(MountSettings *settings) {
    if (settings == NULL) {
        ESP_LOGE(TAG, "Invalid output settings pointer");
        return;
    }

    mount_settings_storage_set_default_settings(settings);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS namespace not found, using default settings");
        return;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace, using defaults: %s", esp_err_to_name(err));
        return;
    }

    uint32_t lat_u32;
    err = nvs_get_u32(nvs_handle, "lat", &lat_u32);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load latitude from NVS, using default: %s", esp_err_to_name(err));
    } else {
        settings->lat = uint32_to_float(lat_u32);
    }

    uint32_t lon_u32;
    err = nvs_get_u32(nvs_handle, "lon", &lon_u32);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load longitude from NVS, using default: %s", esp_err_to_name(err));
    } else {
        settings->lon = uint32_to_float(lon_u32);
    }

    int32_t elevation_i32;
    err = nvs_get_i32(nvs_handle, "elevation", &elevation_i32);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load elevation from NVS, using default: %s", esp_err_to_name(err));
    } else {
        settings->elevation = (int) elevation_i32;
    }

    nvs_close(nvs_handle);
}
