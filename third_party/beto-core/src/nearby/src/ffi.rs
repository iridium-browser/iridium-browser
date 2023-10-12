// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use crate::{run_cmd_shell, run_cmd_shell_with_color, YellowStderr};
use std::{fs, path};

// wrapper for checking all ffi related things
pub fn check_everything(root: &path::Path) -> anyhow::Result<()> {
    check_np_ffi(root)?;
    check_ldt_ffi(root)?;
    check_cmake_projects(root)?;

    Ok(())
}

pub fn check_np_ffi(root: &path::Path) -> anyhow::Result<()> {
    log::info!("Checking np_c_ffi cargo build");
    let mut ffi_dir = root.to_path_buf();
    ffi_dir.push("presence/np_c_ffi");
    for cargo_cmd in [
        "fmt --check",
        // Default build, RustCrypto + no_std
        "build --release",
        "clippy --release",
    ] {
        run_cmd_shell(&ffi_dir, format!("cargo {}", cargo_cmd))?;
    }
    Ok(())
}

pub fn check_ldt_ffi(root: &path::Path) -> anyhow::Result<()> {
    log::info!("Checking LFT ffi cargo build");
    let mut ffi_dir = root.to_path_buf();
    ffi_dir.push("presence/ldt_np_adv_ffi");

    for cargo_cmd in [
        "fmt --check",
        // Default build, RustCrypto + no_std
        "build --release",
        // Turn on std, still using RustCrypto
        "build --features=std",
        // Turn off default features and try to build with std",
        "build --no-default-features --features=std",
        // Turn off RustCrypto and use openssl
        "build --no-default-features --features=openssl",
        // Turn off RustCrypto and use boringssl
        "--config .cargo/config-boringssl.toml build --no-default-features --features=boringssl",
        "doc --no-deps",
        "clippy --release",
        "clippy --features=std",
        "clippy --no-default-features --features=openssl",
        "clippy --no-default-features --features=std",
        // TODO also clippy for boringssl?
        "deny check",
    ] {
        run_cmd_shell(&ffi_dir, format!("cargo {}", cargo_cmd))?;
    }

    Ok(())
}

pub fn check_cmake_projects(root: &path::Path) -> anyhow::Result<()> {
    log::info!("Checking CMake build and tests (for ffi c/c++ code)");
    let mut build_dir = root.to_path_buf();
    build_dir.push("presence/cmake-build");
    fs::create_dir_all(&build_dir)?;

    run_cmd_shell_with_color::<YellowStderr>(&build_dir, "cmake .. -DENABLE_TESTS=true")?;
    run_cmd_shell_with_color::<YellowStderr>(&build_dir, "cmake --build .")?;

    // run the np_cpp_ffi unit tests
    let mut np_cpp_tests_dir = build_dir.clone();
    np_cpp_tests_dir.push("np_cpp_ffi/tests");
    run_cmd_shell_with_color::<YellowStderr>(&np_cpp_tests_dir, "ctest")?;

    // Run the LDT ffi unit tests. These are rebuilt and tested against all of the different
    // Cargo build configurations based on the feature flags.
    let mut ldt_tests_dir = build_dir.clone();
    ldt_tests_dir.push("ldt_np_c_sample/tests");

    let mut ldt_ffi_crate_dir = root.to_path_buf();
    ldt_ffi_crate_dir.push("presence/ldt_np_adv_ffi");

    for build_config in [
        // test with default build settings (rustcrypto, no_std)
        "build --release",
        // test with std and default features
        "build --features std --release",
        // test with boringssl crypto feature flag
        "--config .cargo/config-boringssl.toml build --no-default-features --features boringssl --release",
        // test with openssl feature flag
        "build --no-default-features --features openssl --release",
        // test without defaults and std feature flag
        "build --no-default-features --features std --release",
    ] {
        run_cmd_shell(&ldt_ffi_crate_dir, format!("cargo {}", build_config))?;
        // Force detection of updated `ldt_np_adv_ffi` static lib
        run_cmd_shell_with_color::<YellowStderr>(&build_dir, "rm -rf ldt_np_c_sample/tests/*")?;
        run_cmd_shell_with_color::<YellowStderr>(&build_dir, "cmake --build .")?;
        run_cmd_shell_with_color::<YellowStderr>(&ldt_tests_dir, "ctest")?;
    }

    Ok(())
}
