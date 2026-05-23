#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

#ifndef PLAYER_ID
#define PLAYER_ID 1
#endif

#define PIN_MENU_UP 2
#define PIN_MENU_DOWN 3
#define PIN_MENU_LEFT 4
#define PIN_MENU_RIGHT 5
#define PIN_BOMB_SELECT 6
#define PIN_TRIGGER 7
#define PIN_RELOAD 8
#define PIN_COIN 17
#define PIN_START 18
#define PIN_TEST_CAL 19
#define PIN_ACTIVE 20
#define ADC_X_GPIO 26
#define ADC_Y_GPIO 27
#define ADC_X_CH 0
#define ADC_Y_CH 1

#define HID_KEY_NONE 0x00
#define HID_KEY_R 0x15
#define HID_KEY_T 0x17
#define HID_KEY_SPACE 0x2C
#define HID_KEY_RIGHT_CTRL 0xE4
#define HID_KEY_ARROW_RIGHT 0x4F
#define HID_KEY_ARROW_LEFT 0x50
#define HID_KEY_ARROW_DOWN 0x51
#define HID_KEY_ARROW_UP 0x52
#define HID_KEY_KEYPAD_1 0x59
#define HID_KEY_KEYPAD_2 0x5A
#define HID_KEY_KEYPAD_5 0x5D
#define HID_KEY_KEYPAD_6 0x5E
#define HID_KEY_KEYPAD_7 0x5F
#define HID_KEY_KEYPAD_8 0x60

typedef struct {
  uint16_t x_min, x_max, y_min, y_max;
  uint8_t filter; // 0..100
  uint32_t magic;
} config_t;

static config_t cfg = { 200, 3900, 200, 3900, 60, 0x47544131 };
static uint16_t last_x = 16384, last_y = 16384;
static bool last_cal = false;

static void setup_pin(uint pin) {
  gpio_init(pin); gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin);
}
static bool pressed(uint pin) { return !gpio_get(pin); }

static uint16_t adc_avg(uint ch, uint samples) {
  adc_select_input(ch);
  uint32_t sum = 0;
  for (uint i=0; i<samples; i++) { sum += adc_read(); sleep_us(80); }
  return (uint16_t)(sum / samples);
}

static uint16_t map_adc(uint16_t v, uint16_t mn, uint16_t mx) {
  if (mx <= mn + 10) return 16384;
  if (v < mn) v = mn;
  if (v > mx) v = mx;
  uint32_t out = ((uint32_t)(v - mn) * 32767u) / (uint32_t)(mx - mn);
  return (uint16_t)out;
}

static uint16_t filter_value(uint16_t oldv, uint16_t newv) {
  uint8_t f = cfg.filter;
  if (f > 95) f = 95;
  int diff = (int)newv - (int)oldv;
  int dead = 20 + (int)f * 4;
  if (diff > -dead && diff < dead) return oldv;
  uint32_t keep = f;
  uint32_t take = 100 - f;
  return (uint16_t)(((uint32_t)oldv * keep + (uint32_t)newv * take) / 100u);
}

static void send_status_line(void) {
  if (!tud_cdc_connected()) return;
  uint16_t rawx = adc_avg(ADC_X_CH, 8);
  uint16_t rawy = adc_avg(ADC_Y_CH, 8);
  char buf[256];
  snprintf(buf, sizeof(buf),
    "GTIO,P%d,RAW,%u,%u,CAL,%u,%u,BTN,%d%d%d%d%d%d%d%d%d%d%d,FILTER,%u\n",
    PLAYER_ID, rawx, rawy, last_x, last_y,
    pressed(PIN_MENU_UP), pressed(PIN_MENU_DOWN), pressed(PIN_MENU_LEFT), pressed(PIN_MENU_RIGHT),
    pressed(PIN_BOMB_SELECT), pressed(PIN_TRIGGER), pressed(PIN_RELOAD), pressed(PIN_COIN),
    pressed(PIN_START), pressed(PIN_TEST_CAL), pressed(PIN_ACTIVE), cfg.filter);
  tud_cdc_write_str(buf);
  tud_cdc_write_flush();
}

static void parse_cdc(void) {
  static char line[128]; static uint8_t pos = 0;
  while (tud_cdc_available()) {
    char c; tud_cdc_read(&c, 1);
    if (c == '\n' || c == '\r') {
      line[pos] = 0; pos = 0;
      if (strncmp(line, "SETCAL,", 7) == 0) {
        uint x1,x2,y1,y2;
        if (sscanf(line+7, "%u,%u,%u,%u", &x1,&x2,&y1,&y2) == 4) {
          cfg.x_min=x1; cfg.x_max=x2; cfg.y_min=y1; cfg.y_max=y2;
          tud_cdc_write_str("OK,CAL\n"); tud_cdc_write_flush();
        }
      } else if (strncmp(line, "FILTER,", 7) == 0) {
        int f = atoi(line+7); if (f<0) f=0; if (f>100) f=100; cfg.filter = (uint8_t)f;
        tud_cdc_write_str("OK,FILTER\n"); tud_cdc_write_flush();
      } else if (strcmp(line, "PING") == 0) {
        char b[32]; snprintf(b,sizeof(b),"PONG,P%d\n", PLAYER_ID); tud_cdc_write_str(b); tud_cdc_write_flush();
      }
    } else if (pos < sizeof(line)-1) line[pos++] = c;
  }
}

