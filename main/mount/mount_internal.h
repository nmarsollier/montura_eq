#pragma once

#include "mount.h"

#include <stdint.h>

/* Internal mount state stored by the mount module. */
extern MountSettings mount_internal_state;

MountResult mount_result_ok(void);

MountResult mount_result_error(const char *message);

/* NVS helpers used to persist mount settings. */
void mount_settings_load(MountSettings *out_settings);

void mount_settings_save(const MountSettings *settings);

MountResult motors_result_code_error_result(MotorResultCode rc);
