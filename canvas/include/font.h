#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct FontChar {
    uint32_t n;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    int16_t a;
    bool c;
} FontChar;

#define FONT_RENDER_SIZE 64

extern FontChar font[101];
