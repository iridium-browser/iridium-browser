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
use ldt::*;
use ldt_np_adv::*;
use libfuzzer_sys::fuzz_target;
use xts_aes::XtsAes128;

fuzz_target!(|data: LdtNpRoundtripFuzzInput| {
    let salt = data.salt.into();
    let padder = salt_padder::<16, RustCrypto>(salt);

    let hkdf = np_hkdf::NpKeySeedHkdf::<RustCrypto>::new(&data.key_seed);
    let ldt_enc = LdtEncryptCipher::<16, XtsAes128<RustCrypto>, Swap>::new(&hkdf.legacy_ldt_key());
    let metadata_key_hmac: [u8; 32] = hkdf
        .legacy_metadata_key_hmac_key()
        .calculate_hmac(&data.plaintext[..14]);

    let cipher = build_np_adv_decrypter_from_key_seed::<RustCrypto>(&hkdf, metadata_key_hmac);

    let len = 16 + (data.len as usize % 16);
    let mut ciphertext = data.plaintext;

    ldt_enc.encrypt(&mut ciphertext[..len], &padder).unwrap();
    let plaintext = cipher
        .decrypt_and_verify(&ciphertext[..len], &padder)
        .unwrap();

    assert_eq!(&data.plaintext[..len], plaintext.as_slice());
});

#[derive(Debug, arbitrary::Arbitrary)]
struct LdtNpRoundtripFuzzInput {
    key_seed: [u8; 32],
    salt: [u8; 2],
    // max length = 2 * AES block size - 1
    plaintext: [u8; 31],
    // will % 16 to get a [0-15] value for how much of the plaintext to use past the first block
    len: u8,
}
