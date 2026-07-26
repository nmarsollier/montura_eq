/* Mount - mount_pulse_guide.c
 *
 * Purpose: validate mount state for PulseGuide, map astronomical
 * directions (North/South/East/West) to signed RA/DEC speed offsets,
 * and delegate to the motors layer.
 *
 * Direction mapping (offset added to base tracking speed):
 *   East  → +RA  (move scope east relative to sky)
 *   West  → -RA  (move scope west relative to sky)
 *   North → +DEC
 *   South → -DEC
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

#include "esp_log.h"

static const char *TAG = "MOUNT_PULSE_GUIDE";

MountResult mount_pulse_guide(GuideDirection direction, uint32_t duration_ms) {
    /* PulseGuide is only valid in READY (calibration) or TRACKING (guiding). */
    MotorsState state = motors_current_state();
    if (state.status != MOTORS_STATUS_READY &&
        state.status != MOTORS_STATUS_TRACKING) {
        ESP_LOGW(TAG, "Rejected: mount status=%s",
                 motors_status_to_string(state.status));
        return mount_result_error("Mount not ready for PulseGuide");
    }

    if (duration_ms == 0 || duration_ms > 30000) {
        return mount_result_error("Duration out of range (1-30000 ms)");
    }

    float ra_rate = mount_get_guide_rate_ra();
    float dec_rate = mount_get_guide_rate_dec();

    switch (direction) {
    case GUIDE_DIRECTION_EAST:
        motors_pulse_guide_start(0,  ra_rate, duration_ms);
        break;
    case GUIDE_DIRECTION_WEST:
        motors_pulse_guide_start(0, -ra_rate, duration_ms);
        break;
    case GUIDE_DIRECTION_NORTH:
        motors_pulse_guide_start(1,  dec_rate, duration_ms);
        break;
    case GUIDE_DIRECTION_SOUTH:
        motors_pulse_guide_start(1, -dec_rate, duration_ms);
        break;
    default:
        return mount_result_error("Invalid guide direction");
    }

    return mount_result_ok();
}

/* ── Guide rate storage (deg/s) — per axis for Alpaca compliance ── */

static float s_guide_rate_ra  = TRACKING_SPEED_SIDEREAL_DPS * 0.5f;
static float s_guide_rate_dec = TRACKING_SPEED_SIDEREAL_DPS * 0.5f;

void mount_set_guide_rate_ra(float rate_dps)  { s_guide_rate_ra = rate_dps; }
void mount_set_guide_rate_dec(float rate_dps) { s_guide_rate_dec = rate_dps; }
float mount_get_guide_rate_ra(void)           { return s_guide_rate_ra; }
float mount_get_guide_rate_dec(void)          { return s_guide_rate_dec; }
