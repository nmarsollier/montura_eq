/* REST - rest_wifi_handler.c
 *
 * Purpose: configure the ESP-IDF Wi-Fi station credentials via REST API.
 */
#include "rest.h"

#include <string.h>

#include "esp_err.h"
#include "wifi/wifi.h"
#include "utils/utils.h"

/*
 * Business use case: allow clients to configure home Wi-Fi credentials.
 *
 * Objective: pass the requested SSID/password to the ESP-IDF Wi-Fi driver.
 * ESP-IDF persists that station configuration internally; Montura EQ does not
 * store a separate Wi-Fi config in its own NVS namespace.
 */
esp_err_t rest_wifi_handler(httpd_req_t *request) {
    HttpRequestBody body = http_request_read_body(request);
    JsonStringResult ssid = json_get_string(body.value, "ssid");
    JsonStringResult password = json_get_string(body.value, "password");

    if (!ssid.ok || strlen(ssid.value) == 0) {
        http_response_bad_request(request, "Missing or invalid 'ssid'");
        return ESP_OK;
    }

    if (!password.ok) {
        http_response_bad_request(request, "Missing or invalid 'password'");
        return ESP_OK;
    }

    esp_err_t result = wifi_configure_home_wifi(ssid.value, password.value);
    if (result != ESP_OK) {
        httpd_resp_set_status(request, "409 Conflict");
        http_response_json(request, "{\"ok\":false,\"message\":\"Failed to configure WiFi\"}");
        return ESP_OK;
    }

    http_response_json(request, "{\"ok\":true,\"message\":\"WiFi configured. Device is connecting.\"}");

    return ESP_OK;
}
