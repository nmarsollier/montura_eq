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

/* ── Target microstep resolution ───────────────────────────── */

#define TMC_TARGET_MICROSTEPS  128

static const char *TAG = "TMC_INIT";

/* ── Per-axis configuration ────────────────────────────────── */

typedef struct {
    const char *name;
    uint8_t     address;   /* UART address (MS1/MS2 pins) */
    uint8_t     irun;      /* run current   (0 – 31) */
    uint8_t     ihold;     /* hold current  (0 – 31) */
} TmcAxis;

static const TmcAxis tmc_axes[] = {
    { .name = "RA",  .address = 0x03, .irun = 15, .ihold = 8 },
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

/* ── Driver init (write-only) ──────────────────────────────── */

static esp_err_t tmc_init_driver(const TmcAxis *axis)
{
    esp_err_t result;

    ESP_LOGI(TAG, "--- Initialising axis %s [Addr: 0x%02X] ---",
             axis->name, axis->address);

    /* GCONF: UART microstep control + digital current control. */
    uint32_t gconf = 0x000000C0;    /* mstep_reg_select=1, i_scale_analog=0 */
    result = tmc_write_register(axis->address, TMC_REG_GCONF, gconf);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: UART write failure on GCONF", axis->name);
        return result;
    }

    /* IHOLD_IRUN: hold + run current, IHOLDDELAY = 1. */
    uint32_t ihold_irun = (uint32_t)axis->ihold
                        | (1U << 8)                    /* IHOLDDELAY */
                        | ((uint32_t)axis->irun << 16);
    result = tmc_write_register(axis->address, TMC_REG_IHOLD_IRUN, ihold_irun);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: UART write failure on IHOLD_IRUN", axis->name);
        return result;
    }

    /* CHOPCONF: MRES=1 (128 µsteps), SpreadCycle, interpolation to 256. */
    uint32_t chopconf = 0x10410153;      /* power-on default */
    chopconf &= ~(0x0FU << 24);          /* clear MRES */
    chopconf |=  (1U << 24);             /* MRES = 1 → 128 µsteps */
    chopconf |=  (1U << 14);             /* SpreadCycle */
    chopconf |=  (1U << 28);             /* intpol → 256 µsteps */

    result = tmc_write_register(axis->address, TMC_REG_CHOPCONF, chopconf);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "%s: UART write failure on CHOPCONF", axis->name);
        return result;
    }

    tmc2209_set_active_microsteps(TMC_TARGET_MICROSTEPS);

    ESP_LOGI(TAG, "%s: configured (µsteps=%u, irun=%u, ihold=%u)",
             axis->name, TMC_TARGET_MICROSTEPS, axis->irun, axis->ihold);
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

    for (size_t i = 0; i < sizeof(tmc_axes) / sizeof(tmc_axes[0]); i++) {
        result = tmc_init_driver(&tmc_axes[i]);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Error initialising axis %s", tmc_axes[i].name);
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }
    return ESP_OK;
}
