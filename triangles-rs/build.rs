use std::{env, path::PathBuf, process::Command};

fn main() {
    let devkitpro = env::var("DEVKITPRO").expect("Please set DEVKITPRO");
    let devkitppc = env::var("DEVKITPPC").expect("Please set DEVKITPPC");
    let tool_dir = PathBuf::from(devkitppc).join("bin");

    println!("cargo:rustc-link-search=native={devkitpro}/libogc/lib/wii");

    let gcc = tool_dir.join("powerpc-eabi-gcc");
    let output = Command::new(&gcc)
        .arg("-print-libgcc-file-name")
        .output()
        .unwrap_or_else(|error| panic!("Failed to run {}: {error}", gcc.display()));
    assert!(output.status.success(), "powerpc-eabi-gcc failed");

    let libgcc =
        String::from_utf8(output.stdout).expect("powerpc-eabi-gcc returned a non-UTF-8 path");
    let libgcc = libgcc.trim();
    let out_dir = PathBuf::from(env::var_os("OUT_DIR").expect("OUT_DIR is not set"));
    let ar = tool_dir.join("powerpc-eabi-ar");
    let status = Command::new(&ar)
        .current_dir(&out_dir)
        .args(["x", libgcc, "crtresxfpr.o", "crtresxgpr.o"])
        .status()
        .unwrap_or_else(|error| panic!("Failed to run {}: {error}", ar.display()));
    assert!(status.success(), "powerpc-eabi-ar failed");

    println!(
        "cargo:rustc-link-arg={}",
        out_dir.join("crtresxgpr.o").display()
    );
    println!(
        "cargo:rustc-link-arg={}",
        out_dir.join("crtresxfpr.o").display()
    );
}
