#pragma once

#include <gccore.h>
#include <stdint.h>

typedef struct Canvas {
    GXTexObj blank_texture;
    GXTexObj font_texture;
    Mtx transform_matrix;
} Canvas;

extern Canvas canvas;

void canvas_init(void);

void canvas_begin(uint32_t screen_width, uint32_t screen_height);

void canvas_end(void);

void canvas_fill_rect(float x, float y, float width, float height, uint32_t color);

void canvas_draw_image(GXTexObj *texture, float x, float y, float width, float height, uint32_t color);

void canvas_fill_text(char *text, float x, float y, float text_size, uint32_t color);
