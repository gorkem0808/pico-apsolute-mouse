#pragma once
#include "tusb.h"

enum {
  REPORT_ID_MOUSE = 1,
  REPORT_ID_KEYBOARD = 2,
};

uint8_t const * tud_descriptor_device_cb(void);
uint8_t const * tud_descriptor_configuration_cb(uint8_t index);
uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid);
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance);
