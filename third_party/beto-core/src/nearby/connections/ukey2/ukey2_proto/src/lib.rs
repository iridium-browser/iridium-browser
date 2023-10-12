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

#[cfg(feature = "cargo")]
pub mod ukey2_all_proto {
    include!(concat!(env!("OUT_DIR"), "/proto/mod.rs"));
}

#[cfg(not(feature = "cargo"))]
pub mod ukey2_all_proto {
    pub mod device_to_device_messages;
    pub mod securegcm;
    pub mod securemessage;
    pub mod ukey;
}

pub use protobuf;

#[cfg(all(test, feature = "cargo"))]
mod tests {
    use std::{fs, path::Path};

    #[test]
    fn check_proto_needs_update() {
        let proto_src_dir = concat!(env!("CARGO_MANIFEST_DIR"), "/src/ukey2_all_proto/");
        for file in fs::read_dir(proto_src_dir).unwrap() {
            let actual_file = file.unwrap();
            let file_name_str = actual_file.file_name();
            if actual_file.file_type().unwrap().is_file()
                && file_name_str.to_string_lossy().ends_with(".rs")
            {
                println!("Checking file {}", &file_name_str.to_string_lossy());
                let newly_generated_file_path =
                    env!("OUT_DIR").to_string() + "/proto/" + &file_name_str.to_string_lossy();
                let current_file_path = proto_src_dir.to_owned() + file_name_str.to_str().unwrap();
                if let Some(diff_str) = diff_file(&newly_generated_file_path, &current_file_path) {
                    panic!(
                        "file '{}' needs to be updated.\n\n{}\n",
                        file_name_str.to_string_lossy(),
                        diff_str
                    );
                }
            }
        }
    }

    // Ignore the first 17 lines of the generated proto files, since that contains the apache header and
    // version numbers of rust-protobuf and protoc that may differ based on the host machine configuration.
    const DIFF_IGNORE_LINES: usize = 17;

    fn diff_file<P: AsRef<Path>>(left: P, right: P) -> Option<String> {
        let left_content = fs::read_to_string(left).unwrap();
        let right_content = fs::read_to_string(right).unwrap();
        let diff = &diff::lines(&left_content, &right_content)[DIFF_IGNORE_LINES..];
        let line_diffs = diff
            .iter()
            .filter_map(|d| match d {
                diff::Result::Left(l) => Some(format!("-{l}")),
                diff::Result::Both(_, _) => None,
                diff::Result::Right(r) => Some(format!("+{r}")),
            })
            .collect::<Vec<String>>();
        if line_diffs.is_empty() {
            None
        } else {
            Some(line_diffs.join("\n"))
        }
    }
}
