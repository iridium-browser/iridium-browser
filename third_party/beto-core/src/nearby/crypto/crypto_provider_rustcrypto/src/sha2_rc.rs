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

use sha2::Digest;

/// RustCrypto implementation for SHA256
pub struct RustCryptoSha256;

impl crypto_provider::sha2::Sha256 for RustCryptoSha256 {
    fn sha256(input: &[u8]) -> [u8; 32] {
        let mut digest = sha2::Sha256::new();
        digest.update(input);
        digest.finalize().into()
    }
}

/// RustCrypto implementation for SHA512
pub struct RustCryptoSha512;

impl crypto_provider::sha2::Sha512 for RustCryptoSha512 {
    fn sha512(input: &[u8]) -> [u8; 64] {
        let mut digest = sha2::Sha512::new();
        digest.update(input);
        digest.finalize().into()
    }
}
