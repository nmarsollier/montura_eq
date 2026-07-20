#include "rest_alpaca_internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "esp_http_server.h"
#include "esp_log.h"

#define ALPACA_RESPONSE_BUFFER 512

static uint32_t s_server_tx = 0;

uint32_t alpaca_next_server_tx(void) {
    return ++s_server_tx;
}

/* ─── Internal helper ─── */

static void alpaca_send_json(httpd_req_t *req, const char *json) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
}

/* ─── Public API ─── */

void alpaca_response_value(httpd_req_t *req, const char *value_json,
                           uint32_t client_id, uint32_t server_tx) {
    char buf[ALPACA_RESPONSE_BUFFER];
    snprintf(buf, sizeof(buf),
             "{\"Value\":%s,\"ClientTransactionID\":%lu,"
             "\"ServerTransactionID\":%lu,\"ErrorNumber\":0,"
             "\"ErrorMessage\":\"\"}",
             value_json, (unsigned long) client_id, (unsigned long) server_tx);
    alpaca_send_json(req, buf);
}

void alpaca_response_ok(httpd_req_t *req,
                        uint32_t client_id, uint32_t server_tx) {
    char buf[ALPACA_RESPONSE_BUFFER];
    snprintf(buf, sizeof(buf),
             "{\"ClientTransactionID\":%lu,"
             "\"ServerTransactionID\":%lu,\"ErrorNumber\":0,"
             "\"ErrorMessage\":\"\"}",
             (unsigned long) client_id, (unsigned long) server_tx);
    alpaca_send_json(req, buf);
}

void alpaca_response_error(httpd_req_t *req, int error_number,
                           const char *message,
                           uint32_t client_id, uint32_t server_tx) {
    char buf[ALPACA_RESPONSE_BUFFER];
    snprintf(buf, sizeof(buf),
             "{\"ClientTransactionID\":%lu,"
             "\"ServerTransactionID\":%lu,"
             "\"ErrorNumber\":%d,"
             "\"ErrorMessage\":\"%s\"}",
             (unsigned long) client_id, (unsigned long) server_tx,
             error_number, message);
    alpaca_send_json(req, buf);
}

/* ─── Parameter parsing ─── */

uint32_t alpaca_get_client_id(httpd_req_t *req) {
    char buf[32];
    esp_err_t err = httpd_req_get_url_query_str(req, buf, sizeof(buf));
    if (err != ESP_OK) return 0;

    char value[16];
    err = httpd_query_key_value(buf, "ClientID", value, sizeof(value));
    if (err != ESP_OK) {
        /* Also try lowercase */
        err = httpd_query_key_value(buf, "clientid", value, sizeof(value));
        if (err != ESP_OK) return 0;
    }
    return (uint32_t) atol(value);
}

/*
 * Request body buffer — read once per request by the handler.
 * Multiple form-param reads operate on this buffer, avoiding the
 * "body consumed" issue with httpd_req_recv.
 *
 * N.B. ESP-IDF reuses httpd_req_t objects so pointer-based caching
 * is NOT safe across requests — the handler MUST call alpaca_read_body
 * at the top of each handler that uses form params.
 */
static char  s_body_buf[512];
static int   s_body_len = 0;

/*
 * URL-decode a percent-encoded string in-place.
 * "2026-06-13T13%3A53%3A04" → "2026-06-13T13:53:04"
 * Returns the new (shorter) length.
 */
static size_t url_decode_inplace(char *s, size_t len) {
    size_t w = 0;
    for (size_t r = 0; r < len; r++, w++) {
        if (s[r] == '%' && r + 2 < len) {
            char hex[3] = {s[r + 1], s[r + 2], '\0'};
            char *end;
            long c = strtol(hex, &end, 16);
            if (end == hex + 2) {
                s[w] = (char) c;
                r += 2;
                continue;
            }
        }
        s[w] = s[r];
    }
    if (w < len) s[w] = '\0';
    return w;
}

void alpaca_read_body(httpd_req_t *req) {
    /* Only read the body if the client sent one — httpd_req_recv on a
     * request without Content-Length can block long enough to trigger
     * the task watchdog (SW_CPU_RESET). */
    char cl[16] = {0};
    bool has_body = httpd_req_get_hdr_value_str(req, "Content-Length", cl, sizeof(cl)) == ESP_OK
                    && atoi(cl) > 0;

    s_body_len = 0;
    s_body_buf[0] = '\0';
    if (has_body) {
        int ret = httpd_req_recv(req, s_body_buf, sizeof(s_body_buf) - 1);
        s_body_len = (ret > 0) ? ret : 0;
        s_body_buf[s_body_len] = '\0';
    }
}

const char *alpaca_dump_body(httpd_req_t *req, int *out_len) {
    (void) req;
    if (out_len) *out_len = s_body_len;
    return (s_body_len > 0) ? s_body_buf : "";
}

char *alpaca_get_form_param(httpd_req_t *req, const char *key) {
    (void) req;
    if (s_body_len <= 0) return NULL;

    size_t key_len = strlen(key);
    const char *p = s_body_buf;
    while (*p) {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            p += key_len + 1;
            const char *val_end = p;
            while (*val_end && *val_end != '&') val_end++;
            size_t vlen = val_end - p;
            char *value = malloc(vlen + 1);
            if (value) {
                memcpy(value, p, vlen);
                value[vlen] = '\0';
                /* URL-decode percent-encoded characters (e.g. %3A → :) */
                url_decode_inplace(value, vlen);
            }
            return value;
        }
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }
    return NULL;
}

/*
 * Try to read a parameter value from the query string, falling back to
 * the URL-encoded form body.  The Alpaca spec allows both; N.I.N.A. uses
 * the query string for MoveAxis and similar methods.
 */
static char *alpaca_get_param_str(httpd_req_t *req, const char *key) {
    /* 1. Query string */
    char qbuf[128];
    if (httpd_req_get_url_query_str(req, qbuf, sizeof(qbuf)) == ESP_OK) {
        char val[64];
        if (httpd_query_key_value(qbuf, key, val, sizeof(val)) == ESP_OK) {
            return strdup(val);
        }
    }

    /* 2. Form body */
    return alpaca_get_form_param(req, key);
}

bool alpaca_get_form_float(httpd_req_t *req, const char *key, float *out) {
    char *val = alpaca_get_param_str(req, key);
    if (!val) return false;
    *out = (float) atof(val);
    free(val);
    return true;
}

bool alpaca_get_form_bool(httpd_req_t *req, const char *key, bool *out) {
    char *val = alpaca_get_param_str(req, key);
    if (!val) return false;
    if (strcasecmp(val, "true") == 0 || strcmp(val, "1") == 0)
        *out = true;
    else
        *out = false;
    free(val);
    return true;
}

bool alpaca_get_form_int(httpd_req_t *req, const char *key, int *out) {
    char *val = alpaca_get_param_str(req, key);
    if (!val) return false;
    *out = atoi(val);
    free(val);
    return true;
}
