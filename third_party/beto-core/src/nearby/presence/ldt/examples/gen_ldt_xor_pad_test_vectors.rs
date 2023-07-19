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

use crypto_provider::aes::BLOCK_SIZE;
use crypto_provider::{aes, CryptoProvider, CryptoRng};
use crypto_provider_rustcrypto::RustCrypto;
use ldt::{LdtEncryptCipher, LdtKey, Swap, XorPadder};
use rand::{Rng as _, SeedableRng as _};
use rand_ext::*;
use serde_json::json;
use xts_aes::XtsAes128;

fn main() {
    let mut rng = rand::rngs::StdRng::from_entropy();
    let mut cp_rng = <RustCrypto as CryptoProvider>::CryptoRng::new();

    let mut array = Vec::<serde_json::Value>::new();
    for _ in 0..1_000 {
        let len = rng.gen_range(BLOCK_SIZE..BLOCK_SIZE * 2);
        let plaintext = random_vec_rc(&mut rng, len);
        let key = LdtKey::from_random::<RustCrypto>(&mut cp_rng);
        let pad_xor: [u8; aes::BLOCK_SIZE] = random_bytes_rc(&mut rng);

        let ldt_enc = LdtEncryptCipher::<BLOCK_SIZE, XtsAes128<RustCrypto>, Swap>::new(&key);

        let mut ciphertext = plaintext.clone();
        ldt_enc.encrypt(&mut ciphertext, &XorPadder::from(pad_xor)).unwrap();

        array.push(json!({
            "plaintext": hex::encode_upper(&plaintext),
            "ciphertext": hex::encode_upper(&ciphertext),
            "key": hex::encode_upper(key.as_concatenated()),
            "xor_pad": hex::encode_upper(pad_xor)
        }));
    }

    println!("{}", serde_json::ser::to_string_pretty(&array).unwrap());
}
