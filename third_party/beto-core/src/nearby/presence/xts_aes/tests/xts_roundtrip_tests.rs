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

use alloc::vec::Vec;
use crypto_provider::aes::*;
use crypto_provider::CryptoProvider;
use crypto_provider_default::CryptoProviderImpl;
use ldt_tbc::TweakableBlockCipherDecrypter;
use ldt_tbc::TweakableBlockCipherEncrypter;
use rand::{self, distributions, Rng as _};
use rand_ext::seeded_rng;
use xts_aes::{Tweak, XtsAes128Key, XtsAes256Key, XtsDecrypter, XtsEncrypter, XtsKey};
extern crate alloc;

#[test]
fn roundtrip_self() {
    let mut rng = seeded_rng();
    for _ in 0..100_000 {
        if rng.gen() {
            let mut key = [0_u8; 32];
            rng.fill(&mut key);
            do_roundtrip(
                XtsEncrypter::<<CryptoProviderImpl as CryptoProvider>::Aes128, _>::new(
                    &XtsAes128Key::from(&key),
                ),
                XtsDecrypter::<<CryptoProviderImpl as CryptoProvider>::Aes128, _>::new(
                    &XtsAes128Key::from(&key),
                ),
                &mut rng,
            )
        } else {
            let mut key = [0_u8; 64];
            rng.fill(&mut key);
            do_roundtrip(
                XtsEncrypter::<<CryptoProviderImpl as CryptoProvider>::Aes256, _>::new(
                    &XtsAes256Key::from(&key),
                ),
                XtsDecrypter::<<CryptoProviderImpl as CryptoProvider>::Aes256, _>::new(
                    &XtsAes256Key::from(&key),
                ),
                &mut rng,
            )
        };
    }

    fn do_roundtrip<A: Aes<Key = K::BlockCipherKey>, K: XtsKey, R: rand::Rng>(
        xts_enc: XtsEncrypter<A, K>,
        xts_dec: XtsDecrypter<A, K>,
        rng: &mut R,
    ) {
        let plaintext_len_range = distributions::Uniform::new_inclusive(BLOCK_SIZE, BLOCK_SIZE * 4);
        let mut plaintext = Vec::<u8>::new();
        plaintext.extend((0..rng.sample(plaintext_len_range)).map(|_| rng.gen::<u8>()));

        let mut ciphertext = plaintext.clone();
        let tweak: Tweak = rng.gen::<u128>().into();
        xts_enc.encrypt_data_unit(tweak.clone(), &mut ciphertext).unwrap();

        assert_eq!(plaintext.len(), ciphertext.len());
        assert_ne!(plaintext, ciphertext);

        xts_dec.decrypt_data_unit(tweak, &mut ciphertext).unwrap();
        assert_eq!(plaintext, ciphertext);
    }
}
