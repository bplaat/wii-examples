#include "texture.h"

#include <malloc.h>

#include "stb_image.h"

uint8_t *texture_convert_rgba8(uint8_t *src, int32_t width, int32_t height) {
    uint8_t *dst = memalign(32, height * width * 4);
    int32_t pos = 0;
    for (int32_t y = 0; y < height; y += 4) {
        for (int32_t x = 0; x < width; x += 4) {
            for (int32_t ry = 0; ry < 4; ry++) {
                for (int32_t rx = 0; rx < 4; rx++) {
                    int32_t offset = ((y + ry) * width + (x + rx)) * 4;
                    dst[pos++] = src[offset + 3];  // alpha
                    dst[pos++] = src[offset];      // red
                }
            }
            for (int32_t ry = 0; ry < 4; ry++) {
                for (int32_t rx = 0; rx < 4; rx++) {
                    int32_t offset = ((y + ry) * width + (x + rx)) * 4;
                    dst[pos++] = src[offset + 1];  // green
                    dst[pos++] = src[offset + 2];  // blue
                }
            }
        }
    }
    return dst;
}

void texture_load_png_rgba8(GXTexObj *texture, const uint8_t *data, size_t size) {
    int32_t width, height, channels;
    uint8_t *src = stbi_load_from_memory(data, size, &width, &height, &channels, 4);
    uint8_t *dst = texture_convert_rgba8(src, width, height);
    free(src);
    GX_InitTexObj(texture, dst, width, height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
}
