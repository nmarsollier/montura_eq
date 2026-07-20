/* Runtime - runtime_loop.c
 *
 * Purpose: run the periodic mount cycle for screen updates and inputs.
 */
#include "runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"

/*
 * Business use case: orchestrate the mount's periodic operational cycle.
 *
 * Objective: keep display refresh and input polling running at a stable cadence.
 * Motor motion runs in its own task owned by the motors_motion subsystem.
 */
static void main_loop_task(void *arg) {
    const SetupRuntimeConfig *config = (const SetupRuntimeConfig *) arg;
    TickType_t main_period_ticks = pdMS_TO_TICKS(config->main_loop_period_ms);
    TickType_t screen_period_ticks = pdMS_TO_TICKS(config->screen_update_period_ms);
    TickType_t inputs_period_ticks = pdMS_TO_TICKS(config->inputs_poll_period_ms);

    TickType_t elapsed_screen_ticks = screen_period_ticks;
    TickType_t elapsed_inputs_ticks = inputs_period_ticks;

    while (true) {
        if (elapsed_screen_ticks >= screen_period_ticks) {
            elapsed_screen_ticks = 0;
        }

        if (elapsed_inputs_ticks >= inputs_period_ticks) {
            led_update();
            elapsed_inputs_ticks = 0;
        }

        elapsed_screen_ticks += main_period_ticks;
        elapsed_inputs_ticks += main_period_ticks;

        vTaskDelay(main_period_ticks);
    }
}

/*
 * Business use case: start the main runtime in the background.
 *
 * Objective: delegate continuous operation to a dedicated FreeRTOS task using
 * the configured runtime periods and priorities.
 */
void setup_runtime_start(void) {
    const SetupRuntimeConfig *config = setup_runtime_config_get();

    xTaskCreate(
        main_loop_task,
        "main_loop",
        config->main_task_stack_size,
        (void *) config,
        config->main_task_priority,
        nullptr);
}
