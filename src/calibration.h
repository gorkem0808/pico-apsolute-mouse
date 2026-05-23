#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint16_t x_min;
    uint16_t x_max;
    uint16_t y_min;
    uint16_t y_max;
    uint16_t flags;
    uint16_t crc;
} calib_t;

#define CAL_FLAG_INVERT_X 0x0001
#define CAL_FLAG_INVERT_Y 0x0002

void calib_load(calib_t *c);
bool calib_save(const calib_t *c);
void calib_defaults(calib_t *c);
uint16_t calib_map_x(const calib_t *c, uint16_t raw);
uint16_t calib_map_y(const calib_t *c, uint16_t raw);
uint16_t calib_crc(const calib_t *c);
