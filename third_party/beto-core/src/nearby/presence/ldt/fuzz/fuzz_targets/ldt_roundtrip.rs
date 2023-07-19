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
use libfuzzer_sys::fuzz_target;
use xts_aes::XtsAes128;

fuzz_target!(|data: LdtFuzzInput| {
    let ldt_enc =
        LdtEncryptCipher::<16, XtsAes128<RustCrypto>, Swap>::new(&LdtKey::from_concatenated(&data.ldt_key));
    let ldt_dec =
        LdtDecryptCipher::<16, XtsAes128<RustCrypto>, Swap>::new(&LdtKey::from_concatenated(&data.ldt_key));
    let len = 16 + (data.len as usize % 16);
    let padder: XorPadder<16> = data.xor_padder.clone().into();

    let mut buffer = data.plaintext.clone();
    ldt_enc.encrypt(&mut buffer[..len], &padder).unwrap();
    ldt_dec.decrypt(&mut buffer[..len], &padder).unwrap();
    assert_eq!(data.plaintext, buffer);
});

#[derive(Debug, arbitrary::Arbitrary)]
struct LdtFuzzInput {
    // XTS-AES128 keys
    ldt_key: [u8; 64],
    xor_padder: [u8; 16],
    // max length = 2 * AES block size - 1
    plaintext: [u8; 31],
    // will % 16 to get a [0-15] value for how much of the plaintext to use past the first block
    len: u8,
}
