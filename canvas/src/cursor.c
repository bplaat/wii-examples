#include "cursor.h"

#include <gccore.h>
#include <wiiuse/wpad.h>

#include "canvas.h"
#include "texture.h"
#include "cursor1_png.h"
#include "cursor2_png.h"
#include "cursor3_png.h"
#include "cursor4_png.h"

Cursor cursors[4] = {0};

void cursor_init(void) {
    // Load cursor textures
    texture_load_png_rgba8(&cursors[0].texture, cursor1_png, cursor1_png_size);
    texture_load_png_rgba8(&cursors[1].texture, cursor2_png, cursor2_png_size);
    texture_load_png_rgba8(&cursors[2].texture, cursor3_png, cursor3_png_size);
    texture_load_png_rgba8(&cursors[3].texture, cursor4_png, cursor4_png_size);
}

void cursor_update(void) {
    WPAD_ScanPads();

    ir_t ir;
    WPAD_IR(0, &ir);
    cursors[0].x = ir.x;
    cursors[0].y = ir.y;

    uint32_t buttonsHeld = WPAD_ButtonsDown(0);
    cursors[0].click = (buttonsHeld & WPAD_BUTTON_A) != 0;
}

void cursor_render(void) {
    canvas_draw_image(&cursors[0].texture, cursors[0].x - 96 / 2, cursors[0].y - 96 / 2, 96, 96, 0xffffffff);
}
