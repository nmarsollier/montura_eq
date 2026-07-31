/* TMC — tmc_init.c
 *
 * Purpose: initialise one or two TMC2209 stepper drivers over a
 *          single-wire UART bus and configure microstepping, current,
 *          and chopper mode.
 *
 * Both drivers share the same UART bus.  Each driver is addressed by
 * its MS1/MS2 pin levels — see the TmcAxis table below.
 */
#include "tmc.h"
#include "tmc_internal.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"

/* ── UART hardware ─────────────────────────────────────────── */

#define TMC_UART_NUM     UART_NUM_2
#define TMC_BAUD_RATE    115200

#define ESP_UART_TX_GPIO GPIO_NUM_17
#define ESP_UART_RX_GPIO GPIO_NUM_18

/* ── TMC2209 register addresses ────────────────────────────── */

#define TMC_REG_GCONF      0x00
#define TMC_REG_IHOLD_IRUN 0x10
#define TMC_REG_CHOPCONF   0x6C

/* ── Target microstep resolution — see TMC_TARGET_MICROSTEPS in tmc.h ── */

/* Convert TMC_TARGET_MICROSTEPS to the MRES register field.
 * TMC2209 MRES mapping: 256→0, 128→1, 64→2, 32→3, 16→4, 8→5, 4→6, 2→7, 1→8 */
static uint8_t tmc_microsteps_to_mres(uint16_t ms) {
    switch (ms) {
        case 256: return 0;
        case 128: return 1;
        case 64:  return 2;
        case 32:  return 3;
        case 16:  return 4;
        case 8:   return 5;
        case 4:   return 6;
        case 2:   return 7;
        default:  return 8;
    }
}

/* Convert MRES register field back to microsteps — used for read-back verification. */
static bool tmc_mres_to_microsteps(uint8_t mres, uint16_t *microsteps) {
    switch (mres) {
        case 0:  *microsteps = 256; return true;
        case 1:  *microsteps = 128; return true;
        case 2:  *microsteps = 64;  return true;
        case 3:  *microsteps = 32;  return true;
        case 4:  *microsteps = 16;  return true;
        case 5:  *microsteps = 8;   return true;
        case 6:  *microsteps = 4;   return true;
        case 7:  *microsteps = 2;   return true;
        case 8:  *microsteps = 1;   return true;
        default: return false;
    }
}

static const char *TAG = "TMC_INIT";

/* ── Per-axis configuration ────────────────────────────────── */

typedef struct {
    const char *name;
    uint8_t     address;   /* UART address (MS1/MS2 pins) */
    uint8_t     irun;      /* run current   (0 – 31) */
    uint8_t     ihold;     /* hold current  (0 – 31) */
} TmcAxis;

static const TmcAxis tmc_axes[] = {
    { .name = "RA",  .address = 0x03, .irun = 16, .ihold = 8 },
    { .name = "DEC", .address = 0x00, .irun = 12, .ihold = 6 },
};

/* ── UART helpers ──────────────────────────────────────────── */

static uint8_t tmc_crc(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (((crc >> 7) ^ (byte & 0x01)) != 0) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
            byte >>= 1;
        }
    }
    return crc;
}

static esp_err_t tmc_write_register(uint8_t address, uint8_t reg, uint32_t value)
{
    uint8_t request[8] = {
        0x05,
        address,
        (uint8_t)(reg | 0x80),
        (uint8_t)(value >> 24),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 8),
        (uint8_t)(value),
        0x00,
    };
    request[7] = tmc_crc(request, 7);

    uart_flush_input(TMC_UART_NUM);

    int written = uart_write_bytes(TMC_UART_NUM, request, sizeof(request));
    if (written != (int)sizeof(request)) {
        return ESP_FAIL;
    }

    uart_wait_tx_done(TMC_UART_NUM, pdMS_TO_TICKS(10));

    /* Absorb the 8-byte echo from the single-wire bus. */
    uint8_t echo_buffer[8];
    uart_read_bytes(TMC_UART_NUM, echo_buffer, sizeof(request), pdMS_TO_TICKS(10));

    return ESP_OK;
}

