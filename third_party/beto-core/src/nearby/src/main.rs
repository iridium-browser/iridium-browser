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

extern crate core;

use clap::Parser as _;
use env_logger::Env;
use std::{env, path};
use support::*;

mod crypto_ffi;
mod ffi;
mod file_header;
mod fuzzers;
mod jni;
mod license;
mod support;
mod ukey2;

fn main() -> anyhow::Result<()> {
    env_logger::Builder::from_env(Env::default().default_filter_or("info")).init();
    let cli: Cli = Cli::parse();

    let root_dir = path::PathBuf::from(
        env::var("CARGO_MANIFEST_DIR").expect("Must be run via Cargo to establish root directory"),
    );

    match cli.subcommand {
        Subcommand::CheckEverything { ref check_options, ref bssl_options } => {
            check_everything(&root_dir, check_options, bssl_options)?
        }
        Subcommand::CheckWorkspace(ref options) => check_workspace(&root_dir, options)?,
        Subcommand::FfiCheckEverything => ffi::check_everything(&root_dir)?,
        Subcommand::BuildBoringssl(ref bssl_options) => {
            crypto_ffi::build_boringssl(&root_dir, bssl_options)?
        }
        Subcommand::CheckBoringssl(ref bssl_options) => {
            crypto_ffi::check_boringssl(&root_dir, bssl_options)?
        }
        Subcommand::PrepareRustOpenssl => crypto_ffi::prepare_patched_rust_openssl(&root_dir)?,
        Subcommand::CheckOpenssl => crypto_ffi::check_openssl(&root_dir)?,
        Subcommand::RunRustFuzzers => fuzzers::run_rust_fuzzers(&root_dir)?,
        Subcommand::BuildFfiFuzzers => fuzzers::build_ffi_fuzzers(&root_dir)?,
        Subcommand::CheckLicenseHeaders => license::check_license_headers(&root_dir)?,
        Subcommand::AddLicenseHeaders => license::add_license_headers(&root_dir)?,
        Subcommand::CheckLdtFfi => ffi::check_ldt_ffi(&root_dir)?,
        Subcommand::CheckUkey2Ffi => ukey2::check_ukey2_ffi(&root_dir)?,
        Subcommand::CheckLdtJni => jni::check_ldt_jni(&root_dir)?,
        Subcommand::CheckNpFfi => ffi::check_np_ffi(&root_dir)?,
        Subcommand::CheckCmakeProjects => ffi::check_cmake_projects(&root_dir)?,
    }

    Ok(())
}

pub fn check_workspace(root: &path::Path, options: &CheckOptions) -> anyhow::Result<()> {
    log::info!("Running cargo checks on workspace");

    let fmt_command = if options.reformat { "cargo fmt" } else { "cargo fmt --check" };

    for cargo_cmd in [
        // ensure formatting is correct (Check for it first because it is fast compared to running tests)
        fmt_command,
        // make sure everything compiles
        "cargo check --workspace --all-targets",
        // run all the tests
        //TODO: re-enable the openssl tests, this was potentially failing due to UB code in the
        // upstream rust-openssl crate's handling of empty slices. This repros consistently when
        // using the rust-openssl crate backed by openssl-sys on Ubuntu 20.04.
        "cargo test --workspace --quiet --exclude crypto_provider_openssl -- --color=always",
        // ensure the docs are valid (cross-references to other code, etc)
        "RUSTDOCFLAGS='--deny warnings' cargo doc --workspace --no-deps",
        "cargo clippy --all-targets --workspace -- --deny warnings",
        "cargo deny --workspace check",
    ] {
        run_cmd_shell(root, cargo_cmd)?;
    }

    Ok(())
}
pub fn check_everything(
    root: &path::Path,
    check_options: &CheckOptions,
    bssl_options: &BuildBoringSslOptions,
) -> anyhow::Result<()> {
    license::check_license_headers(root)?;
    check_workspace(root, check_options)?;
    crypto_ffi::check_boringssl(root, bssl_options)?;
    crypto_ffi::check_openssl(root)?;
    ffi::check_everything(root)?;
    jni::check_ldt_jni(root)?;
    ukey2::check_ukey2_ffi(root)?;
    fuzzers::run_rust_fuzzers(root)?;
    fuzzers::build_ffi_fuzzers(root)?;

    Ok(())
}

#[derive(clap::Parser)]
struct Cli {
    #[clap(subcommand)]
    subcommand: Subcommand,
}

#[derive(clap::Subcommand, Debug, Clone)]
enum Subcommand {
    /// Checks everything in beto-rust
    CheckEverything {
        #[command(flatten)]
        check_options: CheckOptions,
        #[command(flatten)]
        bssl_options: BuildBoringSslOptions,
    },
    /// Checks everything included in the top level workspace
    CheckWorkspace(CheckOptions),
    /// Clones boringssl and uses bindgen to generate the rust crate
    BuildBoringssl(BuildBoringSslOptions),
    /// Run crypto provider tests using boringssl backend
    CheckBoringssl(BuildBoringSslOptions),
    /// Applies AOSP specific patches to the 3p `openssl` crate so that it can use a boringssl
    /// backend
    PrepareRustOpenssl,
    /// Run crypto provider tests using openssl crate with boringssl backend
    CheckOpenssl,
    /// Build and run pure Rust fuzzers for 10000 runs
    RunRustFuzzers,
    /// Build FFI fuzzers
    BuildFfiFuzzers,
    /// Builds and runs tests for all C/C++ projects. This is a combination of CheckNpFfi,
    /// CheckLdtFfi, and CheckCmakeBuildAndTests
    FfiCheckEverything,
    /// Builds the crate checks the cbindgen generation of C/C++ bindings
    CheckNpFfi,
    /// Builds ldt_np_adv_ffi crate with all possible different sets of feature flags
    CheckLdtFfi,
    /// Checks the CMake build and runs all of the C/C++ tests
    CheckCmakeProjects,
    /// Checks the workspace 3rd party crates and makes sure they have a valid license
    CheckLicenseHeaders,
    /// Generate new headers for any files that are missing them
    AddLicenseHeaders,
    /// Builds and runs tests for the UKEY2 FFI
    CheckUkey2Ffi,
    /// Checks the build of ldt_jni wrapper with non default features, ie rust-openssl, and boringssl
    CheckLdtJni,
}

#[derive(clap::Args, Debug, Clone, Default)]
pub struct CheckOptions {
    #[arg(long, help = "reformat files with cargo fmt")]
    reformat: bool,
}

#[derive(clap::Args, Debug, Clone, Default)]
pub struct BuildBoringSslOptions {
    #[arg(
        long,
        // the commit after this one causes failures in rust-openssl
        default_value = "d995d82ad53133017e34b009e9c6912b2ef6aeb7",
        help = "Commit hash to use when checking out boringssl"
    )]
    commit_hash: String,
}
