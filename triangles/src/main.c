// A simple Wii homebrew test program that draws some triangles with display lists
#include <gccore.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#define DEFAULT_FIFO_SIZE (256 * 1024)

GXRModeObj *screenmode;

// Triangle display list
// clang-format off
_Alignas(32) uint8_t triangle_list[32] = {
    GX_TRIANGLES | GX_VTXFMT0,
    0, 3,           // number of vertexes (16-bit big endian)
    0, 1,           // position
    255, 0, 0, 255, // color
    -1, -1,         // position
    0, 255, 0, 255, // color
    1, -1,          // position
    0, 0, 255, 255  // color
};
// clang-format on

// Poweroff callbacks
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

    GX_ClearVtxDesc();
    GX_InvVtxCache();
    GX_InvalidateTexAll();
    VIDEO_SetBlack(false);

    // Init wpad buttons
    WPAD_Init();

    // Set power off handlers
    SYS_SetPowerCallback(poweroff);
    WPAD_SetPowerButtonCallback(wpad_poweroff);

    // Set projection matrix
    Mtx44 perspective_matrix;
    guPerspective(perspective_matrix, 45, (float)screenmode->viWidth / (float)screenmode->viHeight, 0.1, 1000);
    GX_LoadProjectionMtx(perspective_matrix, GX_PERSPECTIVE);

    // Game state
    float triangle_rotation = 0;

    // Game loop
    while (running) {
        // Update
        triangle_rotation += 1;

        // Read buttons
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) running = false;

        // Enable depth test and disable culling
        GX_SetZMode(GX_ENABLE, GX_LEQUAL, GX_TRUE);
        GX_SetCullMode(GX_CULL_NONE);

        // Setup triangle draw
        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
        GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S8, 0);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GX_SetNumChans(1);
        GX_SetNumTexGens(0);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

        // Draw triangles
        for (int32_t y = -2; y < 2; y++) {
            for (int32_t x = -2; x < 2; x++) {
                // Set triangle matrix
                Mtx triangle_matrix;
                guMtxRotDeg(triangle_matrix, 'x', triangle_rotation);
                Mtx temp_matrix;
                guMtxRotDeg(temp_matrix, 'y', triangle_rotation);
                guMtxConcat(triangle_matrix, temp_matrix, triangle_matrix);
                guMtxTransApply(triangle_matrix, triangle_matrix, x * 2 + 1, y * 2 + 1, -10);
                GX_LoadPosMtxImm(triangle_matrix, GX_PNMTX0);

                // Draw triangle display list
                GX_CallDispList(triangle_list, sizeof(triangle_list));
            }
        }

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

    // Disconnect wpads
    WPAD_Disconnect(WPAD_CHAN_ALL);
    return 0;
}