/*
 * Read a TMC2209 register over the single-wire UART bus.
 *
 * The ESP32 TX (GPIO 17) and RX (GPIO 18) share the TMC2209 PDN_UART pin
 * through a series resistor on TX.  After sending the read request the TX
 * pad is temporarily switched to input so the TMC2209 can drive the line
 * for its 8-byte response without the ESP32 TX output stage fighting it.
 */
static esp_err_t tmc_read_register(uint8_t address, uint8_t reg, uint32_t *value)
{
    if (value == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t request[4] = {
        0x05,
        address,
        (uint8_t)(reg & 0x7F),   /* bit 7 = 0 → read */
        0x00,
    };
    request[3] = tmc_crc(request, 3);

    uart_flush_input(TMC_UART_NUM);

    int written = uart_write_bytes(TMC_UART_NUM, request, sizeof(request));
    if (written != (int)sizeof(request)) return ESP_FAIL;

    uart_wait_tx_done(TMC_UART_NUM, pdMS_TO_TICKS(10));

    /* Absorb the 4-byte echo. */
    uint8_t echo_buf[4];
    uart_read_bytes(TMC_UART_NUM, echo_buf, sizeof(request), pdMS_TO_TICKS(10));

    /*
     * Float the TX pin so the TMC2209 can drive the shared bus.
     * RX stays configured as UART input through the IO MUX.
     */
    gpio_set_direction(ESP_UART_TX_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ESP_UART_TX_GPIO, GPIO_PULLUP_ONLY);

    /* Collect the 8-byte TMC2209 response frame. */
    uint8_t response[32];
    int len = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(50);

    while (xTaskGetTickCount() < deadline && len < (int)sizeof(response)) {
        int n = uart_read_bytes(TMC_UART_NUM, response + len,
                                sizeof(response) - len, pdMS_TO_TICKS(5));
        if (n > 0) len += n;
    }

    /* Restore TX to UART-controlled output. */
    gpio_set_direction(ESP_UART_TX_GPIO, GPIO_MODE_OUTPUT);
    uart_set_pin(TMC_UART_NUM, ESP_UART_TX_GPIO, ESP_UART_RX_GPIO,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    if (len < 8) return ESP_ERR_TIMEOUT;

    /* Find and validate a frame in the response buffer. */
    for (int i = 0; i <= len - 8; i++) {
        uint8_t *frame = &response[i];
        if (frame[0] != 0x05) continue;
        if ((frame[2] & 0x7F) != (reg & 0x7F)) continue;
        if (tmc_crc(frame, 7) != frame[7]) continue;

        *value = ((uint32_t)frame[3] << 24) |
                 ((uint32_t)frame[4] << 16) |
                 ((uint32_t)frame[5] << 8)  |
                 ((uint32_t)frame[6]);
        return ESP_OK;
    }

    return ESP_ERR_INVALID_RESPONSE;
}

/* ── Driver init (write + verify) ──────────────────────────── */

static esp_err_t tmc_init_driver(const TmcAxis *axis)
{
    esp_err_t result;
    uint32_t verify = 0;

    ESP_LOGI(TAG, "--- Initialising axis %s [Addr: 0x%02X] ---",
             axis->name, axis->address);

    /* ── GCONF: UART microstep control + digital current control ── */
    uint32_t gconf = 0x000000C0;    /* mstep_reg_select=1, pdn_disable=1, i_scale_analog=0 */
    result = tmc_write_register(axis->address, TMC_REG_GCONF, gconf);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: UART write failure on GCONF", axis->name);
        return result;
    }

    /* Verify GCONF — mstep_reg_select (bit 7) must be set.
     * Readback may fail transiently on the first attempt; the real
     * verification is CHOPCONF below (the register that controls µsteps). */
    result = tmc_read_register(axis->address, TMC_REG_GCONF, &verify);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "%s: cannot read back GCONF (%s) — continuing",
                 axis->name, esp_err_to_name(result));
    } else if (!(verify & (1U << 7))) {
        ESP_LOGW(TAG, "%s: GCONF mstep_reg_select NOT latched! (read 0x%08lX, expected bit 7 set)",
                 axis->name, (unsigned long)verify);
        ESP_LOGW(TAG, "%s: TMC2209 may be using pin-controlled microsteps — movement will be wrong!", axis->name);
    } else {
        ESP_LOGI(TAG, "%s: GCONF verified (0x%08lX, mstep_reg_select=1, i_scale_analog=%lu)",
                 axis->name, (unsigned long)verify, (unsigned long)(verify & 1U));
    }

    /* ── IHOLD_IRUN ── */
    uint32_t ihold_irun = (uint32_t)axis->ihold
                        | (1U << 8)                    /* IHOLDDELAY */
                        | ((uint32_t)axis->irun << 16);
    result = tmc_write_register(axis->address, TMC_REG_IHOLD_IRUN, ihold_irun);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: UART write failure on IHOLD_IRUN", axis->name);
        return result;
    }

    /* ── CHOPCONF ── */
    uint32_t chopconf = 0x10410153;      /* power-on default */
    chopconf &= ~(0x0FU << 24);          /* clear MRES */
    chopconf |=  ((uint32_t)tmc_microsteps_to_mres(TMC_TARGET_MICROSTEPS) << 24);
    chopconf |=  (1U << 14);             /* SpreadCycle */
    chopconf |=  (1U << 28);             /* intpol → 256 µsteps */

    result = tmc_write_register(axis->address, TMC_REG_CHOPCONF, chopconf);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: UART write failure on CHOPCONF", axis->name);
        return result;
    }

    /* ── Verify CHOPCONF ── */
    uint16_t verified_msteps = 0;
    result = tmc_read_register(axis->address, TMC_REG_CHOPCONF, &verify);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: cannot read back CHOPCONF (%s) — µsteps UNVERIFIED!",
                 axis->name, esp_err_to_name(result));
        return ESP_FAIL;
    }

    uint8_t mres = (uint8_t)((verify >> 24) & 0x0F);
    bool intpol_ok = (verify & (1U << 28)) != 0;
    bool spread_ok = (verify & (1U << 14)) != 0;

    if (!tmc_mres_to_microsteps(mres, &verified_msteps)) {
        ESP_LOGE(TAG, "%s: CHOPCONF readback has invalid MRES=%u (0x%08lX)",
                 axis->name, mres, (unsigned long)verify);
        return ESP_FAIL;
    }

    if (verified_msteps != TMC_TARGET_MICROSTEPS) {
        ESP_LOGE(TAG, "%s: CHOPCONF MRES MISMATCH! wrote=%u µsteps, hardware=%u µsteps (0x%08lX)",
                 axis->name, TMC_TARGET_MICROSTEPS, verified_msteps, (unsigned long)verify);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s: CHOPCONF verified — %u µsteps, intpol=%s, SpreadCycle=%s (0x%08lX)",
             axis->name, verified_msteps,
             intpol_ok ? "ON" : "OFF",
             spread_ok ? "ON" : "OFF",
             (unsigned long)verify);
    tmc2209_set_active_microsteps(verified_msteps);

    ESP_LOGI(TAG, "%s: init complete (cached µsteps=%u, irun=%u, ihold=%u)",
             axis->name, verified_msteps, axis->irun, axis->ihold);
    return ESP_OK;
}

/* ── Public entry point ────────────────────────────────────── */

esp_err_t tmc2209_hw_init(void)
{
    uart_config_t config = {
        .baud_rate  = TMC_BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t result = uart_driver_install(TMC_UART_NUM, 512, 512, 0, NULL, 0);
    if (result != ESP_OK) return result;

    result = uart_param_config(TMC_UART_NUM, &config);
    if (result != ESP_OK) return result;

    result = uart_set_pin(TMC_UART_NUM,
                          ESP_UART_TX_GPIO, ESP_UART_RX_GPIO,
                          UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (result != ESP_OK) return result;

    gpio_set_pull_mode(ESP_UART_RX_GPIO, GPIO_PULLUP_ONLY);

    bool all_ok = true;
    for (size_t i = 0; i < sizeof(tmc_axes) / sizeof(tmc_axes[0]); i++) {
        result = tmc_init_driver(&tmc_axes[i]);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Error initialising axis %s", tmc_axes[i].name);
            all_ok = false;
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }

    if (!all_ok) {
        ESP_LOGE(TAG, "One or more axes failed UART verification — microsteps not trusted, mount in error state");
        tmc2209_set_active_microsteps(0);
        return ESP_FAIL;
    }

    return ESP_OK;
}
