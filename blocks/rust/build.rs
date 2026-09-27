use std::env;
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
    let dependencies = out_dir.join("materials.d");

    println!("cargo:rerun-if-changed={}", script.display());
    for image in [
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
    ] {
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
