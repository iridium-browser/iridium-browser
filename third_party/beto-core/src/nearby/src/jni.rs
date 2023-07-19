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

use crate::support::run_cmd_shell;
use std::path;

// This has to happen after both boringssl has been built and prepare rust openssl patches has been run.
pub fn check_ldt_jni(root: &path::Path) -> anyhow::Result<()> {
    for feature in ["opensslbssl", "boringssl"] {
        run_cmd_shell(root, format!("cargo --config .cargo/config-boringssl.toml build -p ldt_np_jni --no-default-features --features={}", feature))?;
    }

    Ok(())
}
