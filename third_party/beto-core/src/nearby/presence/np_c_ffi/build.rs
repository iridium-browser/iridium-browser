// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use cbindgen::ItemType::*;
use std::vec;

const C_CONFIG: &str = "cbindgen_configs/c_config.toml";
const C_OUTPUT_HEADER_FILE: &str = "include/c/np_c_ffi.h";

const CPP_CONFIG: &str = "cbindgen_configs/cpp_config.toml";
const CPP_INCLUDE_DIR_BASE: &str = "include/cpp/";
const CPP_PUBLIC_HEADER_FILE: &str = "np_cpp_ffi_types.h";
const CPP_INTERNAL_HEADER_FILE: &str = "np_cpp_ffi_functions.h";

fn main() {
    let crate_dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();

    let config_file = format!("{crate_dir}/{C_CONFIG}");
    let config = cbindgen::Config::from_file(config_file).expect("Config file should exist");
    generate_c_header(&crate_dir, config);

    let config_file = format!("{crate_dir}/{CPP_CONFIG}");
    let config = cbindgen::Config::from_file(config_file).expect("Config file should exist");
    generate_private_cpp_header(&crate_dir, config.clone());
    generate_public_cpp_header(&crate_dir, config);
}

fn generate_c_header(crate_dir: &String, config: cbindgen::Config) {
    let output_header_file = format!("{crate_dir}/{C_OUTPUT_HEADER_FILE}");
    generate_header(&output_header_file, crate_dir, config);
}

fn generate_private_cpp_header(crate_dir: &String, mut config: cbindgen::Config) {
    config.export.item_types.append(&mut vec![Functions]);
    config.includes.push(CPP_PUBLIC_HEADER_FILE.to_string());
    let output_header_file =
        format!("{crate_dir}/{CPP_INCLUDE_DIR_BASE}/{CPP_INTERNAL_HEADER_FILE}");
    generate_header(&output_header_file, crate_dir, config);
}

fn generate_public_cpp_header(crate_dir: &String, mut config: cbindgen::Config) {
    config.export.item_types.append(&mut vec![Enums, Structs]);
    let output_header_file = format!("{crate_dir}/{CPP_INCLUDE_DIR_BASE}/{CPP_PUBLIC_HEADER_FILE}");
    generate_header(&output_header_file, crate_dir, config);
}

fn generate_header(output_file: &String, crate_dir: &String, config: cbindgen::Config) {
    let _ = cbindgen::generate_with_config(crate_dir, config)
        .map_err(|e| println!("cargo:warning=ERROR: {e}"))
        .expect("c header file generation failed")
        .write_to_file(output_file);
}
