#pragma once

#include <gccore.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Cursor {
    bool enabled;
    int32_t x;
    int32_t y;
    int32_t angle;
    uint32_t buttons_held;
    GXTexObj texture;
} Cursor;

extern Cursor cursors[4];

void cursor_init(void);

void cursor_update(void);

void cursor_render(void);
