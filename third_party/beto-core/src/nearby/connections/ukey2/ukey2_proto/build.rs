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

use protobuf_codegen::Customize;

fn main() {
    protobuf_codegen::Codegen::new()
        .protoc()
        // All inputs and imports from the inputs must reside in `includes` directories.
        .includes(["proto"])
        // Inputs must reside in some of include paths.
        .input("proto/ukey.proto")
        .input("proto/securemessage.proto")
        .input("proto/securegcm.proto")
        .input("proto/device_to_device_messages.proto")
        .customize(Customize::default().gen_mod_rs(true))
        .cargo_out_dir("proto")
        .run_from_script()
}
