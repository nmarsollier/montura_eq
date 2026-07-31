#pragma once

#include <esp_http_server.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* ═══════════════════════════════════════════════════════════════
 * Configuration
 * ═══════════════════════════════════════════════════════════════ */

/* ─── Alpaca server identity ─── */
#define ALPACA_SERVER_NAME        "NRM-WO"
#define ALPACA_SERVER_DESCRIPTION "NRM-WO \xe2\x80\x94 Ecuatorial Mount Controller"
#define ALPACA_DRIVER_INFO        "NRM-WO Alpaca Driver v1.0"
#define ALPACA_DRIVER_VERSION     "1.0.0"
#define ALPACA_INTERFACE_VERSION  3

/* ─── Network ─── */
#define ALPACA_DEFAULT_PORT       11111
#define ALPACA_MAX_URI_HANDLERS   64

/* ─── Telescope device ─── */
#define ALPACA_DEVICE_NAME        "NRM-WO"
#define ALPACA_DEVICE_TYPE        "Telescope"
#define ALPACA_DEVICE_NUMBER      0
#define ALPACA_UNIQUE_ID          "nrm-wo-telescope-001"

/* ─── Physical constants (mock values for optional features) ─── */
#define ALPACA_APERTURE_AREA      0.0113f   /* m^2  (120mm aperture) */
#define ALPACA_APERTURE_DIAMETER  0.120f    /* m    (120mm) */
#define ALPACA_FOCAL_LENGTH       0.900f    /* m    (900mm) */

/* ═══════════════════════════════════════════════════════════════
 * Response helpers
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Send a successful Alpaca response with a JSON value.
 * JSON format: {"Value":<value>,"ClientTransactionID":<cid>,
 *               "ServerTransactionID":<stx>,"ErrorNumber":0,"ErrorMessage":""}
 */
void alpaca_response_value(httpd_req_t *req, const char *value_json,
                           uint32_t client_id, uint32_t server_tx);

/*
 * Send a successful Alpaca response with no value (void).
 */
void alpaca_response_ok(httpd_req_t *req,
                        uint32_t client_id, uint32_t server_tx);

/*
 * Send an Alpaca error response.
 * error_number is one of the ASCOM Alpaca error codes.
 */
void alpaca_response_error(httpd_req_t *req, int error_number,
                           const char *message,
                           uint32_t client_id, uint32_t server_tx);

/* ═══════════════════════════════════════════════════════════════
 * Parameter parsing
 * ═══════════════════════════════════════════════════════════════ */

/*
 * Extract the ClientID from the request query string.
 * Returns 0 if not present or unparseable.
 */
uint32_t alpaca_get_client_id(httpd_req_t *req);

/*
 * Read and parse the request body as URL-encoded form data.
 * Returns the value for the given key, or NULL if not found.
 * Caller must free the returned string.
 */
char *alpaca_get_form_param(httpd_req_t *req, const char *key);

/*
 * Parse a form parameter as a float. Returns true on success.
 */
bool alpaca_get_form_float(httpd_req_t *req, const char *key, float *out);

/*
 * Parse a form parameter as a bool ("true"/"false"). Returns true on success.
 */
bool alpaca_get_form_bool(httpd_req_t *req, const char *key, bool *out);

/*
 * Parse a form parameter as an int. Returns true on success.
 */
bool alpaca_get_form_int(httpd_req_t *req, const char *key, int *out);

/*
 * Get the next server transaction ID (monotonically increasing).
 */
uint32_t alpaca_next_server_tx(void);

/*
 * Read the request body once into the shared buffer.
 * MUST be called before any alpaca_get_form_*() calls in the same handler.
 */
void alpaca_read_body(httpd_req_t *req);

/* Diagnostic: return the shared body buffer. */
const char *alpaca_dump_body(httpd_req_t *req, int *out_len);

/*
 * Reset the stored MoveAxis rates to zero — called when a STOP is issued.
 */
void alpaca_moveaxis_reset(void);

