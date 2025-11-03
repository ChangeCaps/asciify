#pragma once

#include <math.h>
#include <stdint.h>

uint8_t rgb_to_xterm(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t ri = (r < 48) ? 0 : (r < 115) ? 1 : (r - 35) / 40;
    uint8_t gi = (g < 48) ? 0 : (g < 115) ? 1 : (g - 35) / 40;
    uint8_t bi = (b < 48) ? 0 : (b < 115) ? 1 : (b - 35) / 40;

    uint8_t cube_color = 16 + (36 * ri) + (6 * gi) + bi;

    uint8_t rc = (ri == 0) ? 0 : 55 + ri * 40;
    uint8_t gc = (gi == 0) ? 0 : 55 + gi * 40;
    uint8_t bc = (bi == 0) ? 0 : 55 + bi * 40;

    uint8_t gray_avg = ((int) r + (int) g + (int) b) / 3;
    uint8_t gray_idx;

    if (gray_avg > 238) {
        gray_idx = 23;
    } else {
        gray_idx = (gray_avg - 3) / 10;
    }

    uint8_t gray_color = 232 + gray_idx;
    uint8_t gray_lvl   = 8 + gray_idx * 10;

    float color_dist = powf(r - rc, 2.0) + pow(g - gc, 2.0) + pow(b - bc, 2.0);
    float gray_dist  = powf(r - gray_lvl, 2.0) + powf(g - gray_lvl, 2.0) +
                      powf(b - gray_lvl, 2.0);

    return (gray_dist < color_dist) ? gray_color : cube_color;
}
