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
use ldt_tbc::{TweakableBlockCipherDecrypter, TweakableBlockCipherEncrypter};
use libfuzzer_sys::fuzz_target;
use xts_aes::*;

fuzz_target!(|data: XtsFuzzInput| {
    // XTS requires at least one block
    if data.plaintext.len() < 16 {
        return;
    }

    let xts_enc =
        <XtsAes128<RustCrypto> as ldt_tbc::TweakableBlockCipher<16>>::EncryptionCipher::new(
            &XtsAes128Key::from(&data.key),
        );
    let xts_dec =
        <XtsAes128<RustCrypto> as ldt_tbc::TweakableBlockCipher<16>>::DecryptionCipher::new(
            &XtsAes128Key::from(&data.key),
        );

    let tweak: Tweak = data.tweak.into();

    let mut buffer = data.plaintext.clone();

    xts_enc
        .encrypt_data_unit(tweak.clone(), &mut buffer[..])
        .unwrap();
    xts_dec.decrypt_data_unit(tweak, &mut buffer[..]).unwrap();
    assert_eq!(data.plaintext, buffer);
});

#[derive(Debug, arbitrary::Arbitrary)]
struct XtsFuzzInput {
    key: [u8; 32],
    tweak: [u8; 16],
    // min length = AES block size
    plaintext: Vec<u8>,
}
