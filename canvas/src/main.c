#include <gccore.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "blocks_texture.h"
#include "blocks_texture_tpl.h"
#include "canvas.h"
#include "cursor.h"

#define DEFAULT_FIFO_SIZE (256 * 1024)

GXRModeObj *screenmode;
bool running = true;

void poweroff(void) { running = false; }

void wpad_poweroff(int32_t chan) {
    if (chan == WPAD_CHAN_ALL) {
        running = false;
    }
}

int main(void) {
    // Init video
    VIDEO_Init();
    VIDEO_SetBlack(true);

    // Get screen mode
    screenmode = VIDEO_GetPreferredMode(NULL);
    VIDEO_Configure(screenmode);
    if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
        screenmode->viWidth = (float)screenmode->viHeight * (16.f / 9.f);
    }

    // Alloc two framebuffers to toggle between
    void *frame_buffers[] = {MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenmode)),
                             MEM_K0_TO_K1(SYS_AllocateFramebuffer(screenmode))};
    uint32_t fb_index = 0;
    VIDEO_SetNextFramebuffer(frame_buffers[fb_index]);

    // Wait for next frame
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (screenmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Init gx fifo buffer
    uint8_t *gx_fifo = MEM_K0_TO_K1(memalign(32, DEFAULT_FIFO_SIZE));
    memset(gx_fifo, 0, DEFAULT_FIFO_SIZE);
    GX_Init(gx_fifo, DEFAULT_FIFO_SIZE);

    // Init other gx stuff
    GX_SetViewport(0, 0, screenmode->fbWidth, screenmode->efbHeight, 0, 1);
    float yscale = GX_GetYScaleFactor(screenmode->efbHeight, screenmode->xfbHeight);
    uint32_t xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetDispCopySrc(0, 0, screenmode->fbWidth, screenmode->efbHeight);
    GX_SetDispCopyDst(screenmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(screenmode->aa, screenmode->sample_pattern, GX_TRUE, screenmode->vfilter);
    GX_SetFieldMode(screenmode->field_rendering,
                    ((screenmode->viHeight == 2 * screenmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_InvVtxCache();
    GX_InvalidateTexAll();
    VIDEO_SetBlack(false);

    // Init wpad buttons
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
    WPAD_SetVRes(0, screenmode->viWidth, screenmode->viHeight);

    // Set power off handlers
    SYS_SetPowerCallback(poweroff);
    WPAD_SetPowerButtonCallback(wpad_poweroff);

    // Init stuff
    canvas_init();
    cursor_init();

    // Load textures
    TPLFile blocks_tpl;
    TPL_OpenTPLFromMemory(&blocks_tpl, (void *)blocks_texture_tpl, blocks_texture_tpl_size);
    GXTexObj dirt_grass_texture;
    TPL_GetTexture(&blocks_tpl, dirt_grass, &dirt_grass_texture);
    GXTexObj stone_coal_texture;
    TPL_GetTexture(&blocks_tpl, stone_coal, &stone_coal_texture);

    // Game state
    float rotation = 0;

    // Game loop
    while (running) {
        // Update
        rotation += 1;

        // Read buttons
        cursor_update();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) running = false;

        // ### Draw cube ###
        {
            // Set projection matrix
            Mtx44 perspective_matrix;
            guPerspective(perspective_matrix, 45, (float)screenmode->viWidth / (float)screenmode->viHeight, 0.1, 1000);
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
        }

        // ### Draw HUD ###
        canvas_begin(screenmode->viWidth, screenmode->viHeight);

        guMtxRotDeg(canvas.transform_matrix, 'z', rotation);
        canvas_draw_image(&dirt_grass_texture, 50, 100, 100, 100, 0xffffffff);
        canvas_draw_image(&dirt_grass_texture, 100, 150, 100, 100, 0xff0000ff);
        canvas_draw_image(&dirt_grass_texture, 150, 200, 100, 100, 0x00ff00ff);
        canvas_draw_image(&dirt_grass_texture, 200, 250, 100, 100, 0x0000ffff);
        guMtxIdentity(canvas.transform_matrix);

        canvas_fill_text("Hello Wii!", 8, 8, 32, 0xffffffff);
        canvas_fill_text("The quick brown fox jumps over the lazy dog.", 8, 8 + 32 + 8, 16, 0xffffffff);

        char debug_string[255];
        sprintf(debug_string, "framebuffer=%dx%d viewport=%dx%d", screenmode->fbWidth, screenmode->xfbHeight,
                screenmode->viWidth, screenmode->viHeight);
        canvas_fill_text(debug_string, 8, 8 + 32 + 8 + 16 + 8, 16, 0xffffffff);

        canvas_fill_text("Press home button to quit...", 8, screenmode->viHeight - 16 - 8 - 24 - 8, 16, 0x00ff00ff);
        canvas_fill_text("Made by Bastiaan van der Plaat", 8, screenmode->viHeight - 24 - 8, 24, 0xff0000ff);

        cursor_render();
        canvas_end();

        // Set clear color for next frame
        GX_SetCopyClear((GXColor){128, 128, 128, 255}, GX_MAX_Z24);

        // Present framebuffer
        GX_DrawDone();
        fb_index ^= 1;
        GX_CopyDisp(frame_buffers[fb_index], GX_TRUE);
        VIDEO_SetNextFramebuffer(frame_buffers[fb_index]);

        // Wait for next frame
        VIDEO_Flush();
        VIDEO_WaitVSync();
        if (screenmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    }

    // Power off console
    SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    return 0;
}
