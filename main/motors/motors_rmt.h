#pragma once

/* motors_rmt.h
 *
 * RMT + DMA step pulse generation for equatorial mount axes.
 *
 * Replaces software GPIO bit-banging (motors_hw_step_*) with hardware-timed
 * RMT (Remote Control) peripherals. Each axis gets a dedicated RMT TX channel
 * fed by GDMA, producing jitter-free STEP pulses while the CPU is yielded.
 *
 * The motion task pre-computes step sequences, encodes them as RMT symbols,
 * and transmits them via DMA. It then sleeps on a semaphore until the RMT
 * ISR signals completion, freeing the CPU for Alpaca, tracking, OLED, etc.
 */

#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialise both RMT TX channels (RA on GPIO 10, DEC on GPIO 15)
 * with DMA enabled. Allocates binary semaphores for completion signalling.
 *
 * Must be called once after motors_hw_init() and before any motion.
 */
esp_err_t motors_rmt_init(void);

/*
 * Release all RMT resources — channels, encoders, semaphores.
 */
esp_err_t motors_rmt_deinit(void);

/*
 * Encode one or more identical step pulses into RMT symbols.
 *
 * Each step is 2 us HIGH followed by (step_period_us - 2) us LOW.
 * When the idle portion exceeds the 15-bit RMT symbol limit (32767 ticks),
 * the function transparently splits the idle across multiple symbols.
 *
 * Parameters:
 *   symbols        — output buffer
 *   max_symbols    — capacity of the output buffer
 *   step_period_us — total period of one step in microseconds
 *   step_count     — number of consecutive steps to encode
 *
 * Returns the number of symbols written. If the buffer is exhausted
 * before all steps are encoded, returns max_symbols (partial batch).
 */
uint32_t motors_rmt_encode_steps(rmt_symbol_word_t *symbols,
                                  uint32_t max_symbols,
                                  uint32_t step_period_us,
                                  uint32_t step_count);

/*
 * Transmit a batch of RMT symbols on the RA axis (non-blocking).
 * The channel must have been initialised via motors_rmt_init().
 * Call motors_rmt_wait_ra() to block until completion.
 */
esp_err_t motors_rmt_transmit_ra(const rmt_symbol_word_t *symbols,
                                  uint32_t num_symbols);

/*
 * Transmit a batch of RMT symbols on the DEC axis (non-blocking).
 */
esp_err_t motors_rmt_transmit_dec(const rmt_symbol_word_t *symbols,
                                   uint32_t num_symbols);

/*
 * Block until the RA transmission completes or the timeout expires.
 * Returns ESP_OK on normal completion, ESP_ERR_TIMEOUT if the
 * deadline passes, or ESP_FAIL if the channel state is invalid.
 */
esp_err_t motors_rmt_wait_ra(TickType_t timeout_ticks);

/*
 * Block until the DEC transmission completes or the timeout expires.
 */
esp_err_t motors_rmt_wait_dec(TickType_t timeout_ticks);

/*
 * Abort in-flight RA transmission and wake any waiting task.
 * Safe to call from any task or ISR context (uses rmt_disable).
 */
void motors_rmt_abort_ra(void);

/*
 * Abort in-flight DEC transmission and wake any waiting task.
 */
void motors_rmt_abort_dec(void);

/*
 * Abort both channels simultaneously — used by motors_motion_stop()
 * to preempt any active motion.
 */
void motors_rmt_abort_both(void);

#ifdef __cplusplus
}
#endif
