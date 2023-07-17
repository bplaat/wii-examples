#include "cursor.h"

#include <gccore.h>

#include "canvas.h"
#include "cursor1_png.h"
#include "cursor2_png.h"
#include "cursor3_png.h"
#include "cursor4_png.h"
#include "texture.h"

Cursor cursors[4] = {0};

void cursor_init(void) {
    // Load cursor textures
    texture_load_png_rgba8(&cursors[0].texture, cursor1_png, cursor1_png_size);
    texture_load_png_rgba8(&cursors[1].texture, cursor2_png, cursor2_png_size);
    texture_load_png_rgba8(&cursors[2].texture, cursor3_png, cursor3_png_size);
    texture_load_png_rgba8(&cursors[3].texture, cursor4_png, cursor4_png_size);

    // Read cursors state
    WPAD_ScanPads();
    for (int32_t i = 0; i < 4; i++) {
        Cursor *cursor = &cursors[i];
        // uint32_t devtype;
        // WPAD_Probe(i, &devtype);
        // cursor->enabled = devtype == WPAD_EXP_NONE || devtype == WPAD_EXP_NUNCHUK || devtype == WPAD_EXP_CLASSIC;
        cursor->enabled = i == 0;
    }
}

void cursor_update(void) {
    // Read cursors state
    WPAD_ScanPads();
    for (int32_t i = 0; i < 4; i++) {
        Cursor *cursor = &cursors[i];
        if (cursor->enabled) {
            ir_t ir;
            WPAD_IR(i, &ir);
            cursor->x = ir.x;
            cursor->y = ir.y;
            cursor->angle = ir.angle;
            cursor->buttons_down = WPAD_ButtonsDown(i);
            cursor->buttons_held = WPAD_ButtonsHeld(i);
            cursor->buttons_up = WPAD_ButtonsUp(i);
        }
    }
}

void cursor_render(void) {
    // Draw enabled cursors on screen
    for (int32_t i = 0; i < 4; i++) {
        Cursor *cursor = &cursors[i];
        if (cursor->enabled) {
            guMtxRotDeg(canvas.transform_matrix, 'z', cursor->angle);
            canvas_draw_image(&cursor->texture, cursor->x - 96 / 2, cursor->y - 96 / 2, 96, 96, 0xffffffff);
        }
    }
    guMtxIdentity(canvas.transform_matrix);
}
