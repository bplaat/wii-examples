use alloc::vec;
use alloc::vec::Vec;
use core::ffi::c_void;

use crate::libogc::{
    DCFlushRange, GX_CLAMP, GX_FALSE, GX_InitTexObj, GX_TF_RGBA8, GXTexObj, TPL_CloseTPLFile,
    TPL_GetTexture, TPL_OpenTPLFromMemory, TPLFile,
};
use crate::png;

pub struct Texture {
    pub object: GXTexObj,
    _pixels: Vec<u8>,
}

pub struct TplTextures {
    pub dirt_grass: Texture,
    pub stone_coal: Texture,
    _tpl: TPLFile,
}

impl TplTextures {
    pub fn from_bytes(bytes: &'static [u8]) -> Result<Self, ()> {
        let mut tpl = TPLFile {
            kind: 0,
            texture_count: 0,
            texture_descriptors: core::ptr::null_mut(),
            file: core::ptr::null_mut(),
        };
        let opened = unsafe {
            TPL_OpenTPLFromMemory(
                &mut tpl,
                bytes.as_ptr().cast_mut().cast(),
                bytes.len() as u32,
            )
        };
        if opened != 1 {
            return Err(());
        }

        let mut dirt_grass = GXTexObj { val: [0; 8] };
        let mut stone_coal = GXTexObj { val: [0; 8] };
        let loaded = unsafe {
            TPL_GetTexture(&mut tpl, 0, &mut dirt_grass) == 0
                && TPL_GetTexture(&mut tpl, 1, &mut stone_coal) == 0
        };
        if !loaded {
            unsafe { TPL_CloseTPLFile(&mut tpl) };
            return Err(());
        }

        Ok(Self {
            dirt_grass: Texture::from_tpl_object(dirt_grass),
            stone_coal: Texture::from_tpl_object(stone_coal),
            _tpl: tpl,
        })
    }
}

impl Drop for TplTextures {
    fn drop(&mut self) {
        unsafe { TPL_CloseTPLFile(&mut self._tpl) }
    }
}

impl Texture {
    fn from_tpl_object(object: GXTexObj) -> Self {
        Self {
            object,
            _pixels: Vec::new(),
        }
    }

    pub fn from_png(bytes: &[u8]) -> Result<Self, ()> {
        let image = png::decode(bytes).map_err(|_| ())?;
        let width = image.width;
        let height = image.height;
        if width == 0 || height == 0 || width > u16::MAX as usize || height > u16::MAX as usize {
            return Err(());
        }

        let padded_width = (width + 3) & !3;
        let padded_height = (height + 3) & !3;
        let mut pixels = vec![0; padded_width * padded_height * 4];
        let mut out = 0;
        for tile_y in (0..padded_height).step_by(4) {
            for tile_x in (0..padded_width).step_by(4) {
                for plane in 0..2 {
                    for y in 0..4 {
                        for x in 0..4 {
                            let px = tile_x + x;
                            let py = tile_y + y;
                            if px < width && py < height {
                                let i = (py * width + px) * 4;
                                pixels[out] = image.rgba[i + if plane == 0 { 3 } else { 1 }];
                                pixels[out + 1] = image.rgba[i + if plane == 0 { 0 } else { 2 }];
                            }
                            out += 2;
                        }
                    }
                }
            }
        }
        unsafe {
            DCFlushRange(pixels.as_mut_ptr().cast(), pixels.len() as u32);
        }
        let mut object = GXTexObj { val: [0; 8] };
        unsafe {
            GX_InitTexObj(
                &mut object,
                pixels.as_mut_ptr().cast::<c_void>(),
                width as u16,
                height as u16,
                GX_TF_RGBA8,
                GX_CLAMP,
                GX_CLAMP,
                GX_FALSE,
            );
        }
        Ok(Self {
            object,
            _pixels: pixels,
        })
    }
}
