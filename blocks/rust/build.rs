use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let devkitpro =
        PathBuf::from(env::var_os("DEVKITPRO").expect("DEVKITPRO must point to devkitPro"));
    let devkitppc =
        PathBuf::from(env::var_os("DEVKITPPC").expect("DEVKITPPC must point to devkitPPC"));
    let libogc = devkitpro.join("libogc2/wii/lib");
    let manifest_dir = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let assets = manifest_dir.join("src/assets");
    let out_dir = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    let script = assets.join("materials.scf");
    let tpl = out_dir.join("materials.tpl");
    let image_data = out_dir.join("materials.bin");
    let dependencies = out_dir.join("materials.d");

    println!("cargo:rerun-if-changed={}", script.display());
    let image_names = [
        "brick_red.png",
        "cactus_side.png",
        "cactus_top.png",
        "dirt.png",
        "dirt_grass.png",
        "grass_top.png",
        "greystone.png",
        "lava.png",
        "leaves.png",
        "sand.png",
        "stone.png",
        "stone_coal.png",
        "stone_diamond.png",
        "stone_gold.png",
        "stone_iron.png",
        "trunk_side.png",
        "trunk_top.png",
        "water.png",
        "wood.png",
    ];
    let script_text = fs::read_to_string(&script).expect("failed to read materials.scf");
    let listed_images: Vec<_> = script_text
        .lines()
        .filter(|line| !line.trim().is_empty())
        .map(|line| {
            line.split_once("filepath=\"")
                .and_then(|(_, rest)| rest.split_once('"'))
                .map(|(name, _)| name)
                .expect("invalid materials.scf entry")
        })
        .collect();
    assert_eq!(listed_images, image_names, "material order changed");
    for image in image_names {
        println!("cargo:rerun-if-changed={}", assets.join(image).display());
    }
    let status = Command::new("gxtexconv")
        .arg("-s")
        .arg(&script)
        .arg("-d")
        .arg(&dependencies)
        .arg("-o")
        .arg(&tpl)
        .status()
        .expect("gxtexconv must be installed by devkitPro");
    assert!(status.success(), "gxtexconv failed to build materials.tpl");

    let tpl_data = fs::read(&tpl).expect("failed to read materials.tpl");
    let read_u32 = |offset: usize| {
        u32::from_be_bytes(
            tpl_data
                .get(offset..offset + 4)
                .expect("truncated materials.tpl")
                .try_into()
                .unwrap(),
        ) as usize
    };
    assert_eq!(read_u32(0), 0x0020_af30, "invalid TPL signature");
    assert_eq!(read_u32(4), image_names.len(), "unexpected material count");
    assert_eq!(read_u32(8), 12, "unexpected TPL table offset");

    const IMAGE_SIZE: usize = 128 * 128 / 2;
    let mut images = Vec::with_capacity(image_names.len() * IMAGE_SIZE);
    for index in 0..image_names.len() {
        let header = read_u32(12 + index * 8);
        assert_eq!(read_u32(16 + index * 8), 0, "unexpected palette");
        let dimensions = tpl_data
            .get(header..header + 4)
            .expect("truncated texture header");
        assert_eq!(u16::from_be_bytes([dimensions[0], dimensions[1]]), 128);
        assert_eq!(u16::from_be_bytes([dimensions[2], dimensions[3]]), 128);
        assert_eq!(read_u32(header + 4), 14, "expected GX CMPR texture");
        assert_eq!(read_u32(header + 12), 1, "expected S repeat wrap");
        assert_eq!(read_u32(header + 16), 1, "expected T repeat wrap");
        assert_eq!(read_u32(header + 20), 1, "expected linear min filter");
        assert_eq!(read_u32(header + 24), 1, "expected linear mag filter");
        assert_eq!(read_u32(header + 28), 0, "unexpected LOD bias");
        assert_eq!(tpl_data.get(header + 32..header + 34), Some(&[0, 0][..]));
        let image = read_u32(header + 8);
        images.extend_from_slice(
            tpl_data
                .get(image..image + IMAGE_SIZE)
                .expect("truncated texture data"),
        );
    }
    fs::write(image_data, images).expect("failed to write materials.bin");

    println!("cargo:rerun-if-env-changed=DEVKITPRO");
    println!("cargo:rerun-if-env-changed=DEVKITPPC");
    println!("cargo:rustc-link-search=native={}", libogc.display());
    println!("cargo:rustc-link-lib=wiiuse");
    println!("cargo:rustc-link-lib=bte");
    println!("cargo:rustc-link-lib=ogc");
    println!("cargo:rustc-link-lib=m");
    println!("cargo:rustc-link-arg=-Wl,--start-group");
    println!("cargo:rustc-link-arg=-lsysbase");
    println!("cargo:rustc-link-arg=-lc");
    println!("cargo:rustc-link-arg=-Wl,--end-group");
    println!("cargo:rustc-link-arg=-lgcc");
    println!("cargo:rustc-link-arg=-mrvl");
    println!("cargo:rustc-link-arg=-mcpu=750");
    println!("cargo:rustc-link-arg=-meabi");
    println!("cargo:rustc-link-arg=-mhard-float");

    println!("cargo:rustc-env=DEVKITPPC={}", devkitppc.display());
    println!("cargo:rustc-env=DEVKITPRO={}", devkitpro.display());
}
