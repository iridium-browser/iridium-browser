#![no_main]
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

use crypto_provider_rustcrypto::RustCrypto;
use ldt_np_adv::*;
use libfuzzer_sys::fuzz_target;

fuzz_target!(|data: LdtNpDecryptFuzzInput| {
    // try to decrypt data that won't pass validation
    let salt = data.salt.into();
    let padder = salt_padder::<16, RustCrypto>(salt);
    let hkdf = np_hkdf::NpKeySeedHkdf::<RustCrypto>::new(&data.key_seed);
    let cipher = build_np_adv_decrypter_from_key_seed::<RustCrypto>(&hkdf, data.metadata_key_hmac);

    let len = 16 + (data.len as usize % 16);
    let ciphertext = data.ciphertext;
    let err = cipher
        .decrypt_and_verify(&ciphertext[..len], &padder)
        .unwrap_err();

    assert_eq!(LdtAdvDecryptError::MacMismatch, err);
});

#[derive(Debug, arbitrary::Arbitrary)]
struct LdtNpDecryptFuzzInput {
    key_seed: [u8; 32],
    salt: [u8; 2],
    // bogus hmac that won't match plaintext
    metadata_key_hmac: [u8; 32],
    // max length = 2 * AES block size - 1
    ciphertext: [u8; 31],
    // will % 16 to get a [0-15] value for how much of the plaintext to use past the first block
    len: u8,
}
