/* Runtime - runtime_config.c
 *
 * Purpose: static runtime configuration for periods and stacks.
 */
#include "runtime.h"

static const SetupRuntimeConfig runtime_config = {
    .main_loop_period_ms = 20,
    .screen_update_period_ms = 100,
    .inputs_poll_period_ms = 50,
    .main_task_stack_size = 4096,
    .main_task_priority = 5,
};

const SetupRuntimeConfig *setup_runtime_config_get(void) {
    return &runtime_config;
}
