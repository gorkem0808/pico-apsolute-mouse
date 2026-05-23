#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

// PINLER
#define PIN_X_ADC       26   // GP26 = ADC0, X potans orta ucu
#define PIN_Y_ADC       27   // GP27 = ADC1, Y potans orta ucu
#define PIN_ENABLE      20   // GP20 -> GND = AKTIF
#define PIN_TRIGGER     14   // GP14 -> buton -> GND = sol tik
#define PIN_BUTTON2     15   // GP15 -> buton -> GND = sag tik

// AYARLAR
#define ADC_MIN_DEFAULT  120
#define ADC_MAX_DEFAULT  3975
#define HID_MAX          32767
#define SEND_INTERVAL_MS 5

// Y ekseni tersse 1/0 değiştir.
#define INVERT_X 0
#define INVERT_Y 0

// Filtre: 0 kapali, 1..8 yumuşatma. Büyük değer daha sakin ama daha yavaş.
#define FILTER_SHIFT 2

typedef struct __attribute__((packed)) {
    uint8_t buttons;
    uint16_t x;
    uint16_t y;
} abs_mouse_report_t;

static uint32_t filt_x = 0;
static uint32_t filt_y = 0;
static bool filter_ready = false;

static uint16_t clamp_u16_i32(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return (uint16_t)lo;
    if (v > hi) return (uint16_t)hi;
    return (uint16_t)v;
}

static uint16_t map_adc_to_hid(uint16_t raw, bool invert) {
    int32_t v = raw;
    if (v < ADC_MIN_DEFAULT) v = ADC_MIN_DEFAULT;
    if (v > ADC_MAX_DEFAULT) v = ADC_MAX_DEFAULT;
    int32_t out = (v - ADC_MIN_DEFAULT) * HID_MAX / (ADC_MAX_DEFAULT - ADC_MIN_DEFAULT);
    if (invert) out = HID_MAX - out;
    return clamp_u16_i32(out, 0, HID_MAX);
}

static uint16_t read_adc_channel(uint channel) {
    adc_select_input(channel);
    sleep_us(10);
    // 8 örnek ortalama: potans titremesini azaltır.
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += adc_read();
        sleep_us(50);
    }
    return (uint16_t)(sum / 8);
}

static void send_abs_mouse(void) {
    if (!tud_hid_ready()) return;

    bool enabled = (gpio_get(PIN_ENABLE) == 0); // GP20 GND = aktif
    if (!enabled) return;

    uint16_t rx = read_adc_channel(0); // GP26
    uint16_t ry = read_adc_channel(1); // GP27

    if (!filter_ready) {
        filt_x = rx;
        filt_y = ry;
        filter_ready = true;
    } else {
        filt_x = filt_x + (((int32_t)rx - (int32_t)filt_x) >> FILTER_SHIFT);
        filt_y = filt_y + (((int32_t)ry - (int32_t)filt_y) >> FILTER_SHIFT);
    }

    uint16_t x = map_adc_to_hid((uint16_t)filt_x, INVERT_X);
    uint16_t y = map_adc_to_hid((uint16_t)filt_y, INVERT_Y);

    uint8_t buttons = 0;
    if (gpio_get(PIN_TRIGGER) == 0) buttons |= 0x01; // sol tik
    if (gpio_get(PIN_BUTTON2) == 0) buttons |= 0x02; // sag tik

    abs_mouse_report_t rpt = {
        .buttons = buttons,
        .x = x,
        .y = y
    };

    tud_hid_report(REPORT_ID_MOUSE, &rpt, sizeof(rpt));
}

int main(void) {
    board_init();
    tusb_init();

    adc_init();
    adc_gpio_init(PIN_X_ADC);
    adc_gpio_init(PIN_Y_ADC);

    gpio_init(PIN_ENABLE);
    gpio_set_dir(PIN_ENABLE, GPIO_IN);
    gpio_pull_up(PIN_ENABLE);

    gpio_init(PIN_TRIGGER);
    gpio_set_dir(PIN_TRIGGER, GPIO_IN);
    gpio_pull_up(PIN_TRIGGER);

    gpio_init(PIN_BUTTON2);
    gpio_set_dir(PIN_BUTTON2, GPIO_IN);
    gpio_pull_up(PIN_BUTTON2);

    uint32_t last_ms = 0;
    while (1) {
        tud_task();
        uint32_t now = board_millis();
        if (now - last_ms >= SEND_INTERVAL_MS) {
            last_ms = now;
            send_abs_mouse();
        }
    }
}
