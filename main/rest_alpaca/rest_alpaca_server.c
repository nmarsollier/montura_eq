/* rest_alpaca_server.c — Alpaca HTTP server on port 11111 */

#include "rest_alpaca.h"
#include "rest_alpaca_internal.h"
#include "utils/utils.h"

#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "REST_ALPACA_SERVER";

#define T  "/api/v1/telescope/0"

/* ─── Route registration ─── */

void rest_alpaca_server_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 11111;
    config.max_uri_handlers = 128;
    config.max_open_sockets = 14;
    config.lru_purge_enable = true;
    config.ctrl_port = 32769;

    esp_err_t result = httpd_start(&server, &config);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Alpaca server: %s", esp_err_to_name(result));
        return;
    }

    /* ─── Management API ───
     * Registered at both the versioned and unversioned paths.
     * The Alpaca spec uses /management/apiversions; many clients also
     * try /management/v1/apiversions. */
    rest_register_get(server, "/management/apiversions",
                      alpaca_management_apiversions_handler);
    rest_register_get(server, "/management/v1/apiversions",
                      alpaca_management_apiversions_handler);
    rest_register_get(server, "/management/description",
                      alpaca_management_description_handler);
    rest_register_get(server, "/management/v1/description",
                      alpaca_management_description_handler);
    rest_register_get(server, "/management/configureddevices",
                      alpaca_management_configureddevices_handler);
    rest_register_get(server, "/management/v1/configureddevices",
                      alpaca_management_configureddevices_handler);

    /* ─── Common device endpoints ─── */
    rest_register_get(server, T "/connected", alpaca_connected_handler);
    rest_register_put(server, T "/connected", alpaca_connected_put_handler);
    rest_register_get(server, T "/description", alpaca_description_handler);
    rest_register_get(server, T "/driverinfo", alpaca_driverinfo_handler);
    rest_register_get(server, T "/driverversion", alpaca_driverversion_handler);
    rest_register_get(server, T "/interfaceversion", alpaca_interfaceversion_handler);
    rest_register_get(server, T "/name", alpaca_name_handler);
    rest_register_get(server, T "/supportedactions", alpaca_supportedactions_handler);

    /* ─── Capabilities ─── */
    rest_register_get(server, T "/canfindhome", alpaca_canfindhome_handler);
    rest_register_get(server, T "/canpark", alpaca_canpark_handler);
    rest_register_get(server, T "/canpulseguide", alpaca_canpulseguide_handler);
    rest_register_get(server, T "/cansetdeclinationrate", alpaca_cansetdeclinationrate_handler);
    rest_register_get(server, T "/cansetguiderates", alpaca_cansetguiderates_handler);
    rest_register_get(server, T "/cansetpark", alpaca_cansetpark_handler);
    rest_register_get(server, T "/cansetpierside", alpaca_cansetpierside_handler);
    rest_register_get(server, T "/cansetrightascensionrate", alpaca_cansetrightascensionrate_handler);
    rest_register_get(server, T "/cansettracking", alpaca_cansettracking_handler);
    rest_register_get(server, T "/canslew", alpaca_canslew_handler);
    rest_register_get(server, T "/canslewaltaz", alpaca_canslewaltaz_handler);
    rest_register_get(server, T "/canslewaltazasync", alpaca_canslewaltazasync_handler);
    rest_register_get(server, T "/canslewasync", alpaca_canslewasync_handler);
    rest_register_get(server, T "/cansync", alpaca_cansync_handler);
    rest_register_get(server, T "/cansyncaltaz", alpaca_cansyncaltaz_handler);
    rest_register_get(server, T "/canunpark", alpaca_canunpark_handler);
    rest_register_get(server, T "/canmoveaxis", alpaca_canmoveaxis_handler);

    /* ─── Telescope properties GET ─── */
    rest_register_get(server, T "/alignmentmode", alpaca_alignmentmode_handler);
    rest_register_get(server, T "/altitude", alpaca_altitude_handler);
    rest_register_get(server, T "/aperturearea", alpaca_aperturearea_handler);
    rest_register_get(server, T "/aperturediameter", alpaca_aperturediameter_handler);
    rest_register_get(server, T "/athome", alpaca_athome_handler);
    rest_register_get(server, T "/atpark", alpaca_atpark_handler);
    rest_register_get(server, T "/azimuth", alpaca_azimuth_handler);
    rest_register_get(server, T "/declination", alpaca_declination_handler);
    rest_register_get(server, T "/declinationrate", alpaca_declinationrate_handler);
    rest_register_get(server, T "/equatorialsystem", alpaca_equatorialsystem_handler);
    rest_register_get(server, T "/focallength", alpaca_focallength_handler);
    rest_register_get(server, T "/guideratedeclination", alpaca_guideratedec_handler);
    rest_register_get(server, T "/guideraterightascension", alpaca_guideratera_handler);
    rest_register_get(server, T "/ispulseguiding", alpaca_ispulseguiding_handler);
    rest_register_get(server, T "/rightascension", alpaca_rightascension_handler);
    rest_register_get(server, T "/rightascensionrate", alpaca_rightascensionrate_handler);
    rest_register_get(server, T "/sideofpier", alpaca_sideofpier_handler);
    rest_register_get(server, T "/siderealtime", alpaca_siderealtime_handler);
    rest_register_get(server, T "/siteelevation", alpaca_siteelevation_handler);
    rest_register_get(server, T "/sitelatitude", alpaca_sitelatitude_handler);
    rest_register_get(server, T "/sitelongitude", alpaca_sitelongitude_handler);
    rest_register_get(server, T "/slewing", alpaca_slewing_handler);
    rest_register_get(server, T "/slewsettletime", alpaca_slewsettletime_handler);
    rest_register_get(server, T "/targetdeclination", alpaca_targetdec_handler);
    rest_register_get(server, T "/targetrightascension", alpaca_targetra_handler);
    rest_register_get(server, T "/tracking", alpaca_tracking_handler);
    rest_register_get(server, T "/trackingrate", alpaca_trackingrate_handler);
    rest_register_get(server, T "/trackingrates", alpaca_trackingrates_handler);
    rest_register_get(server, T "/utcdate", alpaca_utcdate_handler);

    /* Mock endpoints */
    rest_register_get(server, T "/axisrates", alpaca_axisrates_handler);
    rest_register_get(server, T "/doesrefraction", alpaca_doesrefraction_handler);
    rest_register_get(server, T "/destinationsideofpier", alpaca_destinationsideofpier_handler);

    /* ─── Telescope methods PUT ─── */
    rest_register_put(server, T "/abortslew", alpaca_abortslew_handler);
    rest_register_put(server, T "/findhome", alpaca_findhome_handler);
    rest_register_put(server, T "/park", alpaca_park_handler);
    rest_register_put(server, T "/unpark", alpaca_unpark_handler);
    rest_register_put(server, T "/setpark", alpaca_setpark_handler);
    rest_register_put(server, T "/slewtocoordinatesasync", alpaca_slewtocoordinatesasync_handler);
    rest_register_put(server, T "/slewtocoordinates", alpaca_slewtocoordinatesasync_handler);
    rest_register_put(server, T "/slewtoaltazasync", alpaca_slewtoaltazasync_handler);
    rest_register_put(server, T "/synctocoordinates", alpaca_synctocoordinates_handler);
    rest_register_put(server, T "/synctoaltaz", alpaca_synctoaltaz_handler);
    rest_register_put(server, T "/slewtotargetasync", alpaca_slewtotargetasync_handler);
    rest_register_put(server, T "/slewtotarget", alpaca_slewtotargetasync_handler);
    rest_register_put(server, T "/synctotarget", alpaca_synctotarget_handler);
    rest_register_put(server, T "/pulseguide", alpaca_pulseguide_handler);
    rest_register_put(server, T "/moveaxis", alpaca_moveaxis_handler);

    /* Property setters */
    rest_register_put(server, T "/tracking", alpaca_tracking_put_handler);
    rest_register_put(server, T "/trackingrate", alpaca_trackingrate_put_handler);
    rest_register_put(server, T "/utcdate", alpaca_utcdate_put_handler);
    rest_register_put(server, T "/sitelatitude", alpaca_sitelatitude_put_handler);
    rest_register_put(server, T "/sitelongitude", alpaca_sitelongitude_put_handler);
    rest_register_put(server, T "/siteelevation", alpaca_siteelevation_put_handler);
    rest_register_put(server, T "/targetrightascension", alpaca_targetra_put_handler);
    rest_register_put(server, T "/targetdeclination", alpaca_targetdec_put_handler);
    rest_register_put(server, T "/sideofpier", alpaca_sideofpier_put_handler);
    rest_register_put(server, T "/slewsettletime", alpaca_slewsettletime_put_handler);
    rest_register_put(server, T "/declinationrate", alpaca_declinationrate_put_handler);
    rest_register_put(server, T "/rightascensionrate", alpaca_rightascensionrate_put_handler);
    rest_register_put(server, T "/guideratedeclination", alpaca_guideratedec_put_handler);
    rest_register_put(server, T "/guideraterightascension", alpaca_guideratera_put_handler);

    ESP_LOGI(TAG, "Alpaca server started on port 11111");
}
