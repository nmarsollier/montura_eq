/* USB Net — usb_net_data.c — NCM data path.
 *
 * NCM mode uses the well-tested tinyusb_net_send_sync wrapper.
 */
#include "usb_net_internal.h"

#include "esp_log.h"

#include "lwip/netif.h"
#include "lwip/pbuf.h"

#include "freertos/FreeRTOS.h"
#include "tinyusb_net.h"

static const char *TAG = "USB_NET_DATA";

/*
 * lwIP calls usb_net_transmit() from its TCP/IP thread.  A long timeout
 * would stall the entire stack (WiFi included).  100 ms is enough for the
 * TinyUSB task to process a deferred send under normal conditions while
 * keeping lwIP responsive — the USB frame interval is 1 ms in Full Speed.
 */
#define USB_NET_TX_TIMEOUT_MS  100

esp_err_t usb_net_lwip_input(void *netif_handle, void *buffer, size_t len, void *l2_buff)
{
    struct netif *netif = (struct netif *)netif_handle;
    if (!netif || !buffer) return ESP_ERR_INVALID_ARG;

    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (!p) return ESP_ERR_NO_MEM;
    if (pbuf_take(p, buffer, len) != ERR_OK) { pbuf_free(p); return ESP_FAIL; }
    if (netif->input(p, netif) != ERR_OK) { pbuf_free(p); return ESP_FAIL; }
    return ESP_OK;
}

esp_err_t usb_net_transmit(void *driver_handle, void *buffer, size_t len)
{
    if (!buffer || !len) return ESP_ERR_INVALID_ARG;

    esp_err_t r = tinyusb_net_send_sync(buffer, (uint16_t)len, NULL,
                                         pdMS_TO_TICKS(USB_NET_TX_TIMEOUT_MS));

    /* ESP_ERR_INVALID_STATE means tud_mounted() == false — the USB cable
     * was unplugged.  Log it so the disconnect is visible in diagnostics. */
    if (r == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "TX dropped: USB not mounted");
    }

    return r;
}

void usb_net_free_rx_buffer(void *driver_handle, void *buffer)
{
    /* NCM mode manages RX buffers internally via TinyUSB — nothing
     * to free here.  This callback satisfies the esp_netif driver
     * interface contract. */
}
