#include "mount.h"
#include "mount_internal.h"
#include "motors.h"

static TrackingMode s_saved_tracking = TRACKING_NONE;

MountResult mount_set_move_axis_speed(float ra_speed, float dec_speed) {
    if ((int) ra_speed == 0 && (int) dec_speed == 0) {
        /* Read before mount_stop() — mount_move_axis_reset() clears it. */
        TrackingMode to_restore = s_saved_tracking;
        s_saved_tracking = TRACKING_NONE;
        MountResult r = mount_stop();
        if (to_restore != TRACKING_NONE) {
            mount_set_tracking(to_restore);
        }
        return r;
    }

    MotorsState s = motors_current_state();
    if (s.status == MOTORS_STATUS_TRACKING && s.tracking != TRACKING_NONE) {
        /* Save before mount_stop() — it calls mount_move_axis_reset(). */
        TrackingMode saved = s.tracking;
        mount_stop();
        s_saved_tracking = saved;
    }

    motors_set_move_axis_speed(ra_speed, dec_speed);

    return mount_result_ok();
}

void mount_move_axis_reset(void) {
    s_saved_tracking = TRACKING_NONE;
}
