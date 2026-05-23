#include "calibration.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#define CAL_MAGIC 0x47544755u  // GTGU
#define CAL_VERSION 3u
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

void calib_defaults(calib_t *c) {
    memset(c, 0, sizeof(*c));
    c->magic = CAL_MAGIC;
    c->version = CAL_VERSION;
    c->x_min = 200;
    c->x_max = 3900;
    c->y_min = 200;
    c->y_max = 3900;
    c->flags = 0;
    c->crc = calib_crc(c);
}

uint16_t calib_crc(const calib_t *c) {
    const uint8_t *p = (const uint8_t*)c;
    uint16_t sum = 0x1234;
    for (unsigned i = 0; i < sizeof(calib_t) - sizeof(uint16_t); i++) {
        sum = (uint16_t)((sum << 5) | (sum >> 11));
        sum ^= p[i];
    }
    return sum;
}

void calib_load(calib_t *c) {
    const calib_t *flash_c = (const calib_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(c, flash_c, sizeof(*c));
    if (c->magic != CAL_MAGIC || c->version != CAL_VERSION || c->crc != calib_crc(c) ||
        c->x_min >= c->x_max || c->y_min >= c->y_max) {
        calib_defaults(c);
    }
}

bool calib_save(const calib_t *c) {
    calib_t tmp = *c;
    tmp.magic = CAL_MAGIC;
    tmp.version = CAL_VERSION;
    tmp.crc = calib_crc(&tmp);

    uint8_t sector[FLASH_SECTOR_SIZE];
    memset(sector, 0xFF, sizeof(sector));
    memcpy(sector, &tmp, sizeof(tmp));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, sector, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    return true;
}

static uint16_t map_range(uint16_t raw, uint16_t in_min, uint16_t in_max) {
    if (raw <= in_min) return 0;
    if (raw >= in_max) return 32767;
    uint32_t v = (uint32_t)(raw - in_min) * 32767u / (uint32_t)(in_max - in_min);
    if (v > 32767u) v = 32767u;
    return (uint16_t)v;
}

uint16_t calib_map_x(const calib_t *c, uint16_t raw) {
    uint16_t v = map_range(raw, c->x_min, c->x_max);
    if (c->flags & CAL_FLAG_INVERT_X) v = 32767 - v;
    return v;
}

uint16_t calib_map_y(const calib_t *c, uint16_t raw) {
    uint16_t v = map_range(raw, c->y_min, c->y_max);
    if (c->flags & CAL_FLAG_INVERT_Y) v = 32767 - v;
    return v;
}
