#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

#include <stdint.h>

typedef struct
{
  uint16_t vendor_id;
  char *vendor_name;
  uint16_t product_id;
  char *product_name;
} usb_device_id;

int8_t get_host_nrf_status(usb_device_id *device_id, uint8_t *status);
int8_t transfer_command1(usb_device_id *device_id, uint16_t data, uint8_t *buffer, uint8_t buffer_size);

#endif
