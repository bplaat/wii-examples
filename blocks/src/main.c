// A voxel example that generates a 64x64x64 world with caves, trees, cacti, a hut, and water. It draws exposed faces
// with GX display lists and block textures.

#include <gccore.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "materials_tpl.h"
#include "world.h"

#define FIFO_SIZE (256 * 1024)
#define CLEAR_COLOR ((GXColor){46, 84, 158, 255})

static volatile bool running = true;

static void poweroff(void) {
    running = false;
}

static void wpad_poweroff(int32_t channel) {
    if (channel == WPAD_CHAN_ALL)
        running = false;
}

static GXTexObj textures[MaterialLayerCount];
static void* lists[MaterialLayerCount];
static uint32_t list_sizes[MaterialLayerCount];

// Corner order matches the six face normals in world.h.
static const int8_t corners[6][4][3] = {
    {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}}, {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}},
    {{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}}, {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}},
    {{-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1}}, {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}}};
static const uint8_t uv[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
static const float face_light[6] = {0.85f, 0.55f, 0.75f, 0.60f, 1.0f, 0.48f};

static void emit_face(const VoxelInstance* instance, bool water) {
    const int8_t (*face)[3] = corners[instance->face];
    float light = (0.38f + 0.62f * face_light[instance->face]) * (float)instance->occlusion / 255.0f;
    uint8_t shade = (uint8_t)(255.0f * light);
    for (int corner = 0; corner < 4; corner++) {
        GX_Position3f32(instance->x + face[corner][0] * 0.5f + 0.5f, instance->y + face[corner][1] * 0.5f + 0.5f,
                        instance->z + face[corner][2] * 0.5f + 0.5f);
        GX_Color4u8(shade, shade, shade, water ? 160 : 255);
        GX_TexCoord2u8(uv[corner][0], uv[corner][1]);
    }
}

static bool build_lists(const WorldInstances* world) {
    size_t total = world->opaqueCount + world->translucentCount;
    for (int material = 0; material < MaterialLayerCount; material++) {
        size_t count = 0;
        for (size_t i = 0; i < total; i++) {
            if (world->instances[i].material == material)
                count++;
        }
        if (count == 0)
            continue;
        size_t capacity = (count * 80 + ((count / 16000) + 1) * 64 + 159) & ~(size_t)31;
        lists[material] = memalign(32, capacity);
        if (!lists[material])
            return false;
        DCInvalidateRange(lists[material], capacity);
        GX_BeginDispList(lists[material], capacity);
        size_t emitted = 0;
        while (emitted < count) {
            size_t batch = count - emitted;
            if (batch > 16000)
                batch = 16000;
            GX_Begin(GX_QUADS, GX_VTXFMT0, batch * 4);
            size_t seen = 0;
            for (size_t i = 0; i < total && seen < emitted + batch; i++) {
                if (world->instances[i].material != material)
                    continue;
                if (seen++ < emitted)
                    continue;
                emit_face(&world->instances[i], material == MaterialLayerWater);
            }
            GX_End();
            emitted += batch;
        }
        list_sizes[material] = GX_EndDispList();
        if (list_sizes[material] == 0)
            return false;
    }
    return true;
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

    TPLFile tpl;
    if (TPL_OpenTPLFromMemory(&tpl, (void*)materials_tpl, materials_tpl_size) != 1)
        return 1;
    for (int i = 0; i < MaterialLayerCount; i++) {
        if (TPL_GetTexture(&tpl, i, &textures[i]) != 0)
            return 1;
    }

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GX_SetCullMode(GX_CULL_NONE);

    WorldInstances world = world_build();
    if (!world.instances || !build_lists(&world))
        return 1;
    free(world.instances);

    Mtx44 projection;
    guPerspective(projection, 50.0f, aspect, 1.0f, 500.0f);
    GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
    float angle = 0;
    while (running) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
            break;
        angle += 0.00267f;
        guVector eye = {32.0f + sinf(angle) * 88.0f, 79.0f, 32.0f + cosf(angle) * 88.0f};
        guVector target = {32.0f, 21.76f, 32.0f};
        guVector up = {0, 1, 0};
        Mtx view;
        guLookAt(view, &eye, &up, &target);
        GX_LoadPosMtxImm(view, GX_PNMTX0);

        GX_SetZMode(GX_ENABLE, GX_LEQUAL, GX_TRUE);
        GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
        for (int material = 0; material < MaterialLayerCount; material++) {
            if (material == MaterialLayerWater || !list_sizes[material])
                continue;
            GX_LoadTexObj(&textures[material], GX_TEXMAP0);
            GX_CallDispList(lists[material], list_sizes[material]);
        }
        if (list_sizes[MaterialLayerWater]) {
            GX_SetZMode(GX_ENABLE, GX_LEQUAL, GX_FALSE);
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            GX_LoadTexObj(&textures[MaterialLayerWater], GX_TEXMAP0);
            GX_CallDispList(lists[MaterialLayerWater], list_sizes[MaterialLayerWater]);
        }

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
    WPAD_Disconnect(WPAD_CHAN_ALL);
    return 0;
}
