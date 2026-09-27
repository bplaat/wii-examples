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
    let texture_script = assets.join("blocks_texture.scf");
    let texture_tpl = out_dir.join("blocks_texture.tpl");
    let texture_dependencies = out_dir.join("blocks_texture.d");

    println!("cargo:rerun-if-changed={}", texture_script.display());
    println!(
        "cargo:rerun-if-changed={}",
        assets.join("dirt_grass.png").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        assets.join("stone_coal.png").display()
    );
    let status = Command::new("gxtexconv")
        .arg("-s")
        .arg(&texture_script)
        .arg("-d")
        .arg(&texture_dependencies)
        .arg("-o")
        .arg(&texture_tpl)
        .status()
        .expect("gxtexconv must be installed by devkitPro");
    assert!(
        status.success(),
        "gxtexconv failed to build blocks_texture.tpl"
    );

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
