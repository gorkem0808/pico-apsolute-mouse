#include "usb_descriptors.h"
#include <string.h>

#ifndef USB_PRODUCT
#define USB_PRODUCT "GT Arcade Kontrol IO V001"
#endif

#define USB_VID 0xCafe
#define USB_PID 0x4021

// HID: absolute pointer + keyboard. CDC: PC calibration app communication.
uint8_t const desc_hid_report[] = {
  // Mouse / pointer with absolute X/Y 0..32767
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x02,       // Usage (Mouse)
  0xA1, 0x01,       // Collection (Application)
  0x85, REPORT_ID_MOUSE,
  0x09, 0x01,       // Usage (Pointer)
  0xA1, 0x00,       // Collection (Physical)
  0x05, 0x09,       // Usage Page (Button)
  0x19, 0x01,       // Usage Minimum (1)
  0x29, 0x05,       // Usage Maximum (5)
  0x15, 0x00,       // Logical Minimum (0)
  0x25, 0x01,       // Logical Maximum (1)
  0x95, 0x05,       // Report Count (5)
  0x75, 0x01,       // Report Size (1)
  0x81, 0x02,       // Input (Data,Var,Abs)
  0x95, 0x01,       // Report Count (1)
  0x75, 0x03,       // Report Size (3)
  0x81, 0x03,       // Input (Cnst,Var,Abs)
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x30,       // Usage (X)
  0x09, 0x31,       // Usage (Y)
  0x16, 0x00, 0x00, // Logical Minimum (0)
  0x26, 0xFF, 0x7F, // Logical Maximum (32767)
  0x75, 0x10,       // Report Size (16)
  0x95, 0x02,       // Report Count (2)
  0x81, 0x02,       // Input (Data,Var,Abs)
  0xC0,
  0xC0,

  // Keyboard report, boot compatible style
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x06,       // Usage (Keyboard)
  0xA1, 0x01,       // Collection (Application)
  0x85, REPORT_ID_KEYBOARD,
  0x05, 0x07,       // Usage Page (Key Codes)
  0x19, 0xE0,
  0x29, 0xE7,
  0x15, 0x00,
  0x25, 0x01,
  0x75, 0x01,
  0x95, 0x08,
  0x81, 0x02,       // modifiers
  0x95, 0x01,
  0x75, 0x08,
  0x81, 0x01,       // reserved
  0x95, 0x06,
  0x75, 0x08,
  0x15, 0x00,
  0x25, 0x65,
  0x05, 0x07,
  0x19, 0x00,
  0x29, 0x65,
  0x81, 0x00,
  0xC0
};

enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)
#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x82
#define EPNUM_HID       0x83

uint8_t const desc_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, 16, 1)
};

uint8_t const desc_device[] = {
  18, TUSB_DESC_DEVICE, 0x00, 0x02,
  TUSB_CLASS_MISC, MISC_SUBCLASS_COMMON, MISC_PROTOCOL_IAD, CFG_TUD_ENDPOINT0_SIZE,
  (uint8_t)(USB_VID & 0xff), (uint8_t)(USB_VID >> 8),
  (uint8_t)(USB_PID & 0xff), (uint8_t)(USB_PID >> 8),
  0x01, 0x01,
  0x01, 0x02, 0x03, 0x01
};

uint8_t const * tud_descriptor_device_cb(void) { return desc_device; }
uint8_t const * tud_descriptor_configuration_cb(uint8_t index) { (void) index; return desc_configuration; }
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) { (void) instance; return desc_hid_report; }

char const* string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 },
  "GT Arcade",
  USB_PRODUCT,
  "V001",
  "GT Arcade CDC"
};

static uint16_t _desc_str[64];
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;
  uint8_t chr_count;
  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) return NULL;
    const char* str = string_desc_arr[index];
    chr_count = strlen(str);
    if (chr_count > 31) chr_count = 31;
    for (uint8_t i=0; i<chr_count; i++) _desc_str[1+i] = str[i];
  }
  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
  return _desc_str;
}
