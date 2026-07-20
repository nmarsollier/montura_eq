/* Mount - mount_slew_to_coordinates.c
 *
 * Purpose: move the mount to a requested equatorial position.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

MountResult mount_slew_to_coordinates(float ra, float dec, int speed_rate) {

    EquatorialCoordinates eq = {
        .ra_hours = ra,
        .dec_deg = dec
    };

    MotorsState ms = motors_current_state();
    AxisCoordinates current = { .ra_axis_deg = ms.ra_position,
                                .dec_axis_deg = ms.dec_position };

    AxisCoordinates axis;
    if (!equatorial_to_axis(eq, current, &axis)) {
        return mount_result_error("Target unreachable — no valid axis solution");
    }

    MotorResultCode rc1 = motors_slew_to_angle(axis.ra_axis_deg, axis.dec_axis_deg, speed_rate,
                                               mount_internal_state.lat);

    if (rc1 != MOTOR_OK) {
        return mount_result_error("Failed to start slew to coordinates");
    }

    return mount_result_ok();
}
