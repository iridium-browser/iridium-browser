use std::fs;
use std::process::{self, Command};

use rustc_version::VersionMeta;
use tempfile::tempdir;

use rustc_build_sysroot::*;

fn run(cmd: &mut Command) {
    assert!(cmd
        .stdout(process::Stdio::null())
        .stderr(process::Stdio::null())
        .status()
        .expect("failed to run {cmd:?}")
        .success());
}

fn build_sysroot(b: SysrootBuilder) {
    let src_dir = rustc_sysroot_src(Command::new("rustc")).unwrap();
    b.cargo({
        let mut cmd = Command::new("cargo");
        cmd.stdout(process::Stdio::null());
        cmd.stderr(process::Stdio::null());
        cmd
    })
    .build_from_source(&src_dir)
    .unwrap();
}

fn test_sysroot_build(target: &str, mode: BuildMode, rustc_version: &VersionMeta) {
    let sysroot_dir = tempdir().unwrap();
    build_sysroot(
        SysrootBuilder::new(sysroot_dir.path(), target)
            .build_mode(mode)
            .rustc_version(rustc_version.clone()),
    );

    let crate_name = "rustc-build-sysroot-test-crate";
    let crate_dir = tempdir().unwrap();
    run(Command::new("cargo")
        .args(["new", crate_name])
        .current_dir(&crate_dir));
    let crate_dir = crate_dir.path().join(crate_name);
    run(Command::new("cargo")
        .arg(mode.as_str())
        .arg("--target")
        .arg(target)
        .current_dir(&crate_dir)
        .env(
            "RUSTFLAGS",
            format!("--sysroot {}", sysroot_dir.path().display()),
        ));
    if mode == BuildMode::Build && target == rustc_version.host {
        run(Command::new("cargo")
            .arg("run")
            .current_dir(&crate_dir)
            .env(
                "RUSTFLAGS",
                format!("--sysroot {}", sysroot_dir.path().display()),
            ));
    }
}

#[test]
fn host() {
    let rustc_version = VersionMeta::for_command(Command::new("rustc")).unwrap();

    for mode in [BuildMode::Build, BuildMode::Check] {
        test_sysroot_build(&rustc_version.host, mode, &rustc_version);
    }
}

#[test]
fn cross() {
    let rustc_version = VersionMeta::for_command(Command::new("rustc")).unwrap();

    for target in [
        "i686-unknown-linux-gnu",
        "aarch64-apple-darwin",
        "i686-pc-windows-msvc",
    ] {
        test_sysroot_build(target, BuildMode::Check, &rustc_version);
    }
}

#[test]
fn no_std() {
    let sysroot_dir = tempdir().unwrap();
    build_sysroot(
        SysrootBuilder::new(sysroot_dir.path(), "thumbv7em-none-eabihf")
            .build_mode(BuildMode::Check)
            .sysroot_config(SysrootConfig::NoStd),
    );
}

#[test]
fn json_target() {
    // Example taken from https://book.avr-rust.com/005.1-the-target-specification-json-file.html
    let target = r#"{
        "arch": "avr",
        "cpu": "atmega328p",
        "data-layout": "e-P1-p:16:8-i8:8-i16:8-i32:8-i64:8-f32:8-f64:8-n8-a:8",
        "env": "",
        "executables": true,
        "linker": "avr-gcc",
        "linker-flavor": "gcc",
        "linker-is-gnu": true,
        "llvm-target": "avr-unknown-unknown",
        "no-compiler-rt": true,
        "os": "unknown",
        "position-independent-executables": false,
        "exe-suffix": ".elf",
        "eh-frame-header": false,
        "pre-link-args": {
          "gcc": ["-mmcu=atmega328p"]
        },
        "late-link-args": {
          "gcc": ["-lgcc"]
        },
        "target-c-int-width": "16",
        "target-endian": "little",
        "target-pointer-width": "16",
        "vendor": "unknown"
    }"#;

    let sysroot_dir = tempdir().unwrap();
    let target_file = sysroot_dir.path().join("mytarget.json");
    fs::write(&target_file, target).unwrap();

    build_sysroot(
        SysrootBuilder::new(sysroot_dir.path(), &target_file)
            .build_mode(BuildMode::Check)
            .sysroot_config(SysrootConfig::NoStd),
    );
}
