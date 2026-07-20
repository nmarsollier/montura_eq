/* Tools - rest_tools.c
 *
 * Shared HTTP route registration helpers used by both the REST API and
 * Alpaca servers. Avoids duplicating register_get / register_post / register_put
 * across server modules.
 */
#include "utils.h"
#include "rest.h"

#include <esp_log.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "REST_TOOLS";

void rest_register_get(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *)) {
    httpd_uri_t route = {
        .uri = uri,
        .method = HTTP_GET,
        .handler = handler,
        .user_ctx = NULL,
    };
    esp_err_t err = httpd_register_uri_handler(server, &route);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        ESP_LOGW(TAG, "GET  %s: %s", uri, esp_err_to_name(err));
    }
}

void rest_register_post(httpd_handle_t server, const char *uri,
                        esp_err_t (*handler)(httpd_req_t *)) {
    httpd_uri_t route = {
        .uri = uri,
        .method = HTTP_POST,
        .handler = handler,
        .user_ctx = NULL,
    };
    esp_err_t err = httpd_register_uri_handler(server, &route);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        ESP_LOGW(TAG, "POST %s: %s", uri, esp_err_to_name(err));
    }
}

void rest_register_put(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *)) {
    httpd_uri_t route = {
        .uri = uri,
        .method = HTTP_PUT,
        .handler = handler,
        .user_ctx = NULL,
    };
    esp_err_t err = httpd_register_uri_handler(server, &route);
    if (err != ESP_OK && err != ESP_ERR_HTTPD_HANDLER_EXISTS) {
        ESP_LOGW(TAG, "PUT  %s: %s", uri, esp_err_to_name(err));
    }
}

/*
 * Business use case: unify REST command responses.
 *
 * Objective: keep the `ok` + `message` contract consistent and map failures
 * to HTTP 409 for clients.
 */
void rest_send_result(httpd_req_t *request, MountResult result) {
    static const char format[] = "{\"ok\":%s,\"message\":\"%s\"}";
    char response[strlen(result.message) + sizeof(format) + 1];

    snprintf(
        response,
        sizeof(response),
        format,
        result.ok ? "true" : "false",
        result.message);

    if (!result.ok) {
        httpd_resp_set_status(request, "409 Conflict");
    }

    http_response_json(request, response);
}

