use std::env;
use std::path::PathBuf;

fn main() {
    let devkitpro =
        PathBuf::from(env::var_os("DEVKITPRO").expect("DEVKITPRO must point to devkitPro"));
    let devkitppc =
        PathBuf::from(env::var_os("DEVKITPPC").expect("DEVKITPPC must point to devkitPPC"));
    let libogc = devkitpro.join("libogc2/wii/lib");

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
