#![no_main]
#![no_std]

use core::{
    ffi::c_void,
    mem::ManuallyDrop,
    sync::atomic::{AtomicBool, Ordering},
};

use ogc_rs::{
    ffi,
    gu::{Gu, RotationAxis},
    gx::{
        types::{Gamma, VtxDest},
        CmpFn, Color, CullMode, Gx, ProjectionType, VtxAttr,
    },
    input::{ControllerPort, WPad, WPadButton},
    system::System,
    utils::mem::to_uncached,
    video::Video,
};

const FIFO_SIZE: usize = 256 * 1024;
const MAX_Z24: u32 = 0x00ff_ffff;

#[repr(C, align(32))]
struct DisplayList([u8; 32]);

// GX_TRIANGLES | GX_VTXFMT0, followed by three direct S8 XY and RGBA8 vertices.
static TRIANGLE_LIST: DisplayList = DisplayList([
    0x90, 0x00, 0x03, 0x00, 0x03, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0xff, 0x00, 0xff, 0x01,
    0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
]);

static RUNNING: AtomicBool = AtomicBool::new(true);

extern "C" fn poweroff() {
    RUNNING.store(false, Ordering::Relaxed);
}

extern "C" fn wpad_poweroff(channel: i32) {
    if channel == ffi::WPAD_CHAN_ALL as i32 {
        RUNNING.store(false, Ordering::Relaxed);
    }
}

fn wait_for_frame(non_interlaced: bool) {
    Video::wait_vsync();
    if non_interlaced {
        Video::wait_vsync();
    }
}

fn draw_triangle() {
    // SAFETY: The static display list is 32-byte aligned, padded to 32 bytes,
    // and remains valid while GX consumes it.
    unsafe {
        ffi::GX_CallDispList(
            TRIANGLE_LIST.0.as_ptr() as *mut c_void,
            TRIANGLE_LIST.0.len() as u32,
        );
    }
}

#[no_mangle]
extern "C" fn main(_argc: isize, _argv: *const *const u8) -> isize {
    let video = Video::init();
    Video::set_black(true);

    let mut config = Video::get_preferred_mode();
    if unsafe { ffi::CONF_GetAspectRatio() } == ffi::CONF_ASPECT_16_9 as _ {
        config.vi_width = (f32::from(config.vi_height) * (16.0 / 9.0)) as u16;
    }
    Video::configure(&config);

    let second_framebuffer =
        (System::allocate_framebuffer(&config) as *mut u8).map_addr(to_uncached) as *mut c_void;
    let framebuffers = [video.framebuffer, second_framebuffer];
    let mut framebuffer_index = 0;

    // SAFETY: Both pointers refer to framebuffers allocated for this video mode.
    unsafe { Video::set_next_framebuffer(framebuffers[framebuffer_index]) };
    Video::flush();
    let non_interlaced = config.tv_type & ffi::VI_NON_INTERLACE != 0;
    wait_for_frame(non_interlaced);

    let _fifo = ManuallyDrop::new(Gx::init(FIFO_SIZE));
    Gx::set_viewport(
        0.0,
        0.0,
        f32::from(config.framebuffer_width),
        f32::from(config.embed_framebuffer_height),
        0.0,
        1.0,
    );
    let y_scale = Gx::get_y_scale_factor(
        config.embed_framebuffer_height,
        config.extern_framebuffer_height,
    );
    let xfb_height = Gx::set_disp_copy_y_scale(y_scale) as u16;
    Gx::set_disp_copy_src(
        0,
        0,
        config.framebuffer_width,
        config.embed_framebuffer_height,
    );
    Gx::set_disp_copy_dst(config.framebuffer_width, xfb_height);
    Gx::set_copy_filter(
        config.anti_aliasing != 0,
        &mut config.sample_pattern,
        true,
        &mut config.v_filter,
    );
    Gx::set_field_mode(
        config.field_rendering != 0,
        config.vi_height != 2 * config.extern_framebuffer_height,
    );
    Gx::set_disp_copy_gamma(Gamma::ONE_ZERO);
    Gx::clear_vtx_desc();
    Gx::inv_vtx_cache();
    Gx::invalidate_tex_all();
    Video::set_black(false);

    WPad::init();
    let wpad = WPad::new(ControllerPort::One);
    System::set_power_callback(poweroff);
    unsafe {
        ffi::WPAD_SetPowerButtonCallback(Some(wpad_poweroff));
    }

    let mut projection = [[0.0; 4]; 4];
    Gu::perspective(
        &mut projection,
        45.0,
        f32::from(config.vi_width) / f32::from(config.vi_height),
        0.1,
        1000.0,
    );
    Gx::load_projection_mtx(&projection, ProjectionType::Perspective);

    let mut rotation_degrees = 0.0_f32;
    while RUNNING.load(Ordering::Relaxed) {
        rotation_degrees = (rotation_degrees + 1.0) % 360.0;

        WPad::update();
        if wpad.is_button_down(WPadButton::HOME) {
            RUNNING.store(false, Ordering::Relaxed);
        }

        Gx::set_z_mode(true, CmpFn::LessEq, true);
        Gx::set_cull_mode(CullMode::None);
        Gx::clear_vtx_desc();
        Gx::set_vtx_desc(VtxAttr::Pos, VtxDest::DIRECT);
        Gx::set_vtx_desc(VtxAttr::Color0, VtxDest::DIRECT);
        Gx::set_vtx_attr_fmt(0, VtxAttr::Pos, ffi::GX_POS_XY, ffi::GX_S8, 0);
        Gx::set_vtx_attr_fmt(0, VtxAttr::Color0, ffi::GX_CLR_RGBA, ffi::GX_RGBA8, 0);
        Gx::set_num_chans(1);
        Gx::set_num_tex_gens(0);
        Gx::set_tev_order(
            0,
            ffi::GX_TEXCOORDNULL as u8,
            ffi::GX_TEXMAP_NULL,
            ffi::GX_COLOR0A0 as u8,
        );
        Gx::set_tev_op(0, ffi::GX_PASSCLR as u8);

        let radians = rotation_degrees * (core::f32::consts::PI / 180.0);
        for y in -2..2 {
            for x in -2..2 {
                let mut x_rotation = [[0.0; 4]; 3];
                let mut y_rotation = [[0.0; 4]; 3];
                let mut rotation = [[0.0; 4]; 3];
                let mut model = [[0.0; 4]; 3];
                Gu::mtx_rotation_radians(&mut x_rotation, RotationAxis::X, radians);
                Gu::mtx_rotation_radians(&mut y_rotation, RotationAxis::Y, radians);
                Gu::mtx_concat(&mut x_rotation, &mut y_rotation, &mut rotation);
                Gu::mtx_translation_apply(
                    &mut rotation,
                    &mut model,
                    ((x * 2 + 1) as f32, (y * 2 + 1) as f32, -10.0),
                );
                Gx::load_pos_mtx_imm(&mut model, 0);
                draw_triangle();
            }
        }

        Gx::set_copy_clear(Color::new(128, 128, 128), MAX_Z24);
        Gx::draw_done();
        framebuffer_index ^= 1;
        // SAFETY: The destination is the inactive allocated framebuffer.
        unsafe { Gx::copy_disp(framebuffers[framebuffer_index], true) };
        // SAFETY: The copied framebuffer remains allocated for the program lifetime.
        unsafe { Video::set_next_framebuffer(framebuffers[framebuffer_index]) };
        Video::flush();
        wait_for_frame(non_interlaced);
    }

    unsafe {
        ffi::WPAD_Disconnect(ffi::WPAD_CHAN_ALL as i32);
    }
    0
}
