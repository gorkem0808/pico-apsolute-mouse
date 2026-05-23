#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

#define PIN_X 26
#define PIN_Y 27
#define PIN_UP 2
#define PIN_DOWN 3
#define PIN_LEFT 4
#define PIN_RIGHT 5
#define PIN_BOMB_SELECT 6
#define PIN_TRIGGER 7
#define PIN_RELOAD 8
#define PIN_COIN 17
#define PIN_START 18
#define PIN_TEST 19
#define PIN_ENABLE 20
#define LED_PIN 25

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif
#define SETTINGS_MAGIC 0x47545632u
#define SETTINGS_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

#if PLAYER_NUM == 2
#define PLAYER_NAME "P2"
#else
#define PLAYER_NAME "P1"
#endif

typedef struct __attribute__((packed)) {
  uint8_t buttons;
  int16_t x;
  int16_t y;
} abs_mouse_report_t;

typedef struct {
  uint32_t magic;
  int deadzone;
  int smoothing;
  int samples;
  int min_move;
  int invert_x;
  int invert_y;
  int x_min;
  int x_max;
  int y_min;
  int y_max;
} settings_t;

static settings_t cfg = {SETTINGS_MAGIC, 8, 80, 24, 4, 0, 0, 0, 4095, 0, 4095};
static int filt_x = 16384, filt_y = 16384, last_x = -9999, last_y = -9999;
static char linebuf[128];
static int linepos = 0;
static void settings_default(void) {
  cfg.magic = SETTINGS_MAGIC;
  cfg.deadzone = 8;
  cfg.smoothing = 80;
  cfg.samples = 24;
  cfg.min_move = 4;
  cfg.invert_x = 0;
  cfg.invert_y = 0;
  cfg.x_min = 0;
  cfg.x_max = 4095;
  cfg.y_min = 0;
  cfg.y_max = 4095;
}

static void settings_load(void) {
  const settings_t *saved = (const settings_t *)(XIP_BASE + SETTINGS_FLASH_OFFSET);
  if (saved->magic == SETTINGS_MAGIC && saved->samples >= 1 && saved->samples <= 64 && saved->x_min != saved->x_max && saved->y_min != saved->y_max) {
    cfg = *saved;
  } else {
    settings_default();
  }
}

static void settings_save(void) {
  uint8_t page[FLASH_PAGE_SIZE];
  memset(page, 0xFF, sizeof(page));
  cfg.magic = SETTINGS_MAGIC;
  memcpy(page, &cfg, sizeof(cfg));
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(SETTINGS_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(SETTINGS_FLASH_OFFSET, page, FLASH_PAGE_SIZE);
  restore_interrupts(ints);
}


static void init_pin_pullup(uint pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_up(pin);
}

static bool pressed(uint pin) { return gpio_get(pin) == 0; }

static uint16_t adc_read_avg(uint channel, int samples) {
  if (samples < 1) samples = 1;
  if (samples > 64) samples = 64;
  adc_select_input(channel);
  uint32_t sum = 0;
  for (int i=0; i<samples; i++) sum += adc_read();
  return (uint16_t)(sum / samples);
}

static int map_range(int v, int in_min, int in_max, int out_min, int out_max) {
  if (in_max == in_min) return (out_min + out_max)/2;
  if (in_min > in_max) { int t=in_min; in_min=in_max; in_max=t; }
  if (v < in_min) v = in_min;
  if (v > in_max) v = in_max;
  return out_min + (int)((int64_t)(v - in_min) * (out_max - out_min) / (in_max - in_min));
}

static int apply_filter(int current, int target) {
  int s = cfg.smoothing;
  if (s < 0) s = 0;
  if (s > 95) s = 95;
  return (current * s + target * (100 - s)) / 100;
}

static void send_cdc_status(uint16_t rawx, uint16_t rawy, int cx, int cy, uint32_t buttons) {
  if (!tud_cdc_connected()) return;
  static absolute_time_t last;
  if (absolute_time_diff_us(last, get_absolute_time()) < 30000) return;
  last = get_absolute_time();
  char out[220];
  snprintf(out, sizeof(out), "DATA,%s,%u,%u,%d,%d,%lu\n", PLAYER_NAME, rawx, rawy, cx, cy, (unsigned long)buttons);
  tud_cdc_write_str(out);
  tud_cdc_write_flush();
}

static void handle_cmd(char *cmd) {
  if (strncmp(cmd, "SET,", 4) == 0) {
    int dz, sm, sam, mm;
    if (sscanf(cmd+4, "%d,%d,%d,%d", &dz, &sm, &sam, &mm) == 4) {
      cfg.deadzone = dz; cfg.smoothing = sm; cfg.samples = sam; cfg.min_move = mm;
      settings_save();
      tud_cdc_write_str("OK,SET SAVED\n"); tud_cdc_write_flush();
    }
  } else if (strncmp(cmd, "CAL,", 4) == 0) {
    int xmin, xmax, ymin, ymax;
    if (sscanf(cmd+4, "%d,%d,%d,%d", &xmin, &xmax, &ymin, &ymax) == 4) {
      cfg.x_min=xmin; cfg.x_max=xmax; cfg.y_min=ymin; cfg.y_max=ymax;
      settings_save();
      tud_cdc_write_str("OK,CAL SAVED\n"); tud_cdc_write_flush();
    }
  } else if (strcmp(cmd, "PING") == 0) {
    tud_cdc_write_str("PONG," PLAYER_NAME ",V3.2\n"); tud_cdc_write_flush();
  } else if (strcmp(cmd, "OPEN_CAL") == 0) {
    tud_cdc_write_str("EVENT,CALIBRATE\n"); tud_cdc_write_flush();
  }
}

static void poll_cdc(void) {
  while (tud_cdc_available()) {
    char c = (char)tud_cdc_read_char();
    if (c == '\r') continue;
    if (c == '\n') {
      linebuf[linepos] = 0;
      handle_cmd(linebuf);
      linepos = 0;
    } else if (linepos < (int)sizeof(linebuf)-1) {
      linebuf[linepos++] = c;
    }
  }
}

static void send_keyboard(uint32_t buttons, bool active) {
  static uint32_t last_buttons = 0;
  if (!active) buttons = 0;
  if (!tud_hid_ready() || buttons == last_buttons) return;
  last_buttons = buttons;
  uint8_t keycode[6] = {0};
  uint8_t mod = 0;
  int n = 0;
#if PLAYER_NUM == 2
  if (buttons & (1u<<PIN_RELOAD)) keycode[n++] = HID_KEY_T;
  if (buttons & (1u<<PIN_BOMB_SELECT)) { mod |= KEYBOARD_MODIFIER_RIGHTCTRL; }
  if (buttons & (1u<<PIN_COIN)) keycode[n++] = HID_KEY_KEYPAD_6;
  if (buttons & (1u<<PIN_START)) keycode[n++] = HID_KEY_KEYPAD_2;
#else
  if (buttons & (1u<<PIN_UP)) keycode[n++] = HID_KEY_ARROW_UP;
  if (buttons & (1u<<PIN_DOWN)) keycode[n++] = HID_KEY_ARROW_DOWN;
  if (buttons & (1u<<PIN_LEFT)) keycode[n++] = HID_KEY_ARROW_LEFT;
  if (buttons & (1u<<PIN_RIGHT)) keycode[n++] = HID_KEY_ARROW_RIGHT;
  if (buttons & (1u<<PIN_RELOAD)) keycode[n++] = HID_KEY_R;
  if (buttons & (1u<<PIN_BOMB_SELECT)) keycode[n++] = HID_KEY_SPACE;
  if (buttons & (1u<<PIN_COIN)) keycode[n++] = HID_KEY_KEYPAD_5;
  if (buttons & (1u<<PIN_START)) keycode[n++] = HID_KEY_KEYPAD_1;
  if (buttons & (1u<<PIN_TEST)) keycode[n++] = HID_KEY_KEYPAD_7;
#endif
  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, mod, keycode);
}

