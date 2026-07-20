/* USB Net — usb_net_data.c — NCM data path.
 *
 * NCM mode uses the well-tested tinyusb_net_send_sync wrapper.
 */
#include "usb_net_internal.h"

#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"

#include "freertos/FreeRTOS.h"
#include "tinyusb_net.h"

void usb_net_set_recv_netif(esp_netif_t *netif) { (void)netif; }

err_t usb_net_lwip_init(struct netif *netif)
{
    netif->name[0] = 'u'; netif->name[1] = 's';
    netif->hwaddr_len = 6;
    netif->mtu = USB_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif
    netif->linkoutput = NULL;
    return ERR_OK;
}

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
    return tinyusb_net_send_sync(buffer, (uint16_t)len, NULL, pdMS_TO_TICKS(5000));
}

void usb_net_free_rx_buffer(void *driver_handle, void *buffer) {}
