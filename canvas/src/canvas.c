#include "canvas.h"

#include <malloc.h>

#include "font_data.h"
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

void canvas_end(void) { GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); }

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

    GX_Flush();
}

void canvas_fill_text(char *text, float x, float y, float text_size, uint32_t color) {
    // Load font texture
    GX_LoadTexObj(&canvas.font_texture, GX_TEXMAP0);

    // Draw text characters
    float scale = text_size / 72.f;
    char *c = text;
    while (*c != '\0') {
        FontChar *font_char = &font_data[*c - 32];

        // Set quad matrix
        float width = font_char->width * scale;
        float height = font_char->height * scale;
        // clang-format off
        Mtx matrix = {
            {width, 0, 0, x + font_char->xoffset * scale + width / 2},
            {0, height, 0, y + font_char->yoffset * scale + height / 2},
            {0, 0, 1, 0}
        };
        // clang-format on
        GX_LoadPosMtxImm(matrix, GX_PNMTX0);

        // Draw character
        float left = font_char->x / 436.f;
        float top = font_char->y / 436.f;
        float right = (font_char->x + font_char->width) / 436.f;
        float bottom = (font_char->y + font_char->height) / 436.f;
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position2f32(0.5, -0.5);
        GX_Color1u32(color);
        GX_TexCoord2f32(right, top);
        GX_Position2f32(0.5, 0.5);
        GX_Color1u32(color);
        GX_TexCoord2f32(right, bottom);
        GX_Position2f32(-0.5, 0.5);
        GX_Color1u32(color);
        GX_TexCoord2f32(left, bottom);
        GX_Position2f32(-0.5, -0.5);
        GX_Color1u32(color);
        GX_TexCoord2f32(left, top);
        GX_End();

        GX_Flush();

        x += font_char->xadvance * scale;
        c++;
    }
}