int main(void) {
  board_init();
  settings_load();
  tusb_init();
  adc_init();
  adc_gpio_init(PIN_X);
  adc_gpio_init(PIN_Y);
  gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT);

  uint pins[] = {PIN_UP,PIN_DOWN,PIN_LEFT,PIN_RIGHT,PIN_BOMB_SELECT,PIN_TRIGGER,PIN_RELOAD,PIN_COIN,PIN_START,PIN_TEST,PIN_ENABLE};
  for (int i=0; i<(int)(sizeof(pins)/sizeof(pins[0])); i++) init_pin_pullup(pins[i]);

  while (true) {
    tud_task();
    poll_cdc();

    uint16_t rawx = adc_read_avg(0, cfg.samples);
    uint16_t rawy = adc_read_avg(1, cfg.samples);
    int mx = map_range(rawx, cfg.x_min, cfg.x_max, 0, 32767);
    int my = map_range(rawy, cfg.y_min, cfg.y_max, 0, 32767);
    if (cfg.invert_x) mx = 32767 - mx;
    if (cfg.invert_y) my = 32767 - my;
    filt_x = apply_filter(filt_x, mx);
    filt_y = apply_filter(filt_y, my);

    if (last_x < 0) { last_x = filt_x; last_y = filt_y; }
    int outx = last_x;
    int outy = last_y;
    if (abs(filt_x - last_x) > cfg.deadzone || abs(filt_x - last_x) > cfg.min_move) { outx = filt_x; last_x = filt_x; }
    if (abs(filt_y - last_y) > cfg.deadzone || abs(filt_y - last_y) > cfg.min_move) { outy = filt_y; last_y = filt_y; }

    uint32_t buttons = 0;
    uint8_t mouse_buttons = 0;
    for (int i=0; i<(int)(sizeof(pins)/sizeof(pins[0])); i++) if (pressed(pins[i])) buttons |= (1u << pins[i]);
    bool system_active = pressed(PIN_ENABLE);
    if (system_active && pressed(PIN_TRIGGER)) mouse_buttons |= 0x01;
    if (system_active && pressed(PIN_BOMB_SELECT)) mouse_buttons |= 0x02;

    gpio_put(LED_PIN, system_active);

    if (tud_hid_ready()) {
      if (system_active) {
        abs_mouse_report_t rpt = {.buttons = mouse_buttons, .x = (int16_t)outx, .y = (int16_t)outy};
        tud_hid_report(REPORT_ID_MOUSE, &rpt, sizeof(rpt));
      } else {
        abs_mouse_report_t rpt = {.buttons = mouse_buttons, .x = (int16_t)last_x, .y = (int16_t)last_y};
        tud_hid_report(REPORT_ID_MOUSE, &rpt, sizeof(rpt));
      }
      send_keyboard(buttons, system_active);
    }
    if (pressed(PIN_TEST)) {
      static absolute_time_t last_event;
      if (absolute_time_diff_us(last_event, get_absolute_time()) > 1000000) {
        last_event = get_absolute_time();
        if (tud_cdc_connected()) { tud_cdc_write_str("EVENT,CALIBRATE\n"); tud_cdc_write_flush(); }
      }
    }
    send_cdc_status(rawx, rawy, outx, outy, buttons);
    sleep_ms(5);
  }
}
