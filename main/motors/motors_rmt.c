/* Motors - motors_rmt.c
 *
 * Purpose: RMT + DMA step pulse generation for RA and DEC axes.
 *
 * Two independent RMT TX channels with GDMA streaming eliminate
 * software jitter and free the CPU during step bursts.
 * Step pulses are 2 us HIGH followed by (period - 2) us LOW.
 */

#include "motors_internal.h"

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "MOTORS_RMT";

/* --------------------------------------------------------------------------
 * Hardware constants
 * -------------------------------------------------------------------------- */
#define RMT_RESOLUTION_HZ     1000000U  /* 1 MHz — 1 us per tick */
#define RMT_MEM_BLOCK_SYMBOLS 48U       /* symbols per mem block for non-DMA */
#define RMT_TRANS_QUEUE_DEPTH 2U        /* one active + one queued */

/* RMT symbol max duration per half — 15-bit field. */
#define RMT_DURATION_MAX  32767U

/* --------------------------------------------------------------------------
 * Per-channel state
 * -------------------------------------------------------------------------- */
typedef struct {
    rmt_channel_handle_t channel;
    rmt_encoder_handle_t encoder;
    SemaphoreHandle_t done_sem;
    bool enabled;
    bool transmitting;
} rmt_chan_ctx_t;

/* --------------------------------------------------------------------------
 * Module state
 * -------------------------------------------------------------------------- */
static struct {
    rmt_chan_ctx_t ra;
    rmt_chan_ctx_t dec;
    bool initialized;
} s_rmt;

/* --------------------------------------------------------------------------
 * ISR callbacks — must be IRAM-resident for low-latency completion.
 * -------------------------------------------------------------------------- */

