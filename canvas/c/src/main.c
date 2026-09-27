// A larger example that shows 2D rendering, PNG image loading, font rendering, and cursor rendering.

#include <gccore.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "blocks_texture.h"
#include "canvas.h"
#include "cursor.h"

#define FIFO_SIZE (256 * 1024)
#define CLEAR_COLOR ((GXColor){128, 128, 128, 255})

_Alignas(32) static const uint8_t blocks_texture_tpl[] = {
    #embed "../target/blocks_texture.tpl"
};

static volatile bool running = true;

static void poweroff(void) {
    running = false;
}

static void wpad_poweroff(int32_t channel) {
    if (channel == WPAD_CHAN_ALL)
        running = false;
}

int main(void) {
    VIDEO_Init();
    VIDEO_SetBlack(true);
    GXRModeObj* screenmode = VIDEO_GetPreferredMode(NULL);

    void* framebuffer0 = SYS_AllocateFramebuffer(screenmode);
    void* framebuffer1 = SYS_AllocateFramebuffer(screenmode);
    void* fifo = memalign(32, FIFO_SIZE);
    if (!framebuffer0 || !framebuffer1 || !fifo) {
        free(framebuffer0);
        free(framebuffer1);
        free(fifo);
        return 1;
    }
    void* framebuffers[2] = {MEM_K0_TO_K1(framebuffer0), MEM_K0_TO_K1(framebuffer1)};
    uint32_t framebuffer = 0;
    bool first_frame = true;
    float aspect = CONF_GetAspectRatio() == CONF_ASPECT_16_9 ? 16.0f / 9.0f : 4.0f / 3.0f;

    VIDEO_Configure(screenmode);
    VIDEO_SetNextFramebuffer(framebuffers[framebuffer]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (screenmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    memset(fifo, 0, FIFO_SIZE);
    GX_Init(fifo, FIFO_SIZE);
    GX_SetViewport(0, 0, screenmode->fbWidth, screenmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, screenmode->fbWidth, screenmode->efbHeight);
    float yscale = GX_GetYScaleFactor(screenmode->efbHeight, screenmode->xfbHeight);
    uint32_t xfb_height = GX_SetDispCopyYScale(yscale);
    GX_SetDispCopySrc(0, 0, screenmode->fbWidth, screenmode->efbHeight);
    GX_SetDispCopyDst(screenmode->fbWidth, xfb_height);
    GX_SetCopyFilter(screenmode->aa, screenmode->sample_pattern, GX_TRUE, screenmode->vfilter);
    GX_SetFieldMode(screenmode->field_rendering,
                    screenmode->viHeight == 2 * screenmode->xfbHeight ? GX_ENABLE : GX_DISABLE);
    GX_SetPixelFmt(screenmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetDispCopyGamma(GX_GM_1_0);
    GX_SetCopyClear(CLEAR_COLOR, GX_MAX_Z24);

    WPAD_Init();
    SYS_SetPowerCallback(poweroff);
    WPAD_SetPowerButtonCallback(wpad_poweroff);

    GX_SetZMode(GX_ENABLE, GX_ALWAYS, GX_TRUE);
    GX_CopyDisp(framebuffers[framebuffer], GX_TRUE);
    uint32_t screen_height = screenmode->viHeight;
    uint32_t screen_width = (uint32_t)(screen_height * aspect);

    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(0, screen_width, screen_height);

    // Init stuff
    canvas_init();
    cursor_init();

    // Load textures
    TPLFile blocks_tpl;
    if (TPL_OpenTPLFromMemory(&blocks_tpl, (void*)blocks_texture_tpl, sizeof(blocks_texture_tpl)) != 1)
        return 1;
    GXTexObj dirt_grass_texture;
    if (TPL_GetTexture(&blocks_tpl, dirt_grass, &dirt_grass_texture) != 0)
        return 1;
    GXTexObj stone_coal_texture;
    if (TPL_GetTexture(&blocks_tpl, stone_coal, &stone_coal_texture) != 0)
        return 1;

    // Game state
    float rotation = 0;
    Mtx44 perspective_matrix;
    guPerspective(perspective_matrix, 45.0f, aspect, 0.1f, 1000.0f);

    // Game loop
    while (running) {
        // Update
        rotation += 1;

        // Read buttons
        cursor_update();
        for (int32_t i = 0; i < 4; i++) {
            Cursor* cursor = &cursors[i];
            if (cursor->enabled) {
                if (cursor->buttons_down & WPAD_BUTTON_HOME)
                    running = false;
            }
        }

        // ### Draw cube ###
        {
            // Set projection matrix
            GX_LoadProjectionMtx(perspective_matrix, GX_PERSPECTIVE);

            // Enable depth test and disable culling
            GX_SetZMode(GX_ENABLE, GX_LEQUAL, GX_TRUE);
            GX_SetCullMode(GX_CULL_NONE);

            // Set vertex pipeline
            GX_ClearVtxDesc();
            GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
            GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
            GX_SetNumChans(1);
            GX_SetNumTexGens(1);
            GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
            GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);

            // Load texture
            GX_LoadTexObj(&stone_coal_texture, GX_TEXMAP0);

            // Set cube matrix
            Mtx cube_matrix;
            guMtxRotDeg(cube_matrix, 'x', rotation);
            Mtx temp_matrix;
            guMtxRotDeg(temp_matrix, 'y', rotation);
            guMtxConcat(cube_matrix, temp_matrix, cube_matrix);
            guMtxTransApply(cube_matrix, cube_matrix, 0, 0, -8);
            GX_LoadPosMtxImm(cube_matrix, GX_PNMTX0);

            // Draw cube
            GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
            GX_Position3f32(-1.0f, 1.0f, -1.0f);
            GX_TexCoord2f32(0.0f, 0.0f);
            GX_Position3f32(-1.0f, 1.0f, 1.0f);
            GX_TexCoord2f32(1.0f, 0.0f);
            GX_Position3f32(-1.0f, -1.0f, 1.0f);
            GX_TexCoord2f32(1.0f, 1.0f);
            GX_Position3f32(-1.0f, -1.0f, -1.0f);
            GX_TexCoord2f32(0.0f, 1.0f);

            GX_Position3f32(1.0f, 1.0f, -1.0f);
            GX_TexCoord2f32(0.0f, 0.0f);
            GX_Position3f32(1.0f, -1.0f, -1.0f);
            GX_TexCoord2f32(1.0f, 0.0f);
            GX_Position3f32(1.0f, -1.0f, 1.0f);
            GX_TexCoord2f32(1.0f, 1.0f);
            GX_Position3f32(1.0f, 1.0f, 1.0f);
            GX_TexCoord2f32(0.0f, 1.0f);

            GX_Position3f32(-1.0f, -1.0f, 1.0f);
            GX_TexCoord2f32(0.0f, 0.0f);
            GX_Position3f32(1.0f, -1.0f, 1.0f);
            GX_TexCoord2f32(1.0f, 0.0f);
            GX_Position3f32(1.0f, -1.0f, -1.0f);
            GX_TexCoord2f32(1.0f, 1.0f);
            GX_Position3f32(-1.0f, -1.0f, -1.0f);
            GX_TexCoord2f32(0.0f, 1.0f);

            GX_Position3f32(-1.0f, 1.0f, 1.0f);
            GX_TexCoord2f32(0.0f, 0.0f);
            GX_Position3f32(-1.0f, 1.0f, -1.0f);
            GX_TexCoord2f32(1.0f, 0.0f);
            GX_Position3f32(1.0f, 1.0f, -1.0f);
            GX_TexCoord2f32(1.0f, 1.0f);
            GX_Position3f32(1.0f, 1.0f, 1.0f);
            GX_TexCoord2f32(0.0f, 1.0f);

            GX_Position3f32(1.0f, -1.0f, -1.0f);
            GX_TexCoord2f32(0.0f, 0.0f);
            GX_Position3f32(1.0f, 1.0f, -1.0f);
            GX_TexCoord2f32(1.0f, 0.0f);
            GX_Position3f32(-1.0f, 1.0f, -1.0f);
            GX_TexCoord2f32(1.0f, 1.0f);
            GX_Position3f32(-1.0f, -1.0f, -1.0f);
            GX_TexCoord2f32(0.0f, 1.0f);

            GX_Position3f32(1.0f, -1.0f, 1.0f);
            GX_TexCoord2f32(0.0f, 0.0f);
            GX_Position3f32(-1.0f, -1.0f, 1.0f);
            GX_TexCoord2f32(1.0f, 0.0f);
            GX_Position3f32(-1.0f, 1.0f, 1.0f);
            GX_TexCoord2f32(1.0f, 1.0f);
            GX_Position3f32(1.0f, 1.0f, 1.0f);
            GX_TexCoord2f32(0.0f, 1.0f);
            GX_End();

            GX_Flush();
        }

        // ### Draw HUD ###
        canvas_begin(screen_width, screen_height);

        guMtxRotDeg(canvas.transform_matrix, 'z', rotation);
        canvas_draw_image(&dirt_grass_texture, 50, 100, 100, 100, 0xffffffff);
        canvas_draw_image(&dirt_grass_texture, 100, 150, 100, 100, 0xff0000ff);
        canvas_draw_image(&dirt_grass_texture, 150, 200, 100, 100, 0x00ff00ff);
        canvas_draw_image(&dirt_grass_texture, 200, 250, 100, 100, 0x0000ffff);
        guMtxIdentity(canvas.transform_matrix);

        float y = 8;
        canvas_fill_text("Hello Wii 🏠!", 8, y, 64, 0xffffffff);
        y += 64 + 8;
        canvas_fill_text("The quick brown fox jumps over the lazy dog.", 8, y, 24, 0xff0000ff);
        y += 24 + 8;

        char debug_string[255];
        sprintf(debug_string, "framebuffer=%dx%d viewport=%dx%d", screenmode->fbWidth, screenmode->xfbHeight,
                screen_width, screen_height);
        canvas_fill_text(debug_string, 8, y, 24, 0xffffffff);

        cursor_render();
        canvas_end();

        GX_DrawDone();
        framebuffer ^= 1;
        // GX_CopyDisp clears depth only when Z writes are enabled.
        GX_SetZMode(GX_ENABLE, GX_LEQUAL, GX_TRUE);
        GX_CopyDisp(framebuffers[framebuffer], GX_TRUE);
        VIDEO_SetNextFramebuffer(framebuffers[framebuffer]);
        if (first_frame) {
            VIDEO_SetBlack(false);
            first_frame = false;
        }
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }

    // Disconnect wpads
    WPAD_Disconnect(WPAD_CHAN_ALL);
    return 0;
}
