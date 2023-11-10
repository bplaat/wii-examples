#include "canvas.h"

#include <malloc.h>

#include "font.h"
#include "font_png.h"
#include "texture.h"

Canvas canvas;

_Alignas(32) uint16_t blank_pixels[16] = {0xffff};

void canvas_init(void) {
    // Create blank texture
    GX_InitTexObj(&canvas.blank_texture, blank_pixels, 1, 1, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);

    // Load font texture
    texture_load_png_rgba8(&canvas.font_texture, font_png, font_png_size);
}

void canvas_begin(uint32_t screen_width, uint32_t screen_height) {
    // Reset canvas state
    guMtxIdentity(canvas.transform_matrix);

    // Set ortographic matrix
    Mtx44 projection_matrix;
    guOrtho(projection_matrix, 0, screen_height, 0, screen_width, -1, 1);
    GX_LoadProjectionMtx(projection_matrix, GX_ORTHOGRAPHIC);

    // Disable depth test, disable culling and enable alpha blending
    GX_SetZMode(GX_DISABLE, GX_LEQUAL, GX_TRUE);
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

    // Set vertex pipeline
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
}

void canvas_end(void) {
    GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_Flush();
}

inline void canvas_fill_rect(float x, float y, float width, float height, uint32_t color) {
    canvas_draw_image(&canvas.blank_texture, x, y, width, height, color);
}

void canvas_draw_image(GXTexObj *texture, float x, float y, float width, float height, uint32_t color) {
    // Load texture
    GX_LoadTexObj(texture, GX_TEXMAP0);

    // Set quad matrix
    // clang-format off
    Mtx matrix = {
        {width, 0, 0, x + width / 2},
        {0, height, 0, y + height / 2},
        {0, 0, 1, 0}
    };
    // clang-format on
    guMtxConcat(matrix, canvas.transform_matrix, matrix);
    GX_LoadPosMtxImm(matrix, GX_PNMTX0);

    // Draw quad
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position2f32(0.5, -0.5);
    GX_Color1u32(color);
    GX_TexCoord2f32(1, 0);
    GX_Position2f32(0.5, 0.5);
    GX_Color1u32(color);
    GX_TexCoord2f32(1, 1);
    GX_Position2f32(-0.5, 0.5);
    GX_Color1u32(color);
    GX_TexCoord2f32(0, 1);
    GX_Position2f32(-0.5, -0.5);
    GX_Color1u32(color);
    GX_TexCoord2f32(0, 0);
    GX_End();
}

static uint32_t get_next_code_point(const char *str, int *index) {
    uint32_t code_point = 0;
    unsigned char current_byte;

    // Read the first byte
    current_byte = str[(*index)++];

    // Determine the number of bytes in the UTF-8 sequence based on the first byte
    int num_bytes = 0;
    if (current_byte < 0x80) {
        num_bytes = 1;
        code_point = current_byte;
    } else if ((current_byte & 0xE0) == 0xC0) {
        num_bytes = 2;
        code_point = current_byte & 0x1F;
    } else if ((current_byte & 0xF0) == 0xE0) {
        num_bytes = 3;
        code_point = current_byte & 0x0F;
    } else if ((current_byte & 0xF8) == 0xF0) {
        num_bytes = 4;
        code_point = current_byte & 0x07;
    } else {
        // Invalid UTF-8 sequence
        return 0;
    }

    // Read the remaining bytes
    for (int i = 1; i < num_bytes; ++i) {
        current_byte = str[(*index)++];
        if ((current_byte & 0xC0) != 0x80) {
            // Invalid UTF-8 sequence
            return 0;
        }
        code_point = (code_point << 6) | (current_byte & 0x3F);
    }

    return code_point;
}

void canvas_fill_text(char *text, float x, float y, float text_size, uint32_t color) {
    // Load font texture
    GX_LoadTexObj(&canvas.font_texture, GX_TEXMAP0);

    // Draw text characters
    float scale = text_size / FONT_RENDER_SIZE;
    int index = 0;
    uint32_t code_point;
    while ((code_point = get_next_code_point(text, &index)) != 0) {
        if (code_point == ' ') {
            x += 16 * scale;
            continue;
        }

        // Get right char
        FontChar *font_char = &font[code_point - 33];
        if (code_point > 127) {
            for (int32_t i = 0; i < sizeof(font) / sizeof(FontChar); i++) {
                if (font[i].n == code_point) {
                    font_char = &font[i];
                    break;
                }
            }
        }

        // Set quad matrix
        float width = font_char->w * scale;
        float height = font_char->h * scale;
        // clang-format off
        Mtx matrix = {
            {width, 0, 0, x + width / 2},
            {0, height, 0, y + font_char->a * scale + height / 2},
            {0, 0, 1, 0}
        };
        // clang-format on
        GX_LoadPosMtxImm(matrix, GX_PNMTX0);

        // Draw character
        float left = font_char->x / 480.f;
        float top = font_char->y / 480.f;
        float right = (font_char->x + font_char->w) / 480.f;
        float bottom = (font_char->y + font_char->h) / 480.f;
        uint32_t c = font_char->c ? 0xffffffff : color;

        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position2f32(0.5, -0.5);
        GX_Color1u32(c);
        GX_TexCoord2f32(right, top);
        GX_Position2f32(0.5, 0.5);
        GX_Color1u32(c);
        GX_TexCoord2f32(right, bottom);
        GX_Position2f32(-0.5, 0.5);
        GX_Color1u32(c);
        GX_TexCoord2f32(left, bottom);
        GX_Position2f32(-0.5, -0.5);
        GX_Color1u32(c);
        GX_TexCoord2f32(left, top);
        GX_End();

        x += (font_char->w + 2) * scale;
    }
}