static bool IRAM_ATTR on_ra_tx_done(rmt_channel_handle_t channel,
                                     const rmt_tx_done_event_data_t *edata,
                                     void *user_data)
{
    (void)channel;
    (void)edata;
    (void)user_data;
    BaseType_t higher_priority_woken = pdFALSE;
    xSemaphoreGiveFromISR(s_rmt.ra.done_sem, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
}

static bool IRAM_ATTR on_dec_tx_done(rmt_channel_handle_t channel,
                                      const rmt_tx_done_event_data_t *edata,
                                      void *user_data)
{
    (void)channel;
    (void)edata;
    (void)user_data;
    BaseType_t higher_priority_woken = pdFALSE;
    xSemaphoreGiveFromISR(s_rmt.dec.done_sem, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
}

/* --------------------------------------------------------------------------
 * Channel factory
 * -------------------------------------------------------------------------- */

/*
 * Create one RMT TX channel.
 *
 * On ESP32-S3 only the LAST TX channel in the group has DMA capability,
 * so with_dma may be set for at most one channel.  The non-DMA channel
 * uses the RMT peripheral's internal FIFO (48 symbols, ~7 ms of max-slew
 * motion) and is refilled synchronously by the motion task.
 */
static esp_err_t create_channel(gpio_num_t gpio, rmt_chan_ctx_t *ctx,
                                 rmt_tx_done_callback_t on_done,
                                 bool with_dma)
{
    rmt_tx_channel_config_t tx_config = {
        .gpio_num = gpio,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = RMT_MEM_BLOCK_SYMBOLS,
        .trans_queue_depth = RMT_TRANS_QUEUE_DEPTH,
        .flags.with_dma = with_dma,
    };

    esp_err_t err = rmt_new_tx_channel(&tx_config, &ctx->channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_tx_channel GPIO %d: %s", gpio, esp_err_to_name(err));
        return err;
    }

    rmt_copy_encoder_config_t copy_config = {};
    err = rmt_new_copy_encoder(&copy_config, &ctx->encoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_copy_encoder GPIO %d: %s", gpio, esp_err_to_name(err));
        return err;
    }

    rmt_tx_event_callbacks_t cbs = {
        .on_trans_done = on_done,
    };
    err = rmt_tx_register_event_callbacks(ctx->channel, &cbs, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_tx_register_event_callbacks GPIO %d: %s", gpio, esp_err_to_name(err));
        return err;
    }

    /*
     * rmt_enable() is called automatically by rmt_transmit() on the
     * first transmission.  Calling it here would reserve the channel
     * resources prematurely and may prevent a second channel from
     * being allocated.
     */

    ctx->done_sem = xSemaphoreCreateBinary();
    if (ctx->done_sem == NULL) {
        ESP_LOGE(TAG, "xSemaphoreCreateBinary GPIO %d failed", gpio);
        return ESP_ERR_NO_MEM;
    }

    ctx->transmitting = false;
    ESP_LOGI(TAG, "Channel ready: GPIO %d", gpio);
    return ESP_OK;
}

/* --------------------------------------------------------------------------
 * Public API — init / deinit
 * -------------------------------------------------------------------------- */

esp_err_t motors_rmt_init(void)
{
    if (s_rmt.initialized) {
        return ESP_OK;
    }

    memset(&s_rmt, 0, sizeof(s_rmt));

    esp_err_t err = create_channel(RA_STEP_GPIO, &s_rmt.ra, on_ra_tx_done, true);
    if (err != ESP_OK) {
        motors_rmt_deinit();
        return err;
    }

    err = create_channel(DEC_STEP_GPIO, &s_rmt.dec, on_dec_tx_done, false);
    if (err != ESP_OK) {
        motors_rmt_deinit();
        return err;
    }

    s_rmt.initialized = true;
    ESP_LOGI(TAG, "RMT+DMA step generation ready (RA GPIO %d, DEC GPIO %d)",
             RA_STEP_GPIO, DEC_STEP_GPIO);
    return ESP_OK;
}

esp_err_t motors_rmt_deinit(void)
{
    if (s_rmt.ra.channel != NULL) {
        rmt_disable(s_rmt.ra.channel);
        rmt_del_channel(s_rmt.ra.channel);
        s_rmt.ra.channel = NULL;
        s_rmt.ra.encoder = NULL; /* freed with channel */
    }
    if (s_rmt.ra.done_sem != NULL) {
        vSemaphoreDelete(s_rmt.ra.done_sem);
        s_rmt.ra.done_sem = NULL;
    }

    if (s_rmt.dec.channel != NULL) {
        rmt_disable(s_rmt.dec.channel);
        rmt_del_channel(s_rmt.dec.channel);
        s_rmt.dec.channel = NULL;
        s_rmt.dec.encoder = NULL;
    }
    if (s_rmt.dec.done_sem != NULL) {
        vSemaphoreDelete(s_rmt.dec.done_sem);
        s_rmt.dec.done_sem = NULL;
    }

    s_rmt.initialized = false;
    return ESP_OK;
}

/* --------------------------------------------------------------------------
 * Step encoding
 * -------------------------------------------------------------------------- */

/*
 * Encode a single step into RMT symbols starting at 'symbols'.
 * Returns the number of symbols consumed.
 *
 * Step structure:
 *   2 us HIGH (pulse)
 *   (period_us - 2) us LOW (idle until next step)
 *
 * When the idle portion exceeds RMT_DURATION_MAX (32767 ticks),
 * the function splits it across multiple idle-only symbols.
 * Each idle symbol carries up to 65534 us of idle (both halves maxed).
 */
static uint32_t encode_one_step(rmt_symbol_word_t *symbols,
                                 uint32_t max_symbols,
                                 uint32_t period_us)
{
    uint32_t idx = 0;
    const uint32_t pulse_ticks = 2U;

    if (period_us <= pulse_ticks) {
        if (idx < max_symbols) {
            symbols[idx].level0 = 1;
            symbols[idx].duration0 = period_us;
            symbols[idx].level1 = 0;
            symbols[idx].duration1 = 0;
            idx++;
        }
        return idx;
    }

    uint32_t idle_ticks = period_us - pulse_ticks;

    /* First symbol: pulse (HIGH) + up to 32767 idle (LOW). */
    if (idx >= max_symbols) return idx;
    symbols[idx].level0 = 1;
    symbols[idx].duration0 = pulse_ticks;
    symbols[idx].level1 = 0;

    if (idle_ticks <= RMT_DURATION_MAX) {
        symbols[idx].duration1 = idle_ticks;
        idx++;
        return idx;
    }

    symbols[idx].duration1 = RMT_DURATION_MAX;
    idx++;
    idle_ticks -= RMT_DURATION_MAX;

    /* Bulk idle symbols — both halves idle, max 65534 us each. */
    while (idle_ticks > (RMT_DURATION_MAX * 2U)) {
        if (idx >= max_symbols) return idx;
        symbols[idx].level0 = 0;
        symbols[idx].duration0 = RMT_DURATION_MAX;
        symbols[idx].level1 = 0;
        symbols[idx].duration1 = RMT_DURATION_MAX;
        idx++;
        idle_ticks -= (RMT_DURATION_MAX * 2U);
    }

    /* Final idle fragment. */
    if (idle_ticks > 0 && idx < max_symbols) {
        symbols[idx].level0 = 0;
        symbols[idx].level1 = 0;
        if (idle_ticks <= RMT_DURATION_MAX) {
            symbols[idx].duration0 = idle_ticks;
            symbols[idx].duration1 = 0;
        } else {
            symbols[idx].duration0 = RMT_DURATION_MAX;
            symbols[idx].duration1 = idle_ticks - RMT_DURATION_MAX;
        }
        idx++;
    }

    return idx;
}

uint32_t motors_rmt_encode_steps(rmt_symbol_word_t *symbols,
                                  uint32_t max_symbols,
                                  uint32_t step_period_us,
                                  uint32_t step_count)
{
    uint32_t total = 0;

    for (uint32_t i = 0; i < step_count; i++) {
        uint32_t remaining = max_symbols - total;
        if (remaining == 0) break;

        uint32_t consumed = encode_one_step(&symbols[total], remaining,
                                             step_period_us);
        total += consumed;
    }

    return total;
}

/* --------------------------------------------------------------------------
 * Transmit — non-blocking, DMA-driven
 * -------------------------------------------------------------------------- */

/*
 * Transmit symbols on one channel.  Enables the RMT channel on the
 * first call (rmt_enable arms the interrupt so the done callback fires).
 * Subsequent calls are no-ops for enable since the channel is already on.
 */
static esp_err_t transmit_channel(rmt_chan_ctx_t *ctx,
                                   const rmt_symbol_word_t *symbols,
                                   uint32_t num_symbols)
{
    if (ctx->channel == NULL || ctx->encoder == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Enable the channel before the first transmission.
     * rmt_enable() arms the TX-done interrupt, without which the
     * ISR callback never fires and the semaphore stays blocked.
     * Called only once per channel lifetime.
     */
    if (!ctx->enabled) {
        esp_err_t enable_err = rmt_enable(ctx->channel);
        if (enable_err != ESP_OK) {
            ESP_LOGE(TAG, "rmt_enable failed: %s", esp_err_to_name(enable_err));
            return enable_err;
        }
        ctx->enabled = true;
    }

    /* Drain any stale semaphore count from a previous abort. */
    xSemaphoreTake(ctx->done_sem, 0);

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
    };

    size_t payload_bytes = num_symbols * sizeof(rmt_symbol_word_t);
    esp_err_t err = rmt_transmit(ctx->channel, ctx->encoder,
                                  symbols, payload_bytes, &tx_cfg);
    if (err == ESP_OK) {
        ctx->transmitting = true;
    } else {
        ESP_LOGE(TAG, "rmt_transmit failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t motors_rmt_transmit_ra(const rmt_symbol_word_t *symbols,
                                  uint32_t num_symbols)
{
    return transmit_channel(&s_rmt.ra, symbols, num_symbols);
}

esp_err_t motors_rmt_transmit_dec(const rmt_symbol_word_t *symbols,
                                   uint32_t num_symbols)
{
    return transmit_channel(&s_rmt.dec, symbols, num_symbols);
}

/* --------------------------------------------------------------------------
 * Wait — block on semaphore until ISR signals completion
 * -------------------------------------------------------------------------- */

esp_err_t motors_rmt_wait_ra(TickType_t timeout_ticks)
{
    if (s_rmt.ra.done_sem == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_rmt.ra.done_sem, timeout_ticks) == pdTRUE) {
        s_rmt.ra.transmitting = false;
        return ESP_OK;
    }
    ESP_LOGW(TAG, "RA wait timeout");
    return ESP_ERR_TIMEOUT;
}

esp_err_t motors_rmt_wait_dec(TickType_t timeout_ticks)
{
    if (s_rmt.dec.done_sem == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_rmt.dec.done_sem, timeout_ticks) == pdTRUE) {
        s_rmt.dec.transmitting = false;
        return ESP_OK;
    }
    ESP_LOGW(TAG, "DEC wait timeout");
    return ESP_ERR_TIMEOUT;
}

/* --------------------------------------------------------------------------
 * Abort — stop DMA and wake blocked task
 * -------------------------------------------------------------------------- */

/*
 * Abort one channel: disable the RMT peripheral (stops DMA immediately),
 * re-enable it for the next transmission, and give the semaphore so any
 * blocked waiter can check motion_active and exit.
 */
static void abort_channel(rmt_chan_ctx_t *ctx)
{
    if (ctx->channel != NULL) {
        rmt_disable(ctx->channel);
        rmt_enable(ctx->channel);
    }
    /*
     * Give the semaphore regardless of whether a transmission was in
     * flight — a binary semaphore silently saturates at count = 1,
     * so a double-give (ISR already fired + this explicit give) is safe.
     */
    if (ctx->done_sem != NULL) {
        xSemaphoreGive(ctx->done_sem);
    }
    ctx->transmitting = false;
}

void motors_rmt_abort_ra(void)
{
    abort_channel(&s_rmt.ra);
}

void motors_rmt_abort_dec(void)
{
    abort_channel(&s_rmt.dec);
}

void motors_rmt_abort_both(void)
{
    abort_channel(&s_rmt.ra);
    abort_channel(&s_rmt.dec);
}
