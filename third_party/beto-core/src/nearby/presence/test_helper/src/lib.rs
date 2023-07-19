// Copyright 2022 Google LLC
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
#![forbid(unsafe_code)]
#![deny(missing_docs)]

//! Helper crate for common functions used in testing

use std::fs;
use std::io::Read;

/// Returns data file path for specific build system. Input is the path to the file relative to the
/// workspace root dir
pub fn get_data_file(file: &str) -> std::path::PathBuf {
    let mut full_path = std::path::PathBuf::from(env!("WORKSPACE_DIR"));
    full_path.push(file);
    full_path
}

/// Opens a file at the specified path (relative to the workspace root)
/// and yields its contents as a string
pub fn load_data_file_contents_as_string(file: &str) -> String {
    let full_path = get_data_file(file);
    let mut file = fs::File::open(full_path).expect("Should be able to open data file");
    let mut data = String::new();
    file.read_to_string(&mut data).expect("should be able to read data file");
    data
}

/// Opens a json file at the specified path and parses it into a value
pub fn parse_json_data_file(file: &str) -> serde_json::Value {
    let data = load_data_file_contents_as_string(file);
    serde_json::de::from_str(data.as_str()).expect("should be able to parse json date file")
}

/// extract a string from a jsonvalue
pub fn extract_key_str<'a>(value: &'a serde_json::Value, key: &str) -> &'a str {
    value[key].as_str().unwrap()
}

/// Decode a hex-encoded vec at `key`
pub fn extract_key_vec(value: &serde_json::Value, key: &str) -> Vec<u8> {
    hex::decode(value[key].as_str().unwrap()).unwrap()
}

/// Decode a hex-encoded array at `key`
pub fn extract_key_array<const N: usize>(value: &serde_json::Value, key: &str) -> [u8; N] {
    extract_key_vec(value, key).try_into().unwrap()
}

/// Convert a hex string to a Vec of the hex bytes
pub fn string_to_hex(str: &str) -> Vec<u8> {
    hex::decode(str).unwrap()
}
