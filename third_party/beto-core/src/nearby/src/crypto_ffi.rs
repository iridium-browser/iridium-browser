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

use crate::support::{run_cmd_shell, run_cmd_shell_with_color, YellowStderr};
use crate::BuildBoringSslOptions;
use anyhow::anyhow;
use owo_colors::OwoColorize as _;
use semver::{Version, VersionReq};
use std::{
    env, fs,
    path::{Path, PathBuf},
};

pub fn build_boringssl(root: &Path, options: &BuildBoringSslOptions) -> anyhow::Result<()> {
    let bindgen_version_req = VersionReq::parse(">=0.61.0")?;
    let bindgen_version = get_bindgen_version()?;

    if !bindgen_version_req.matches(&bindgen_version) {
        return Err(anyhow!("Bindgen does not match expected version: {bindgen_version_req}"));
    }

    let mut vendor_dir =
        root.parent().ok_or_else(|| anyhow!("project root dir no parent dir"))?.to_path_buf();
    vendor_dir.push("boringssl-build");
    fs::create_dir_all(&vendor_dir)?;

    let mut build_dir = clone_repo_if_needed(
        &vendor_dir,
        "boringssl",
        "https://boringssl.googlesource.com/boringssl",
    )?;

    run_cmd_shell_with_color::<YellowStderr>(
        &build_dir,
        format!("git checkout {}", &options.commit_hash),
    )?;

    build_dir.push("build");
    fs::create_dir_all(&build_dir)?;

    let target = run_cmd_shell_with_color::<YellowStderr>(&vendor_dir, "rustc -vV")?
        .stdout()
        .lines()
        .find(|l| l.starts_with("host: "))
        .and_then(|l| l.split_once(' '))
        .ok_or_else(|| anyhow!("Couldn't get rustc target"))?
        .1
        .to_string();
    let target = shell_escape::escape(target.into());
    run_cmd_shell_with_color::<YellowStderr>(
        &build_dir,
        format!(
            "cmake -G Ninja .. -DRUST_BINDINGS={} -DCMAKE_POSITION_INDEPENDENT_CODE=true",
            target
        ),
    )?;
    run_cmd_shell(&build_dir, "ninja")?;

    Ok(())
}

pub fn check_boringssl(root: &Path, options: &BuildBoringSslOptions) -> anyhow::Result<()> {
    log::info!("Checking boringssl");

    build_boringssl(root, options)?;

    let mut bssl_dir = root.to_path_buf();
    bssl_dir.push("crypto/crypto_provider_boringssl");

    run_cmd_shell(&bssl_dir, "cargo check")?;
    run_cmd_shell(&bssl_dir, "cargo fmt --check")?;
    run_cmd_shell(&bssl_dir, "cargo clippy --all-targets")?;
    run_cmd_shell(&bssl_dir, "cargo test -- --color=always")?;
    run_cmd_shell(&bssl_dir, "cargo doc --no-deps")?;
    Ok(())
}

pub fn prepare_patched_rust_openssl(root: &Path) -> anyhow::Result<()> {
    let mut vendor_dir =
        root.parent().ok_or_else(|| anyhow!("project root dir no parent dir"))?.to_path_buf();
    vendor_dir.push("boringssl-build");
    fs::create_dir_all(&vendor_dir)?;

    let repo_dir = clone_repo_if_needed(
        &vendor_dir,
        "rust-openssl",
        "https://github.com/sfackler/rust-openssl.git",
    )?;

    run_cmd_shell_with_color::<YellowStderr>(
        &repo_dir,
        "git checkout 11797d9ecb73e94b7f55a49274318abc9dc074d2",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(&repo_dir, "git branch -f BASE_COMMIT")?;
    run_cmd_shell_with_color::<YellowStderr>(
        &repo_dir,
        format!(
            "git am {}/scripts/openssl-patches/*.patch",
            root.to_str().ok_or_else(|| anyhow!("root dir is not UTF-8"))?
        ),
    )?;

    println!("{}", "Preparation complete. The required repositories are downloaded to `beto-rust/boringssl-build`. If
you need to go back to a clean state, you can remove that directory and rerun this script.

You can now build and test with boringssl using the following command
  `cargo --config .cargo/config-boringssl.toml test -p crypto_provider* --features=boringssl,std`
".cyan());

    Ok(())
}

pub fn check_openssl(root: &Path) -> anyhow::Result<()> {
    log::info!("Checking rust openssl");
    prepare_patched_rust_openssl(root)?;

    // test the openssl crate with the boringssl feature
    run_cmd_shell(
        root,
        concat!(
            "cargo --config .cargo/config-boringssl.toml test -p crypto_provider_openssl ",
            "--features=boringssl -- --color=always"
        ),
    )?;

    Ok(())
}

/// If the repo dir doesn't exist, or errors when running `git fetch -a`, re-clone it.
///
/// Returns the repo dir
fn clone_repo_if_needed(
    dir: &Path,
    repo_subdir_name: &str,
    repo_url: &str,
) -> anyhow::Result<PathBuf> {
    let mut repo_dir = dir.to_path_buf();
    repo_dir.push(repo_subdir_name);

    if run_cmd_shell_with_color::<YellowStderr>(&repo_dir, "git fetch -a").is_err() {
        // delete it and start over
        if repo_dir.exists() {
            fs::remove_dir_all(&repo_dir)?;
        }
        run_cmd_shell_with_color::<YellowStderr>(dir, format!("git clone {}", repo_url))?;
    }

    Ok(repo_dir)
}

fn get_bindgen_version() -> anyhow::Result<Version> {
    let bindgen_version_output = run_cmd_shell(&env::current_dir().unwrap(), "bindgen --version")?;

    let version = bindgen_version_output
        .stdout()
        .lines()
        .next()
        .ok_or(anyhow!("bindgen version output stream is empty"))?
        .strip_prefix("bindgen ")
        .ok_or(anyhow!("bindgen version output missing expected prefix of \"bindgen \""))?
        .parse::<Version>()?;

    Ok(version)
}
