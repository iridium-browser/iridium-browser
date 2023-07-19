#![no_main]
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

use libfuzzer_sys::fuzz_target;
use ukey2_connections::{D2DConnectionContextV1, DeserializeError};
use crypto_provider_rustcrypto::RustCrypto;

const PROTOCOL_VERSION: u8 = 1;

fuzz_target!(|input: [u8; 73]| {
    let result = D2DConnectionContextV1::from_saved_session::<RustCrypto>(&input);
    if input[0] != PROTOCOL_VERSION {
        assert_eq!(result.unwrap_err(), DeserializeError::BadProtocolVersion);
    } else {
        assert!(result.is_ok());
    }
});
