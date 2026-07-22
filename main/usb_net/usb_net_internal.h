#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/* ── IP / DHCP configuration ───────────────────────────────── */

#define USB_NET_IP_OCTET1   192
#define USB_NET_IP_OCTET2   168
#define USB_NET_IP_OCTET3   7
#define USB_NET_IP_OCTET4   1

#define USB_NET_NETMASK_O1  255
#define USB_NET_NETMASK_O2  255
#define USB_NET_NETMASK_O3  255
#define USB_NET_NETMASK_O4  0

#define USB_NET_DHCP_START_O4      2
#define USB_NET_DHCP_END_O4       10
#define USB_NET_DHCP_LEASE_MINUTES 1440
#define USB_NET_MTU               1500

/* ── lwIP / driver helpers ─────────────────────────────────── */

esp_err_t  usb_net_lwip_input(void *netif_handle, void *buffer, size_t len, void *l2_buff);
esp_err_t  usb_net_transmit(void *driver_handle, void *buffer, size_t len);
void       usb_net_free_rx_buffer(void *driver_handle, void *buffer);

/* ── TinyUSB NCM callbacks (defined in usb_net_init.c) ─────── */

esp_err_t  usb_net_tinyusb_recv_cb(void *buffer, uint16_t len, void *ctx);
void       usb_net_tinyusb_init_cb(void *ctx);
void       usb_net_tinyusb_free_tx(void *buffer, void *ctx);