static void hid_keyboard_send(void) {
  if (!tud_hid_ready()) return;
  uint8_t keycode[6] = {0};
  uint8_t mod = 0;
  uint8_t n = 0;

  bool active = pressed(PIN_ACTIVE);
  // Menu/test keys are allowed even if system is not active.
  if (pressed(PIN_MENU_UP) && n<6) keycode[n++] = HID_KEY_ARROW_UP;
  if (pressed(PIN_MENU_DOWN) && n<6) keycode[n++] = HID_KEY_ARROW_DOWN;
  if (pressed(PIN_MENU_LEFT) && n<6) keycode[n++] = HID_KEY_ARROW_LEFT;
  if (pressed(PIN_MENU_RIGHT) && n<6) keycode[n++] = HID_KEY_ARROW_RIGHT;
  if (pressed(PIN_TEST_CAL) && n<6) keycode[n++] = (PLAYER_ID==1) ? HID_KEY_KEYPAD_7 : HID_KEY_KEYPAD_8;
  if (pressed(PIN_COIN) && n<6) keycode[n++] = (PLAYER_ID==1) ? HID_KEY_KEYPAD_5 : HID_KEY_KEYPAD_6;
  if (pressed(PIN_START) && n<6) keycode[n++] = (PLAYER_ID==1) ? HID_KEY_KEYPAD_1 : HID_KEY_KEYPAD_2;

  if (active) {
    if (pressed(PIN_BOMB_SELECT) && n<6) keycode[n++] = (PLAYER_ID==1) ? HID_KEY_SPACE : HID_KEY_RIGHT_CTRL;
    if (pressed(PIN_RELOAD) && n<6) keycode[n++] = (PLAYER_ID==1) ? HID_KEY_R : HID_KEY_T;
  }
  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, mod, keycode);
}

static void hid_pointer_send(void) {
  if (!tud_hid_ready()) return;
  uint16_t rawx = adc_avg(ADC_X_CH, 16);
  uint16_t rawy = adc_avg(ADC_Y_CH, 16);
  uint16_t x = map_adc(rawx, cfg.x_min, cfg.x_max);
  uint16_t y = map_adc(rawy, cfg.y_min, cfg.y_max);
  x = filter_value(last_x, x);
  y = filter_value(last_y, y);
  last_x = x; last_y = y;

  uint8_t buttons = 0;
  if (pressed(PIN_ACTIVE)) {
    if (pressed(PIN_TRIGGER)) buttons |= 0x01; // left
    if (pressed(PIN_BOMB_SELECT)) buttons |= 0x02; // right
  }
  uint8_t report[5];
  report[0] = buttons;
  report[1] = x & 0xFF; report[2] = x >> 8;
  report[3] = y & 0xFF; report[4] = y >> 8;
  tud_hid_report(REPORT_ID_MOUSE, report, sizeof(report));
}

int main(void) {
  board_init();
  stdio_init_all();
  tusb_init();
  adc_init(); adc_gpio_init(ADC_X_GPIO); adc_gpio_init(ADC_Y_GPIO);
  setup_pin(PIN_MENU_UP); setup_pin(PIN_MENU_DOWN); setup_pin(PIN_MENU_LEFT); setup_pin(PIN_MENU_RIGHT);
  setup_pin(PIN_BOMB_SELECT); setup_pin(PIN_TRIGGER); setup_pin(PIN_RELOAD);
  setup_pin(PIN_COIN); setup_pin(PIN_START); setup_pin(PIN_TEST_CAL); setup_pin(PIN_ACTIVE);

  uint32_t last_hid = 0, last_status = 0;
  while (true) {
    tud_task();
    parse_cdc();
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_hid >= 5) { last_hid = now; hid_pointer_send(); hid_keyboard_send(); }
    if (now - last_status >= 50) { last_status = now; send_status_line(); }

    bool cal = pressed(PIN_TEST_CAL);
    if (cal && !last_cal && tud_cdc_connected()) {
      char b[32]; snprintf(b,sizeof(b),"CALREQ,P%d\n", PLAYER_ID); tud_cdc_write_str(b); tud_cdc_write_flush();
    }
    last_cal = cal;
  }
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen; return 0;
}
