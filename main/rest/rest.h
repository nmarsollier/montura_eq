#pragma once

#include "esp_http_server.h"
#include "mount.h"

/*
 * Identifier for physical axes managed by the motors module.
 */
typedef enum {
    MOTOR_AXIS_RA,
    MOTOR_AXIS_DEC,
    MOTOR_AXIS_UNKNOWN
} MotorAxis;

void rest_server_start(void);

esp_err_t rest_status_handler(httpd_req_t *request);

esp_err_t rest_html_handler(httpd_req_t *request);

esp_err_t rest_tracking_handler(httpd_req_t *request);

esp_err_t rest_slew_to_coordinates_handler(httpd_req_t *request);

esp_err_t rest_move_axis_handler(httpd_req_t *request);
esp_err_t rest_move_axis_speed_handler(httpd_req_t *request);

esp_err_t rest_stop_handler(httpd_req_t *request);

esp_err_t rest_park_handler(httpd_req_t *request);

esp_err_t rest_unpark_handler(httpd_req_t *request);

esp_err_t rest_home_handler(httpd_req_t *request);

esp_err_t rest_settings_handler(httpd_req_t *request);

esp_err_t rest_wifi_handler(httpd_req_t *request);

void rest_send_result(
    httpd_req_t *request,
    MountResult result);

MotorAxis motors_axis_from_string(const char *value);
