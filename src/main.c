#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "tusb.h"
#include "pins.h"
#include "calibration.h"
#include "usb_descriptors.h"

#ifndef PLAYER_ID
#define PLAYER_ID 1
#endif

#define BTN_UP_BIT       (1u << 0)
#define BTN_DOWN_BIT     (1u << 1)
#define BTN_LEFT_BIT     (1u << 2)
#define BTN_RIGHT_BIT    (1u << 3)
#define BTN_SELECT_BIT   (1u << 4)
#define BTN_TRIGGER_BIT  (1u << 5)
#define BTN_RELOAD_BIT   (1u << 6)
#define BTN_COIN_BIT     (1u << 7)
#define BTN_START_BIT    (1u << 8)
#define BTN_TEST_BIT     (1u << 9)
#define BTN_AIM_BIT      (1u << 10)

static calib_t g_cal;
static uint16_t g_raw_x, g_raw_y, g_x, g_y;
static uint16_t g_mouse_x = 16384, g_mouse_y = 16384;
static uint32_t g_buttons, g_prev_buttons;
static absolute_time_t g_last_state_tx;
static char g_cmd[96];
static int g_cmd_len = 0;

static inline bool pressed(uint pin) { return gpio_get(pin) == 0; }

static void init_button(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

static uint16_t read_adc_channel(uint ch) {
    adc_select_input(ch);
    sleep_us(25);
    return adc_read();
}

static void read_inputs(void) {
    g_raw_x = read_adc_channel(ADC_CH_X);
    g_raw_y = read_adc_channel(ADC_CH_Y);
    g_x = calib_map_x(&g_cal, g_raw_x);
    g_y = calib_map_y(&g_cal, g_raw_y);

    uint32_t b = 0;
    if (pressed(PIN_MENU_UP)) b |= BTN_UP_BIT;
    if (pressed(PIN_MENU_DOWN)) b |= BTN_DOWN_BIT;
    if (pressed(PIN_MENU_LEFT)) b |= BTN_LEFT_BIT;
    if (pressed(PIN_MENU_RIGHT)) b |= BTN_RIGHT_BIT;
    if (pressed(PIN_GRENADE_SEL)) b |= BTN_SELECT_BIT;
    if (pressed(PIN_TRIGGER)) b |= BTN_TRIGGER_BIT;
    if (pressed(PIN_RELOAD)) b |= BTN_RELOAD_BIT;
    if (pressed(PIN_COIN)) b |= BTN_COIN_BIT;
    if (pressed(PIN_START)) b |= BTN_START_BIT;
    if (pressed(PIN_TEST_CAL)) b |= BTN_TEST_BIT;
    if (pressed(PIN_AIM_ENABLE)) b |= BTN_AIM_BIT;
    g_buttons = b;
}

static uint8_t keycode_for_button(uint32_t bit) {
    switch (bit) {
        case BTN_UP_BIT: return HID_KEY_ARROW_UP;
        case BTN_DOWN_BIT: return HID_KEY_ARROW_DOWN;
        case BTN_LEFT_BIT: return HID_KEY_ARROW_LEFT;
        case BTN_RIGHT_BIT: return HID_KEY_ARROW_RIGHT;
        case BTN_SELECT_BIT: return HID_KEY_SPACE;
        case BTN_RELOAD_BIT: return (PLAYER_ID == 1) ? HID_KEY_R : HID_KEY_T;
        case BTN_COIN_BIT: return (PLAYER_ID == 1) ? HID_KEY_KEYPAD_5 : HID_KEY_KEYPAD_6;
        case BTN_START_BIT: return (PLAYER_ID == 1) ? HID_KEY_KEYPAD_1 : HID_KEY_KEYPAD_2;
        case BTN_TEST_BIT: return HID_KEY_KEYPAD_7;
        default: return 0;
    }
}

static void send_hid_reports(void) {
    if (!tud_hid_ready()) return;

    uint8_t mouse_buttons = 0;
    if (g_buttons & BTN_TRIGGER_BIT) mouse_buttons |= 0x01; // left button
    if (g_buttons & BTN_SELECT_BIT) mouse_buttons |= 0x02;  // right button / grenade

    abs_mouse_report_t rpt = {0};
    rpt.buttons = mouse_buttons;
    if (g_buttons & BTN_AIM_BIT) {
        g_mouse_x = g_x;
        g_mouse_y = g_y;
    }
    rpt.x = g_mouse_x;
    rpt.y = g_mouse_y;
    tud_hid_report(REPORT_ID_MOUSE, &rpt, sizeof(rpt));

    uint8_t keys[6] = {0};
    int n = 0;
    const uint32_t key_bits[] = {
        BTN_UP_BIT, BTN_DOWN_BIT, BTN_LEFT_BIT, BTN_RIGHT_BIT, BTN_SELECT_BIT,
        BTN_RELOAD_BIT, BTN_COIN_BIT, BTN_START_BIT, BTN_TEST_BIT
    };
    for (unsigned i = 0; i < sizeof(key_bits)/sizeof(key_bits[0]) && n < 6; i++) {
        if (g_buttons & key_bits[i]) {
            uint8_t kc = keycode_for_button(key_bits[i]);
            if (kc) keys[n++] = kc;
        }
    }
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keys);
}

