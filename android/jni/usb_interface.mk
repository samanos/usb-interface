LOCAL_PATH:= $(call my-dir)

# usb_interface

include $(CLEAR_VARS)

ROOT_REL:= ../..
ROOT_ABS:= $(LOCAL_PATH)/../..

VENDOR_LIBUSB := $(ROOT_ABS)/vendor/libusb

LOCAL_SRC_FILES := $(ROOT_REL)/src/usb_interface.c

LOCAL_C_INCLUDES += \
	$(ROOT_ABS)/src \
	$(ROOT_ABS)/vendor

LOCAL_SHARED_LIBRARIES += libusb1.0

LOCAL_MODULE := usb_interface

LOCAL_CFLAGS += -std=c99

include $(BUILD_SHARED_LIBRARY)

include $(VENDOR_LIBUSB)/android/jni/libusb.mk
