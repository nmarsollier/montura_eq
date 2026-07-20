/* Rest - rest_api_server.c
 *
 * Purpose: create the REST API HTTP server on port 80.
 */
#include "rest.h"
#include "utils.h"

#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "REST_API_SERVER";

/*
 * Business use case: publish the device's REST surface.
 *
 * Objective: start a dedicated httpd instance on port 80 and register all
 * REST API routes for the UI and integration clients.
 */
void rest_server_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 18;
    config.max_open_sockets = 4;
    config.lru_purge_enable = true;
    config.ctrl_port = 32768;

    esp_err_t result = httpd_start(&server, &config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start REST server: %s", esp_err_to_name(result));
        return;
    }

    rest_register_get(server, "/api/status", rest_status_handler);
    /* Serve the embedded SPA at the root path `/`. */
    rest_register_get(server, "/", rest_html_handler);

    rest_register_post(server, "/api/tracking", rest_tracking_handler);
    rest_register_post(server, "/api/move-axis", rest_move_axis_handler);
    rest_register_post(server, "/api/move-axis-speed", rest_move_axis_speed_handler);
    rest_register_post(server, "/api/slew-to-coordinates", rest_slew_to_coordinates_handler);
    rest_register_post(server, "/api/stop", rest_stop_handler);
    rest_register_post(server, "/api/park", rest_park_handler);
    rest_register_post(server, "/api/home", rest_home_handler);
    rest_register_post(server, "/api/unpark", rest_unpark_handler);
    rest_register_post(server, "/api/settings", rest_settings_handler);
    rest_register_post(server, "/api/wifi-config", rest_wifi_handler);

    ESP_LOGI(TAG, "REST server started on port 80");
}
