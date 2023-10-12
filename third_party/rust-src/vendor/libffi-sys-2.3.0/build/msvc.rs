use crate::common::*;

const INCLUDE_DIRS: &[&str] = &["libffi", "libffi/include", "include/msvc"];

// libffi expects us to include the same folder in case of x86 and x86_64 architectures
const INCLUDE_DIRS_X86: &[&str] = &["libffi/src/x86"];
const INCLUDE_DIRS_X86_64: &[&str] = &["libffi/src/x86"];
const INCLUDE_DIRS_AARCH64: &[&str] = &["libffi/src/aarch64"];

const BUILD_FILES: &[&str] = &[
    "tramp.c",
    "closures.c",
    "prep_cif.c",
    "raw_api.c",
    "types.c",
];
const BUILD_FILES_X86: &[&str] = &["x86/ffi.c"];
const BUILD_FILES_X86_64: &[&str] = &["x86/ffi.c", "x86/ffiw64.c"];
const BUILD_FILES_AARCH64: &[&str] = &["aarch64/ffi.c"];

fn add_file(build: &mut cc::Build, file: &str) {
    build.file(format!("libffi/src/{}", file));
}

fn unsupported(arch: &str) -> ! {
    panic!("Unsupported architecture: {}", arch)
}

pub fn build_and_link() {
    let target = env::var("TARGET").unwrap();
    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();

    // we should collect all include dirs together with platform specific ones
    // to pass them over to the asm pre-processing step
    let mut all_includes = vec![];
    for each_include in INCLUDE_DIRS {
        all_includes.push(*each_include);
    }
    for each_include in match target_arch.as_str() {
        "x86" => INCLUDE_DIRS_X86,
        "x86_64" => INCLUDE_DIRS_X86_64,
        "aarch64" => INCLUDE_DIRS_AARCH64,
        _ => unsupported(&target_arch),
    } {
        all_includes.push(*each_include);
    }

    let asm_path = pre_process_asm(all_includes.as_slice(), &target, &target_arch);
    let mut build = cc::Build::new();

    for each_include in all_includes {
        build.include(each_include);
    }

    for each_source in BUILD_FILES {
        add_file(&mut build, each_source);
    }
    for each_source in match target_arch.as_str() {
        "x86" => BUILD_FILES_X86,
        "x86_64" => BUILD_FILES_X86_64,
        "aarch64" => BUILD_FILES_AARCH64,
        _ => unsupported(&target_arch),
    } {
        add_file(&mut build, each_source);
    }

    build
        .file(asm_path)
        .define("WIN32", None)
        .define("_LIB", None)
        .define("FFI_BUILDING", None)
        .warnings(false)
        .compile("libffi");
}

pub fn probe_and_link() {
    // At the time of writing it wasn't clear if MSVC builds will support
    // dynamic linking of libffi; assuming it's even installed. To ensure
    // existing MSVC setups continue to work, we just compile libffi from source
    // and statically link it.
    build_and_link();
}

pub fn pre_process_asm(include_dirs: &[&str], target: &str, target_arch: &str) -> String {
    let folder_name = match target_arch {
        "x86" => "x86",
        "x86_64" => "x86",
        "aarch64" => "aarch64",
        _ => unsupported(target_arch),
    };

    let file_name = match target_arch {
        "x86" => "sysv_intel",
        "x86_64" => "win64_intel",
        "aarch64" => "win64_armasm",
        _ => unsupported(target_arch),
    };

    let mut cmd = cc::windows_registry::find(target, "cl.exe").expect("Could not locate cl.exe");

    // When cross-compiling we should provide MSVC includes as part of the INCLUDE env.var
    let build = cc::Build::new();
    for (key, value) in build.get_compiler().env() {
        if key.to_string_lossy() == "INCLUDE" {
            cmd.env(
                "INCLUDE",
                format!("{};{}", value.to_string_lossy(), include_dirs.join(";")),
            );
        }
    }

    cmd.arg("/EP");
    cmd.arg(format!("libffi/src/{}/{}.S", folder_name, file_name));

    let out_path = format!("libffi/src/{}/{}.asm", folder_name, file_name);
    let asm_file = fs::File::create(&out_path).expect("Could not create output file");

    cmd.stdout(asm_file);

    run_command("Pre-process ASM", &mut cmd);

    out_path
}
