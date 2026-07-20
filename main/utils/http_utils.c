/* Tools - http_response.c
 *
 * Purpose: send HTTP responses from REST handlers.
 */
#include "utils/utils.h"

#include <stdio.h>
#include <string.h>

void http_response_json(httpd_req_t *request, const char *json) {
    httpd_resp_set_type(request, "application/json");
    httpd_resp_sendstr(request, json);
}

void http_response_html(httpd_req_t *request, const char *html, unsigned int len) {
    httpd_resp_set_type(request, "text/html");
    httpd_resp_send(request, html, len);
}

void http_response_bad_request(httpd_req_t *request, const char *message) {
    static const char format[] = "{\"ok\":false,\"message\":\"%s\"}";
    char response[strlen(message) + sizeof(format) + 1];

    snprintf(
        response,
        sizeof(response),
        format,
        message);

    httpd_resp_set_status(request, "400 Bad Request");
    http_response_json(request, response);
}

HttpRequestBody http_request_read_body(httpd_req_t *request) {
    HttpRequestBody body = {
        .length = 0,
        .complete = false,
        .value = {0}
    };

    int total = 0;

    if (HTTP_RESPONSE_BODY_MAX_LENGTH <= 0) {
        return body;
    }

    while (total < request->content_len && total < HTTP_RESPONSE_BODY_MAX_LENGTH - 1) {
        int received = httpd_req_recv(request, body.value + total, HTTP_RESPONSE_BODY_MAX_LENGTH - 1 - total);

        if (received <= 0) {
            break;
        }

        total += received;
    }

    body.value[total] = '\0';
    body.length = total;
    body.complete = total >= request->content_len;

    return body;
}
