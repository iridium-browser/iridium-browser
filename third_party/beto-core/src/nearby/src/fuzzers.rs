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

use crate::{run_cmd_shell_with_color, YellowStderr};
use std::{fs, path};

pub(crate) fn run_rust_fuzzers(root: &path::Path) -> anyhow::Result<()> {
    log::info!("Running rust fuzzers");
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("presence/xts_aes"),
        "cargo +nightly fuzz run xts-roundtrip -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("presence/ldt"),
        "cargo +nightly fuzz run ldt-roundtrip -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("presence/ldt_np_adv"),
        "cargo +nightly fuzz run ldt-np-decrypt -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("presence/ldt_np_adv"),
        "cargo +nightly fuzz run ldt-np-roundtrip -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("connections/ukey2/ukey2_connections"),
        "cargo +nightly fuzz run fuzz_connection -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("connections/ukey2/ukey2_connections"),
        "cargo +nightly fuzz run fuzz_from_saved_session -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("connections/ukey2/ukey2_connections"),
        "cargo +nightly fuzz run fuzz_handshake -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("crypto/crypto_provider_test"),
        "cargo +nightly fuzz run fuzz_p256 -- -runs=10000 -max_total_time=60",
    )?;
    run_cmd_shell_with_color::<YellowStderr>(
        &root.join("crypto/crypto_provider_test"),
        concat!(
            "cargo +nightly fuzz run fuzz_p256 --features=openssl --no-default-features ",
            "-- -runs=10000 -max_total_time=60"
        ),
    )?;

    Ok(())
}

pub(crate) fn build_ffi_fuzzers(root: &path::Path) -> anyhow::Result<()> {
    log::info!("Building ffi fuzzers");
    // TODO currently broken because the FFI stuff hasn't necessarily been built yet
    let mut build_dir = root.to_path_buf();
    build_dir.push("presence/ldt_np_adv_ffi_fuzz/cmake-build");

    if build_dir.exists() {
        fs::remove_dir_all(&build_dir)?;
    }
    fs::create_dir_all(&build_dir)?;
    run_cmd_shell_with_color::<YellowStderr>(&build_dir, "cmake ../.. -DENABLE_FUZZ=true")?;
    run_cmd_shell_with_color::<YellowStderr>(&build_dir, "make")?;
    fs::remove_dir_all(&build_dir)?;

    Ok(())
}