static void cdc_write_line(const char *s) {
    if (!tud_cdc_connected()) return;
    tud_cdc_write_str(s);
    tud_cdc_write_flush();
}

static void send_state(bool force) {
    if (!force && absolute_time_diff_us(g_last_state_tx, get_absolute_time()) < 50000) return;
    g_last_state_tx = get_absolute_time();
    char line[160];
    snprintf(line, sizeof(line),
        "STATE P=%d RX=%u RY=%u X=%u Y=%u B=%lu CAL=%u,%u,%u,%u,%u\n",
        PLAYER_ID, g_raw_x, g_raw_y, g_x, g_y, (unsigned long)g_buttons,
        g_cal.x_min, g_cal.x_max, g_cal.y_min, g_cal.y_max, g_cal.flags);
    cdc_write_line(line);
}

static void handle_command(char *cmd) {
    while (*cmd == ' ' || *cmd == '\r' || *cmd == '\n') cmd++;
    if (strncmp(cmd, "GETCAL", 6) == 0) {
        send_state(true);
        cdc_write_line("OK GETCAL\n");
    } else if (strncmp(cmd, "DEFAULT", 7) == 0) {
        calib_defaults(&g_cal);
        calib_save(&g_cal);
        cdc_write_line("OK DEFAULT\n");
    } else if (strncmp(cmd, "SETCAL", 6) == 0) {
        unsigned xmin, xmax, ymin, ymax, flags;
        if (sscanf(cmd + 6, "%u %u %u %u %u", &xmin, &xmax, &ymin, &ymax, &flags) == 5 && xmin < xmax && ymin < ymax) {
            g_cal.x_min = (uint16_t)xmin;
            g_cal.x_max = (uint16_t)xmax;
            g_cal.y_min = (uint16_t)ymin;
            g_cal.y_max = (uint16_t)ymax;
            g_cal.flags = (uint16_t)flags;
            calib_save(&g_cal);
            cdc_write_line("OK SETCAL SAVED\n");
        } else {
            cdc_write_line("ERR SETCAL FORMAT\n");
        }
    } else if (strncmp(cmd, "PING", 4) == 0) {
        cdc_write_line("OK PONG GT_ARCADE_PICO_PRO_V3\n");
    } else {
        cdc_write_line("ERR UNKNOWN\n");
    }
}

static void poll_cdc(void) {
    while (tud_cdc_available()) {
        char c = (char)tud_cdc_read_char();
        if (c == '\n' || c == '\r') {
            if (g_cmd_len > 0) {
                g_cmd[g_cmd_len] = 0;
                handle_command(g_cmd);
                g_cmd_len = 0;
            }
        } else if (g_cmd_len < (int)sizeof(g_cmd) - 1) {
            g_cmd[g_cmd_len++] = c;
        }
    }
}

int main(void) {
    board_init();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    adc_init();
    adc_gpio_init(PIN_ADC_X);
    adc_gpio_init(PIN_ADC_Y);

    init_button(PIN_MENU_UP);
    init_button(PIN_MENU_DOWN);
    init_button(PIN_MENU_LEFT);
    init_button(PIN_MENU_RIGHT);
    init_button(PIN_GRENADE_SEL);
    init_button(PIN_TRIGGER);
    init_button(PIN_RELOAD);
    init_button(PIN_COIN);
    init_button(PIN_START);
    init_button(PIN_TEST_CAL);
    init_button(PIN_AIM_ENABLE);

    calib_load(&g_cal);
    tusb_init();
    g_last_state_tx = get_absolute_time();

    while (true) {
        tud_task();
        read_inputs();
        send_hid_reports();
        poll_cdc();
        send_state(false);

        if ((g_buttons & BTN_TEST_BIT) && !(g_prev_buttons & BTN_TEST_BIT)) {
            cdc_write_line(PLAYER_ID == 1 ? "OPEN_CAL P=1\n" : "OPEN_CAL P=2\n");
        }
        g_prev_buttons = g_buttons;
        gpio_put(LED_PIN, (g_buttons & BTN_AIM_BIT) ? 1 : 0);
        sleep_ms(5);
    }
}

void tud_mount_cb(void) {}
void tud_umount_cb(void) {}
void tud_suspend_cb(bool remote_wakeup_en) { (void) remote_wakeup_en; }
void tud_resume_cb(void) {}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}
