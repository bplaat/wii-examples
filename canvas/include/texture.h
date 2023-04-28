#pragma once

#include <gccore.h>
#include <stddef.h>
#include <stdint.h>

uint8_t *texture_convert_rgba8(uint8_t *src, int32_t width, int32_t height);

void texture_load_png_rgba8(GXTexObj *texture, const uint8_t *data, size_t size);
