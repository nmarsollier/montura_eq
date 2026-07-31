/* Motors - motors_limits.c
 *
 * Purpose: load, persist, and update operational axis limits.
 *
 * Limits are stored in NVS under the "mount" namespace alongside site
 * settings.  On first boot (no NVS) they default to the factory values
 * below.  The UI can override them at runtime; changes are committed
 * immediately and survive reboot.
 */
#include "motors.h"
#include "motors_internal.h"

#include "nvs.h"
#include "utils/utils.h"

#include "esp_log.h"

static const char *TAG = "MOTORS_LIMITS";
static const char *NVS_NS = "mount";

#define LIMITS_DEFAULT_RA_MIN  -100.0f
#define LIMITS_DEFAULT_RA_MAX   100.0f
#define LIMITS_DEFAULT_DEC_MIN -150.0f
#define LIMITS_DEFAULT_DEC_MAX  150.0f

/* ── NVS persistence ─────────────────────────────────────── */

void motors_limits_load(void) {
    motors_state.limits.ra_min  = LIMITS_DEFAULT_RA_MIN;
    motors_state.limits.ra_max  = LIMITS_DEFAULT_RA_MAX;
    motors_state.limits.dec_min = LIMITS_DEFAULT_DEC_MIN;
    motors_state.limits.dec_max = LIMITS_DEFAULT_DEC_MAX;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS not found, using factory limits");
        return;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed, using factory limits: %s", esp_err_to_name(err));
        return;
    }

    uint32_t v;
    if (nvs_get_u32(h, "ra_min", &v) == ESP_OK)  motors_state.limits.ra_min  = uint32_to_float(v);
    if (nvs_get_u32(h, "ra_max", &v) == ESP_OK)  motors_state.limits.ra_max  = uint32_to_float(v);
    if (nvs_get_u32(h, "dec_min", &v) == ESP_OK) motors_state.limits.dec_min = uint32_to_float(v);
    if (nvs_get_u32(h, "dec_max", &v) == ESP_OK) motors_state.limits.dec_max = uint32_to_float(v);

    nvs_close(h);
    ESP_LOGI(TAG, "Limits loaded: RA [%.1f, %.1f]  DEC [%.1f, %.1f]",
             motors_state.limits.ra_min, motors_state.limits.ra_max,
             motors_state.limits.dec_min, motors_state.limits.dec_max);
}

void motors_limits_save(void) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open for write failed: %s", esp_err_to_name(err));
        return;
    }

    nvs_set_u32(h, "ra_min",  float_to_uint32(motors_state.limits.ra_min));
    nvs_set_u32(h, "ra_max",  float_to_uint32(motors_state.limits.ra_max));
    nvs_set_u32(h, "dec_min", float_to_uint32(motors_state.limits.dec_min));
    nvs_set_u32(h, "dec_max", float_to_uint32(motors_state.limits.dec_max));

    err = nvs_commit(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
    }
    nvs_close(h);

    ESP_LOGI(TAG, "Limits saved: RA [%.1f, %.1f]  DEC [%.1f, %.1f]",
             motors_state.limits.ra_min, motors_state.limits.ra_max,
             motors_state.limits.dec_min, motors_state.limits.dec_max);
}

/* ── Runtime setters ─────────────────────────────────────── */

void motors_set_current_as_home(void) {
    motors_state.ra_steps  = 0;
    motors_state.dec_steps = 0;
    motors_state.status    = MOTORS_STATUS_READY;
    motors_limits_save();
    ESP_LOGI(TAG, "Home set at current position");
}

void motors_set_current_as_ra_left_limit(void) {
    motors_state.limits.ra_min = motors_get_ra_deg();
    motors_limits_save();
    ESP_LOGI(TAG, "RA left limit set to %.3f", motors_state.limits.ra_min);
}

void motors_set_current_as_ra_right_limit(void) {
    motors_state.limits.ra_max = motors_get_ra_deg();
    motors_limits_save();
    ESP_LOGI(TAG, "RA right limit set to %.3f", motors_state.limits.ra_max);
}

void motors_set_current_as_dec_left_limit(void) {
    motors_state.limits.dec_min = motors_get_dec_deg();
    motors_limits_save();
    ESP_LOGI(TAG, "DEC left limit set to %.3f", motors_state.limits.dec_min);
}

void motors_set_current_as_dec_right_limit(void) {
    motors_state.limits.dec_max = motors_get_dec_deg();
    motors_limits_save();
    ESP_LOGI(TAG, "DEC right limit set to %.3f", motors_state.limits.dec_max);
}

void motors_limits_reset(void) {
    motors_state.limits.ra_min  = LIMITS_DEFAULT_RA_MIN;
    motors_state.limits.ra_max  = LIMITS_DEFAULT_RA_MAX;
    motors_state.limits.dec_min = LIMITS_DEFAULT_DEC_MIN;
    motors_state.limits.dec_max = LIMITS_DEFAULT_DEC_MAX;
    motors_limits_save();
    ESP_LOGI(TAG, "Limits reset to factory defaults");
}
