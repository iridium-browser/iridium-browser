use crate::common::*;
use std::process::Command;

pub fn build_and_link() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let build_dir = Path::new(&out_dir).join("libffi-build");
    let prefix = Path::new(&out_dir).join("libffi-root");
    let libdir = Path::new(&prefix).join("lib");
    let libdir64 = Path::new(&prefix).join("lib64");

    // Copy LIBFFI_DIR into build_dir to avoid an unnecessary build
    if let Err(e) = fs::remove_dir_all(&build_dir) {
        assert_eq!(
            e.kind(),
            std::io::ErrorKind::NotFound,
            "can't remove the build directory: {}",
            e
        );
    }
    run_command(
        "Copying libffi into the build directory",
        Command::new("cp").arg("-R").arg("libffi").arg(&build_dir),
    );

    // Generate configure, run configure, make, make install
    configure_libffi(prefix, &build_dir);

    run_command(
        "Building libffi",
        Command::new("make")
            .env_remove("DESTDIR")
            .arg("install")
            .current_dir(&build_dir),
    );

    // Cargo linking directives
    println!("cargo:rustc-link-lib=static=ffi");
    println!("cargo:rustc-link-search={}", libdir.display());
    println!("cargo:rustc-link-search={}", libdir64.display());
}

pub fn probe_and_link() {
    println!("cargo:rustc-link-lib=dylib=ffi");
}

pub fn configure_libffi(prefix: PathBuf, build_dir: &Path) {
    let mut command = Command::new("sh");

    command
        .arg("./configure")
        .arg("--with-pic")
        .arg("--disable-shared")
        .arg("--disable-docs");

    let target = std::env::var("TARGET").unwrap();
    let host = std::env::var("HOST").unwrap();
    if target != host {
        let cross_host = match target.as_str() {
            // Autoconf uses riscv64 while Rust uses riscv64gc for the architecture
            "riscv64gc-unknown-linux-gnu" => "riscv64-unknown-linux-gnu",
            // Autoconf does not yet recognize illumos, but Solaris should be fine
            "x86_64-unknown-illumos" => "x86_64-unknown-solaris",
            // configure.host does not extract `ios-sim` as OS.
            // The sources for `ios-sim` should be the same as `ios`.
            "aarch64-apple-ios-sim" => "aarch64-apple-ios",
            // Everything else should be fine to pass straight through
            other => other,
        };
        command.arg(format!("--host={}", cross_host));
    }

    let mut c_cfg = cc::Build::new();
    c_cfg
        .cargo_metadata(false)
        .target(&target)
        .warnings(false)
        // Work around a build failure with clang-16 and newer.  Can be removed
        // once https://github.com/libffi/libffi/pull/764 is merged.
        .flag_if_supported("-Wno-implicit-function-declaration")
        .host(&host);
    let c_compiler = c_cfg.get_compiler();

    command.env("CC", c_compiler.path());

    let mut cflags = c_compiler.cflags_env();
    match env::var_os("CFLAGS") {
        None => (),
        Some(flags) => {
            cflags.push(" ");
            cflags.push(&flags);
        }
    }
    command.env("CFLAGS", cflags);

    for (k, v) in c_compiler.env().iter() {
        command.env(k, v);
    }

    command.current_dir(&build_dir);

    if cfg!(windows) {
        // When using MSYS2, OUT_DIR will be a Windows like path such as
        // C:\foo\bar. Unfortunately, the various scripts used for building
        // libffi do not like such a path, so we have to turn this into a Unix
        // like path such as /c/foo/bar.
        //
        // This code assumes the path only uses : for the drive letter, and only
        // uses \ as a component separator. It will likely break for file paths
        // that include a :.
        let mut msys_prefix = prefix
            .to_str()
            .unwrap()
            .replace(":\\", "/")
            .replace("\\", "/");

        msys_prefix.insert(0, '/');

        command.arg("--prefix").arg(msys_prefix);
    } else {
        command.arg("--prefix").arg(prefix);
    }

    run_command("Configuring libffi", &mut command);
}
