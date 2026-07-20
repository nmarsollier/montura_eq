/* Mount - mount_visible_status.c
 *
 * Purpose: build the visible status snapshot for UI and API consumers.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

VisibleStatusData mount_get_visible_status(void) {
    MotorsState pos = motors_current_state();
    AxisCoordinates axis = {
        .ra_axis_deg = pos.ra_position,
        .dec_axis_deg = pos.dec_position
    };

    EquatorialCoordinates eq = axis_to_equatorial(axis);

    /* Pier side from dec_axis sign (DEC is linear, not normalised):
     * dec_axis >= 0 -> pierEast, dec_axis < 0 -> pierWest. */
    int pier = (axis.dec_axis_deg >= 0.0f) ? 0 : 1;

    return (VisibleStatusData)
    {
        .status = pos.status,
        .tracking = pos.tracking,
        .ra = ra_hours_to_hms(eq.ra_hours),
        .dec = dec_deg_to_dms(eq.dec_deg),
        .lst_hours = mount_get_lst(),
        .pier_side = pier,
        .settings = mount_internal_state,
    };
}
