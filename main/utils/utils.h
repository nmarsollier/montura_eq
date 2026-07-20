#pragma once

#include <stddef.h>

#include "esp_http_server.h"

/* String utilities. */
bool string_join(char *buffer, size_t buffer_size, const char *const *items, size_t count, const char *separator,
                 const char *prefix, const char *suffix);

/* HTTP response helpers. */
#define HTTP_RESPONSE_BODY_MAX_LENGTH 512

typedef struct {
    int length;
    bool complete;
    char value[HTTP_RESPONSE_BODY_MAX_LENGTH];
} HttpRequestBody;

void http_response_json(httpd_req_t *request, const char *json);

void http_response_html(httpd_req_t *request, const char *html, unsigned int len);

void http_response_bad_request(httpd_req_t *request, const char *message);

HttpRequestBody http_request_read_body(httpd_req_t *request);

/* JSON utilities. */
#define JSON_STRING_RESULT_MAX_LENGTH 128

typedef struct {
    bool ok;
    char value[JSON_STRING_RESULT_MAX_LENGTH];
} JsonStringResult;

typedef struct {
    bool ok;
    float value;
} JsonFloatResult;

typedef struct {
    bool ok;
    int value;
} JsonIntResult;

JsonStringResult json_get_string(const char *json, const char *key);

JsonFloatResult json_get_float(const char *json, const char *key);

JsonIntResult json_get_int(const char *json, const char *key);

/* Number utilities. */
uint32_t float_to_uint32(float value);

float uint32_to_float(uint32_t value);

void rest_register_get(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *));

void rest_register_post(httpd_handle_t server, const char *uri,
                        esp_err_t (*handler)(httpd_req_t *));

void rest_register_put(httpd_handle_t server, const char *uri,
                       esp_err_t (*handler)(httpd_req_t *));
