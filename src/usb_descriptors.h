#pragma once
#include "tusb.h"

enum {
    REPORT_ID_MOUSE = 1,
    REPORT_ID_KEYBOARD = 2,
};

typedef struct __attribute__((packed)) {
    uint8_t buttons;
    uint16_t x;
    uint16_t y;
} abs_mouse_report_t;
