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

static const char *TAG = "MOUNT_SETTINGS_SAVE";
static const char *NVS_NAMESPACE = "mount";

/*
 * Business use case: persist the current mount configuration.
 *
 * Objective: keep site settings across restarts without manual reconfiguration.
 */
void mount_settings_save(const MountSettings *settings) {
    if (settings == NULL) {
        ESP_LOGE(TAG, "Invalid settings pointer");
        return;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace for writing: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u32(nvs_handle, "lat", float_to_uint32(settings->lat));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save latitude to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_set_u32(nvs_handle, "lon", float_to_uint32(settings->lon));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save longitude to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_set_i32(nvs_handle, "elevation", (int32_t) settings->elevation);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save elevation to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS changes: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    nvs_close(nvs_handle);
}
