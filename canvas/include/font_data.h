#pragma once

#include <stdint.h>

typedef struct FontChar {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int16_t xoffset;
    int16_t yoffset;
    int16_t xadvance;
} FontChar;

extern FontChar font_data[];
