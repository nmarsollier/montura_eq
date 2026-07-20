#pragma once

#include <esp_http_server.h>

/* ─── Alpaca server ─── */
void rest_alpaca_server_start(void);

/* ─── Common GET handlers ─── */
esp_err_t alpaca_connected_handler(httpd_req_t *req);

esp_err_t alpaca_description_handler(httpd_req_t *req);

esp_err_t alpaca_driverinfo_handler(httpd_req_t *req);

esp_err_t alpaca_driverversion_handler(httpd_req_t *req);

esp_err_t alpaca_interfaceversion_handler(httpd_req_t *req);

esp_err_t alpaca_name_handler(httpd_req_t *req);

esp_err_t alpaca_supportedactions_handler(httpd_req_t *req);

/* ─── Capability GET handlers (all return bool) ─── */
esp_err_t alpaca_canfindhome_handler(httpd_req_t *req);

esp_err_t alpaca_canpark_handler(httpd_req_t *req);

esp_err_t alpaca_canpulseguide_handler(httpd_req_t *req);

esp_err_t alpaca_cansetdeclinationrate_handler(httpd_req_t *req);

esp_err_t alpaca_cansetguiderates_handler(httpd_req_t *req);

esp_err_t alpaca_cansetpark_handler(httpd_req_t *req);

esp_err_t alpaca_cansetpierside_handler(httpd_req_t *req);

esp_err_t alpaca_cansetrightascensionrate_handler(httpd_req_t *req);

esp_err_t alpaca_cansettracking_handler(httpd_req_t *req);

esp_err_t alpaca_canslew_handler(httpd_req_t *req);

esp_err_t alpaca_canslewaltaz_handler(httpd_req_t *req);

esp_err_t alpaca_canslewaltazasync_handler(httpd_req_t *req);

esp_err_t alpaca_canslewasync_handler(httpd_req_t *req);

esp_err_t alpaca_cansync_handler(httpd_req_t *req);

esp_err_t alpaca_cansyncaltaz_handler(httpd_req_t *req);

esp_err_t alpaca_canunpark_handler(httpd_req_t *req);

esp_err_t alpaca_canmoveaxis_handler(httpd_req_t *req);

/* ─── Telescope property GET handlers ─── */
esp_err_t alpaca_alignmentmode_handler(httpd_req_t *req);

esp_err_t alpaca_altitude_handler(httpd_req_t *req);

esp_err_t alpaca_aperturearea_handler(httpd_req_t *req);

esp_err_t alpaca_aperturediameter_handler(httpd_req_t *req);

esp_err_t alpaca_athome_handler(httpd_req_t *req);

esp_err_t alpaca_atpark_handler(httpd_req_t *req);

esp_err_t alpaca_azimuth_handler(httpd_req_t *req);

esp_err_t alpaca_declination_handler(httpd_req_t *req);

esp_err_t alpaca_declinationrate_handler(httpd_req_t *req);

esp_err_t alpaca_equatorialsystem_handler(httpd_req_t *req);

esp_err_t alpaca_focallength_handler(httpd_req_t *req);

esp_err_t alpaca_guideratedec_handler(httpd_req_t *req);

esp_err_t alpaca_guideratera_handler(httpd_req_t *req);

esp_err_t alpaca_ispulseguiding_handler(httpd_req_t *req);

esp_err_t alpaca_rightascension_handler(httpd_req_t *req);

esp_err_t alpaca_rightascensionrate_handler(httpd_req_t *req);

esp_err_t alpaca_sideofpier_handler(httpd_req_t *req);

esp_err_t alpaca_siderealtime_handler(httpd_req_t *req);

esp_err_t alpaca_siteelevation_handler(httpd_req_t *req);

esp_err_t alpaca_sitelatitude_handler(httpd_req_t *req);

esp_err_t alpaca_sitelongitude_handler(httpd_req_t *req);

esp_err_t alpaca_slewing_handler(httpd_req_t *req);

esp_err_t alpaca_slewsettletime_handler(httpd_req_t *req);

esp_err_t alpaca_targetdec_handler(httpd_req_t *req);

esp_err_t alpaca_targetra_handler(httpd_req_t *req);

esp_err_t alpaca_tracking_handler(httpd_req_t *req);

esp_err_t alpaca_trackingrate_handler(httpd_req_t *req);

esp_err_t alpaca_trackingrates_handler(httpd_req_t *req);

esp_err_t alpaca_utcdate_handler(httpd_req_t *req);

esp_err_t alpaca_axisrates_handler(httpd_req_t *req);

esp_err_t alpaca_doesrefraction_handler(httpd_req_t *req);

esp_err_t alpaca_destinationsideofpier_handler(httpd_req_t *req);

/* ─── Telescope method PUT handlers ─── */
esp_err_t alpaca_connected_put_handler(httpd_req_t *req);

esp_err_t alpaca_abortslew_handler(httpd_req_t *req);

esp_err_t alpaca_findhome_handler(httpd_req_t *req);

esp_err_t alpaca_park_handler(httpd_req_t *req);

esp_err_t alpaca_unpark_handler(httpd_req_t *req);

esp_err_t alpaca_setpark_handler(httpd_req_t *req);

esp_err_t alpaca_slewtocoordinatesasync_handler(httpd_req_t *req);

esp_err_t alpaca_slewtoaltazasync_handler(httpd_req_t *req);

esp_err_t alpaca_synctocoordinates_handler(httpd_req_t *req);

esp_err_t alpaca_synctoaltaz_handler(httpd_req_t *req);

esp_err_t alpaca_slewtotargetasync_handler(httpd_req_t *req);

esp_err_t alpaca_synctotarget_handler(httpd_req_t *req);

esp_err_t alpaca_pulseguide_handler(httpd_req_t *req);

esp_err_t alpaca_moveaxis_handler(httpd_req_t *req);

esp_err_t alpaca_tracking_put_handler(httpd_req_t *req);

esp_err_t alpaca_trackingrate_put_handler(httpd_req_t *req);

esp_err_t alpaca_utcdate_put_handler(httpd_req_t *req);

esp_err_t alpaca_sitelatitude_put_handler(httpd_req_t *req);

esp_err_t alpaca_sitelongitude_put_handler(httpd_req_t *req);

esp_err_t alpaca_siteelevation_put_handler(httpd_req_t *req);

esp_err_t alpaca_targetra_put_handler(httpd_req_t *req);

esp_err_t alpaca_targetdec_put_handler(httpd_req_t *req);

esp_err_t alpaca_sideofpier_put_handler(httpd_req_t *req);

esp_err_t alpaca_slewsettletime_put_handler(httpd_req_t *req);

esp_err_t alpaca_declinationrate_put_handler(httpd_req_t *req);

esp_err_t alpaca_rightascensionrate_put_handler(httpd_req_t *req);

esp_err_t alpaca_guideratedec_put_handler(httpd_req_t *req);

esp_err_t alpaca_guideratera_put_handler(httpd_req_t *req);

/* ─── Management API ─── */
esp_err_t alpaca_management_apiversions_handler(httpd_req_t *req);

esp_err_t alpaca_management_description_handler(httpd_req_t *req);

esp_err_t alpaca_management_configureddevices_handler(httpd_req_t *req);
