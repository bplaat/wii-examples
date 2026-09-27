#![allow(non_snake_case)]

use crate::font::{FONT, FONT_RENDER_SIZE};
use crate::texture::Texture;
use core::alloc::{GlobalAlloc, Layout};
use core::ffi::{c_char, c_int, c_void};
use core::ptr::NonNull;

pub struct LibogcAllocator;

unsafe impl GlobalAlloc for LibogcAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        unsafe { memalign(layout.align().max(32), layout.size().max(1)).cast() }
    }

    unsafe fn dealloc(&self, pointer: *mut u8, _layout: Layout) {
        unsafe { free(pointer.cast()) }
    }
}

#[global_allocator]
static ALLOCATOR: LibogcAllocator = LibogcAllocator;

// System and video values
pub const SYS_BASE_CACHED: u32 = 0x8000_0000;
pub const SYS_BASE_UNCACHED: u32 = 0xc000_0000;
pub const VI_NON_INTERLACE: u32 = 1;
pub const CONF_ASPECT_16_9: i32 = 1;
pub const WPAD_CHAN_ALL: i32 = -1;
pub const WPAD_BUTTON_HOME: u32 = 0x8000;
pub const WPAD_FMT_BTNS_ACC_IR: i32 = 2;

// GX values
pub const GX_TRUE: u8 = 1;
pub const GX_FALSE: u8 = 0;
pub const GX_ENABLE: u8 = 1;
pub const GX_DISABLE: u8 = 0;
pub const GX_QUADS: u8 = 0x80;
pub const GX_VTXFMT0: u8 = 0;
pub const GX_VA_POS: u8 = 9;
pub const GX_VA_CLR0: u8 = 11;
pub const GX_VA_TEX0: u8 = 13;
pub const GX_DIRECT: u8 = 1;
pub const GX_POS_XY: u32 = 0;
pub const GX_POS_XYZ: u32 = 1;
pub const GX_F32: u32 = 4;
pub const GX_CLR_RGBA: u32 = 1;
pub const GX_RGBA8: u32 = 5;
pub const GX_TEX_ST: u32 = 1;
pub const GX_TF_RGBA8: u8 = 6;
pub const GX_CLAMP: u8 = 0;
pub const GX_TEXCOORD0: u16 = 0;
pub const GX_TEXMAP0: u32 = 0;
pub const GX_TG_MTX2X4: u32 = 1;
pub const GX_TG_TEX0: u32 = 4;
pub const GX_IDENTITY: u32 = 60;
pub const GX_TEVSTAGE0: u8 = 0;
pub const GX_COLOR0A0: u8 = 4;
pub const GX_MODULATE: u8 = 0;
pub const GX_REPLACE: u8 = 3;
pub const GX_ORTHOGRAPHIC: u8 = 1;
pub const GX_PERSPECTIVE: u8 = 0;
pub const GX_PNMTX0: u32 = 0;
pub const GX_CULL_NONE: u8 = 0;
pub const GX_ALWAYS: u8 = 7;
pub const GX_LEQUAL: u8 = 3;
pub const GX_ZC_LINEAR: u8 = 0;
pub const GX_PF_RGB8_Z24: u8 = 0;
pub const GX_PF_RGB565_Z16: u8 = 2;
pub const GX_GM_1_0: u8 = 0;
pub const GX_BM_BLEND: u8 = 1;
pub const GX_BM_NONE: u8 = 0;
pub const GX_BL_SRCALPHA: u8 = 4;
pub const GX_BL_INVSRCALPHA: u8 = 5;
pub const GX_LO_CLEAR: u8 = 0;
pub const GX_SRC_VTX: u8 = 1;
pub const GX_DF_NONE: u8 = 0;
pub const GX_AF_NONE: u8 = 2;
pub const GX_MAX_Z24: u32 = 0x00ff_ffff;
#[repr(C)]
pub struct GXRModeObj {
    pub vi_tv_mode: u32,
    pub fb_width: u16,
    pub efb_height: u16,
    pub xfb_height: u16,
    pub vi_x_origin: u16,
    pub vi_y_origin: u16,
    pub vi_width: u16,
    pub vi_height: u16,
    pub xfb_mode: u32,
    pub field_rendering: u8,
    pub aa: u8,
    pub sample_pattern: [[u8; 2]; 12],
    pub vfilter: [u8; 7],
    pub copy_interlaced: u8,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct GXColor {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

impl GXColor {
    pub const fn new(r: u8, g: u8, b: u8, a: u8) -> Self {
        Self { r, g, b, a }
    }
}

#[repr(C)]
pub struct GXTexObj {
    pub val: [u32; 8],
}

#[repr(C)]
pub struct TPLFile {
    pub kind: c_int,
    pub texture_count: c_int,
    pub texture_descriptors: *mut c_void,
    pub file: *mut c_void,
}

pub type Mtx = [[f32; 4]; 3];
pub type Mtx44 = [[f32; 4]; 4];

#[repr(C)]
pub struct GXFifoObj {
    _private: [u8; 0],
}
pub type PowerCallback = Option<extern "C" fn()>;
pub type WpadPowerCallback = Option<extern "C" fn(c_int)>;

pub struct HeapBuffer(NonNull<c_void>);

impl HeapBuffer {
    pub unsafe fn from_raw(pointer: *mut c_void) -> Option<Self> {
        NonNull::new(pointer).map(Self)
    }

    pub fn as_ptr(&self) -> *mut c_void {
        self.0.as_ptr()
    }
}

impl Drop for HeapBuffer {
    fn drop(&mut self) {
        unsafe { free(self.0.as_ptr()) }
    }
}

#[repr(C)]
pub struct IrDot {
    pub visible: u8,
    pub rx: i16,
    pub ry: i16,
    pub size: u8,
}
#[repr(C)]
pub struct IrData {
    pub dots: [IrDot; 4],
    pub num_dots: u8,
    pub state: c_int,
    pub raw_valid: c_int,
    pub sensorbar: [f32; 15],
    pub ax: f32,
    pub ay: f32,
    pub distance: f32,
    pub z: f32,
    pub angle: f32,
    pub smooth_valid: c_int,
    pub sx: f32,
    pub sy: f32,
    pub error_cnt: f32,
    pub glitch_cnt: f32,
    pub valid: c_int,
    pub x: f32,
    pub y: f32,
    pub aspect: c_int,
    pub pos: c_int,
    pub vres: [u32; 2],
    pub offset: [c_int; 2],
}

unsafe extern "C" {
    // Write-gather pipe and libogc heap
    pub static wgPipe: *mut c_void;
    pub fn memalign(alignment: usize, size: usize) -> *mut c_void;
    pub fn free(pointer: *mut c_void);

    // Video mode and framebuffer setup
    pub fn VIDEO_Init();
    pub fn VIDEO_SetBlack(black: bool);
    pub fn VIDEO_GetPreferredMode(mode: *mut GXRModeObj) -> *mut GXRModeObj;
    pub fn SYS_AllocateFramebuffer(mode: *const GXRModeObj) -> *mut c_void;
    pub fn CONF_GetAspectRatio() -> c_int;
    pub fn VIDEO_Configure(mode: *const GXRModeObj);
    pub fn VIDEO_SetNextFramebuffer(framebuffer: *mut c_void);
    pub fn VIDEO_Flush();
    pub fn VIDEO_WaitVSync();

    // GX initialization and display output
    pub fn GX_Init(fifo: *mut c_void, size: u32) -> *mut GXFifoObj;
    pub fn GX_SetViewport(x: f32, y: f32, width: f32, height: f32, near: f32, far: f32);
    pub fn GX_SetScissor(x: u32, y: u32, width: u32, height: u32);
    pub fn GX_GetYScaleFactor(efb_height: u16, xfb_height: u16) -> f32;
    pub fn GX_SetDispCopyYScale(scale: f32) -> u32;
    pub fn GX_SetDispCopySrc(x: u16, y: u16, width: u16, height: u16);
    pub fn GX_SetDispCopyDst(width: u16, height: u16);
    pub fn GX_SetCopyFilter(aa: u8, sample_pattern: *const [u8; 2], vf: u8, vfilter: *const u8);
    pub fn GX_SetFieldMode(field_mode: u8, half_aspect_ratio: u8);
    pub fn GX_SetPixelFmt(pixel_format: u8, depth_compression: u8);
    pub fn GX_SetDispCopyGamma(gamma: u8);
    pub fn GX_SetCopyClear(color: GXColor, depth: u32);

    // GX vertex, texture, and blend state
    pub fn GX_SetZMode(enable: u8, function: u8, update_enable: u8);
    pub fn GX_CopyDisp(destination: *mut c_void, clear: u8);
    pub fn GX_ClearVtxDesc();
    pub fn GX_SetVtxDesc(attribute: u8, mode: u8);
    pub fn GX_SetVtxAttrFmt(
        format: u8,
        attribute: u32,
        component_type: u32,
        component_size: u32,
        fractional_bits: u32,
    );
    pub fn GX_SetNumChans(count: u8);
    pub fn GX_SetChanCtrl(
        channel: i32,
        enable: u8,
        ambient_source: u8,
        material_source: u8,
        light_mask: u8,
        diffuse_function: u8,
        attenuation_function: u8,
    );
    pub fn GX_SetNumTexGens(count: u32);
    pub fn GX_SetNumTevStages(count: u8);
    pub fn GX_SetTexCoordGen(coord: u16, kind: u32, source: u32, matrix: u32);
    pub fn GX_SetTevOrder(stage: u8, coordinate: u8, map: u32, color: u8);
    pub fn GX_SetTevOp(stage: u8, mode: u8);
    pub fn GX_SetCullMode(mode: u8);
    pub fn GX_SetBlendMode(kind: u8, src: u8, dst: u8, op: u8);
    pub fn GX_SetCurrentMtx(matrix: u32);

    // GX commands and texture objects
    pub fn GX_Flush();
    pub fn GX_DrawDone();
    pub fn GX_InitTexObj(
        obj: *mut GXTexObj,
        data: *mut c_void,
        width: u16,
        height: u16,
        format: u8,
        wrap_s: u8,
        wrap_t: u8,
        mipmap: u8,
    );
    pub fn GX_LoadTexObj(obj: *const GXTexObj, map: u8);
    pub fn TPL_OpenTPLFromMemory(tpl: *mut TPLFile, memory: *mut c_void, len: u32) -> c_int;
    pub fn TPL_GetTexture(tpl: *mut TPLFile, id: c_int, texture: *mut GXTexObj) -> c_int;
    pub fn TPL_CloseTPLFile(tpl: *mut TPLFile);
    pub fn GX_Begin(primitive: u8, format: u8, vertices: u16);

    // GX matrix loading and cache management
    pub fn GX_LoadProjectionMtx(matrix: *const Mtx44, projection_type: u8);
    pub fn GX_LoadPosMtxImm(matrix: *const Mtx, index: u32);
    pub fn DCFlushRange(pointer: *mut c_void, size: u32);

    // GU matrix operations
    pub fn guPerspective(matrix: *mut Mtx44, fovy: f32, aspect: f32, near: f32, far: f32);
    pub fn guOrtho(
        matrix: *mut Mtx44,
        top: f32,
        bottom: f32,
        left: f32,
        right: f32,
        near: f32,
        far: f32,
    );
    pub fn ps_guMtxIdentity(matrix: *mut Mtx);
    pub fn ps_guMtxRotRad(matrix: *mut Mtx, axis: c_char, radians: f32);
    pub fn ps_guMtxConcat(a: *const Mtx, b: *const Mtx, dst: *mut Mtx);
    pub fn ps_guMtxTransApply(src: *const Mtx, dst: *mut Mtx, x: f32, y: f32, z: f32);

    // Wii Remote input and power callbacks
    pub fn WPAD_Init() -> c_int;
    pub fn WPAD_ScanPads() -> c_int;
    pub fn WPAD_SetDataFormat(chan: c_int, format: c_int) -> c_int;
    pub fn WPAD_SetVRes(chan: c_int, xres: u32, yres: u32) -> c_int;
    pub fn WPAD_IR(chan: c_int, ir: *mut IrData);
    pub fn WPAD_ButtonsDown(chan: c_int) -> u32;
    pub fn SYS_SetPowerCallback(callback: PowerCallback) -> PowerCallback;
    pub fn WPAD_SetPowerButtonCallback(callback: WpadPowerCallback);
    pub fn WPAD_Disconnect(chan: c_int) -> c_int;
}

#[inline]
pub unsafe fn pipe_u32(value: u32) {
    unsafe { core::ptr::write_volatile(wgPipe.cast::<u32>(), value) }
}
#[inline]
pub unsafe fn pipe_f32(value: f32) {
    unsafe { core::ptr::write_volatile(wgPipe.cast::<f32>(), value) }
}

#[inline]
unsafe fn gx_position2(x: f32, y: f32) {
    unsafe {
        pipe_f32(x);
        pipe_f32(y);
    }
}

#[inline]
unsafe fn gx_position3(x: f32, y: f32, z: f32) {
    unsafe {
        pipe_f32(x);
        pipe_f32(y);
        pipe_f32(z);
    }
}

#[inline]
unsafe fn gx_texcoord(s: f32, t: f32) {
    unsafe {
        pipe_f32(s);
        pipe_f32(t);
    }
}

#[inline]
unsafe fn gx_color(color: u32) {
    unsafe {
        pipe_u32(color);
    }
}

unsafe fn draw_quad(color: u32, uvs: [[f32; 2]; 4]) {
    unsafe {
        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        for (position, uv) in [
            ([0.5, -0.5], uvs[0]),
            ([0.5, 0.5], uvs[1]),
            ([-0.5, 0.5], uvs[2]),
            ([-0.5, -0.5], uvs[3]),
        ] {
            gx_position2(position[0], position[1]);
            gx_color(color);
            gx_texcoord(uv[0], uv[1]);
        }
        GX_End();
    }
}

pub unsafe fn draw_image(
    texture: &Texture,
    transform: &Mtx,
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    color: u32,
) {
    unsafe {
        GX_LoadTexObj(&texture.object, GX_TEXMAP0 as u8);
        let source = [
            [width, 0.0, 0.0, x + width / 2.0],
            [0.0, height, 0.0, y + height / 2.0],
            [0.0, 0.0, 1.0, 0.0],
        ];
        let mut matrix = [[0.0; 4]; 3];
        guMtxConcat(&source, transform, &mut matrix);
        GX_LoadPosMtxImm(&matrix, GX_PNMTX0);
        // Map the PNG's top row to the top vertices of the quad.
        draw_quad(color, [[1.0, 0.0], [1.0, 1.0], [0.0, 1.0], [0.0, 0.0]]);
    }
}

pub unsafe fn draw_text(
    texture: &Texture,
    transform: &Mtx,
    text: &str,
    mut x: f32,
    y: f32,
    size: f32,
    color: u32,
) {
    unsafe {
        GX_LoadTexObj(&texture.object, GX_TEXMAP0 as u8);
        let scale = size / FONT_RENDER_SIZE as f32;
        for ch in text.chars() {
            if ch == ' ' {
                x += 16.0 * scale;
                continue;
            }
            let Some(glyph) = FONT.iter().find(|glyph| glyph.n == ch as u32) else {
                continue;
            };
            if glyph.w == 0 || glyph.h == 0 {
                continue;
            }
            let width = glyph.w as f32 * scale;
            let height = glyph.h as f32 * scale;
            let source = [
                [width, 0.0, 0.0, x + width / 2.0],
                [0.0, height, 0.0, y + glyph.a as f32 * scale + height / 2.0],
                [0.0, 0.0, 1.0, 0.0],
            ];
            let mut matrix = [[0.0; 4]; 3];
            guMtxConcat(&source, transform, &mut matrix);
            GX_LoadPosMtxImm(&matrix, GX_PNMTX0);
            // The glyph atlas uses the same top-to-bottom PNG orientation.
            let left = (glyph.x as f32 + 0.5) / 480.0;
            let top = (glyph.y as f32 + 0.5) / 480.0;
            let right = (glyph.x as f32 + glyph.w as f32 - 0.5) / 480.0;
            let bottom = (glyph.y as f32 + glyph.h as f32 - 0.5) / 480.0;
            draw_quad(
                if glyph.c { u32::MAX } else { color },
                [[right, top], [right, bottom], [left, bottom], [left, top]],
            );
            x += (glyph.w as f32 + 2.0) * scale;
        }
    }
}

pub unsafe fn draw_cube(texture: &Texture, rotation: f32, projection: &Mtx44) {
    unsafe {
        GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
        GX_SetZMode(GX_ENABLE, GX_LEQUAL, GX_TRUE);
        GX_SetCullMode(GX_CULL_NONE);
        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
        GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS as u32, GX_POS_XYZ, GX_F32, 0);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0 as u32, GX_TEX_ST, GX_F32, 0);
        GX_SetNumChans(1);
        GX_SetNumTexGens(1);
        GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2X4, GX_TG_TEX0, GX_IDENTITY);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0 as u8, GX_TEXMAP0, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
        GX_LoadTexObj(&texture.object, GX_TEXMAP0 as u8);
        let mut matrix = [[0.0; 4]; 3];
        let mut temp = [[0.0; 4]; 3];
        guMtxRotDeg(&mut matrix, b'x' as c_char, rotation);
        guMtxRotDeg(&mut temp, b'y' as c_char, rotation);
        guMtxConcat(&matrix, &temp, &mut matrix);
        guMtxTransApply(&matrix, &mut matrix, 0.0, 0.0, -8.0);
        GX_LoadPosMtxImm(&matrix, GX_PNMTX0);
        let faces: [[([f32; 3], [f32; 2]); 4]; 6] = [
            [
                ([-1.0, 1.0, -1.0], [0.0, 0.0]),
                ([-1.0, 1.0, 1.0], [1.0, 0.0]),
                ([-1.0, -1.0, 1.0], [1.0, 1.0]),
                ([-1.0, -1.0, -1.0], [0.0, 1.0]),
            ],
            [
                ([1.0, 1.0, -1.0], [0.0, 0.0]),
                ([1.0, -1.0, -1.0], [1.0, 0.0]),
                ([1.0, -1.0, 1.0], [1.0, 1.0]),
                ([1.0, 1.0, 1.0], [0.0, 1.0]),
            ],
            [
                ([-1.0, -1.0, 1.0], [0.0, 0.0]),
                ([1.0, -1.0, 1.0], [1.0, 0.0]),
                ([1.0, -1.0, -1.0], [1.0, 1.0]),
                ([-1.0, -1.0, -1.0], [0.0, 1.0]),
            ],
            [
                ([-1.0, 1.0, 1.0], [0.0, 0.0]),
                ([-1.0, 1.0, -1.0], [1.0, 0.0]),
                ([1.0, 1.0, -1.0], [1.0, 1.0]),
                ([1.0, 1.0, 1.0], [0.0, 1.0]),
            ],
            [
                ([1.0, -1.0, -1.0], [0.0, 0.0]),
                ([1.0, 1.0, -1.0], [1.0, 0.0]),
                ([-1.0, 1.0, -1.0], [1.0, 1.0]),
                ([-1.0, -1.0, -1.0], [0.0, 1.0]),
            ],
            [
                ([1.0, -1.0, 1.0], [0.0, 0.0]),
                ([-1.0, -1.0, 1.0], [1.0, 0.0]),
                ([-1.0, 1.0, 1.0], [1.0, 1.0]),
                ([1.0, 1.0, 1.0], [0.0, 1.0]),
            ],
        ];
        GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
        for face in faces {
            for (position, uv) in face {
                gx_position3(position[0], position[1], position[2]);
                gx_texcoord(uv[0], uv[1]);
            }
        }
        GX_End();
        GX_Flush();
    }
}
#[inline]
pub unsafe fn GX_End() {}
#[inline]
pub unsafe fn guMtxRotDeg(matrix: *mut Mtx, axis: c_char, degrees: f32) {
    unsafe { ps_guMtxRotRad(matrix, axis, degrees * (core::f32::consts::PI / 180.0)) }
}
#[inline]
pub unsafe fn guMtxIdentity(matrix: *mut Mtx) {
    unsafe { ps_guMtxIdentity(matrix) }
}
#[inline]
pub unsafe fn guMtxConcat(a: *const Mtx, b: *const Mtx, dst: *mut Mtx) {
    unsafe { ps_guMtxConcat(a, b, dst) }
}
#[inline]
pub unsafe fn guMtxTransApply(src: *const Mtx, dst: *mut Mtx, x: f32, y: f32, z: f32) {
    unsafe { ps_guMtxTransApply(src, dst, x, y, z) }
}
