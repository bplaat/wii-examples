use crate::libogc::{GXTexObj, TPL_CloseTPLFile, TPL_GetTexture, TPL_OpenTPLFromMemory, TPLFile};
use crate::world::Material;

pub struct Materials {
    textures: [GXTexObj; Material::COUNT],
    tpl: TPLFile,
}

impl Materials {
    pub fn load(bytes: &'static [u8]) -> Option<Self> {
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
            return None;
        }
        let mut textures = [GXTexObj { val: [0; 8] }; Material::COUNT];
        for (material, texture) in Material::ALL.into_iter().zip(&mut textures) {
            if unsafe { TPL_GetTexture(&mut tpl, material.index() as i32, texture) } != 0 {
                unsafe { TPL_CloseTPLFile(&mut tpl) };
                return None;
            }
        }
        Some(Self { textures, tpl })
    }

    pub fn get(&self, material: Material) -> &GXTexObj {
        &self.textures[material.index()]
    }
}

impl Drop for Materials {
    fn drop(&mut self) {
        unsafe { TPL_CloseTPLFile(&mut self.tpl) }
    }
}
