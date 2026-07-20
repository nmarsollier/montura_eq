#pragma once

void setup_init(void);

void setup_runtime_start(void);

typedef struct {
    int main_loop_period_ms;
    int screen_update_period_ms;
    int inputs_poll_period_ms;
    int main_task_stack_size;
    int main_task_priority;
} SetupRuntimeConfig;

const SetupRuntimeConfig *setup_runtime_config_get(void);
