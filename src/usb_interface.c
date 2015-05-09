#include <string.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>

#include "protocol.h"
#include "usb_interface.h"

#define VND_IN  (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN)
#define STD_IN  (LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN)

/* Queries USB device for string descriptor.
 * Return values:
 *  -1 - unable to open the device
 *  -2 - unable to get descriptor
 *  -3 - invalid return type
 */
static int get_descriptor(libusb_device *device, int index, int langid, char *buf, int buflen) {
  unsigned char buffer[256];
  int rval;

  libusb_device_handle *handle = NULL;
  rval = libusb_open(device, &handle);
  if (rval)
    return -1;

  // make standard request GET_DESCRIPTOR, type string and given index
  // (e.g. dev->iProduct)
  rval = libusb_control_transfer(handle, STD_IN, LIBUSB_REQUEST_GET_DESCRIPTOR,
    (LIBUSB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000);
  libusb_close(handle);

  if (rval < 0)
    return -2;

  // rval should be bytes read, but buffer[0] contains the actual response size
  if ((unsigned char)buffer[0] < rval)
    rval = (unsigned char)buffer[0]; // string is shorter than bytes read

  if (buffer[1] != LIBUSB_DT_STRING) // second byte is the data type
    return -3;

  // we're dealing with UTF-16LE here so actual chars is half of rval,
  // and index 0 doesn't count
  rval /= 2;

  // lossy conversion to ISO Latin1
  int i;
  for (i = 1; i < rval && i < buflen; i++) {
    if (buffer[2 * i + 1] == 0)
      buf[i - 1] = buffer[2 * i];
    else
      buf[i - 1] = '?'; // outside of ISO Latin1 range
  }

  buf[i - 1] = 0;
  return i - 1;
}

static uint8_t is_interesting(libusb_device *device, usb_device_id *device_id) {
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(device, &descriptor);
  if (descriptor.idVendor != device_id->vendor_id || descriptor.idProduct != device_id->product_id)
    return 0;

  char device_vendor[256];
  if (get_descriptor(device, descriptor.iManufacturer, 0x0409, device_vendor, sizeof(device_vendor)) < 0)
    return 0;
  if (strcmp(device_vendor, device_id->vendor_name) != 0)
    return 0;

  char device_product[256];
  if (get_descriptor(device, descriptor.iProduct, 0x0409, device_product, sizeof(device_product)) < 0)
    return 0;
  if (strcmp(device_product, device_id->product_name) != 0)
    return 0;

  return 1;
}

/* Finds device by vendor and product names.
 * If zero is returned, allocates new reference for the found usb device,
 * in which case you must dereference usb device when you are done with it
 * by calling libusb_unref_device(device)
 *
 * Return values:
 *   0 - device found
 *  -1 - unable to initialize libusb
 *  -2 - unable to get device list
 *  -3 - device not found
 */
static int8_t device_by_id(libusb_device **found, usb_device_id *device_id) {
  int err = libusb_init(NULL);
  if (err)
		return -1;

  libusb_device **list;
  ssize_t cnt = libusb_get_device_list(NULL, &list);
  if (cnt < 0)
    return -2;

  *found = NULL;
  for (ssize_t i = 0; i < cnt; i++) {
    libusb_device *device = list[i];
    if (is_interesting(device, device_id)) {
      *found = libusb_ref_device(device);
      break;
    }
  }

  libusb_free_device_list(list, 1);

  if (*found != NULL)
    return 0;
  else
    return -3;
}

/* Gets NRF status of the host.
 * Return values:
 *  -1 - device not found
 *  -2 - unable to open device
 */
int8_t get_host_nrf_status(usb_device_id *device_id, uint8_t *status) {

  libusb_device *device;
  if (device_by_id(&device, device_id))
    return -1;

  libusb_device_handle *handle = NULL;
  if (libusb_open(device, &handle))
    return -2;

  unsigned char buffer[256];
  libusb_control_transfer(handle, VND_IN, USB_NRF_STATUS, 0, 0, buffer, sizeof(buffer), 1000);
  libusb_close(handle);
  libusb_unref_device(device);

  *status = buffer[0];
  return 0;
}

int8_t transfer_command1(usb_device_id *device_id, uint16_t data, uint8_t *buffer, uint8_t buffer_size) {

  libusb_device *device;
  if (device_by_id(&device, device_id))
    return -1;

  libusb_device_handle *handle = NULL;
  if (libusb_open(device, &handle))
    return -2;

  int8_t reply_len = libusb_control_transfer(handle, VND_IN, USB_NRF_TRANSFER1, data, 0, buffer, buffer_size, 1000);
  libusb_close(handle);
  libusb_unref_device(device);

  return reply_len;
}
