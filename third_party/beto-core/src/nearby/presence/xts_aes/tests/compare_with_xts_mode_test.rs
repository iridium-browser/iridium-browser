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

use aes::{cipher, cipher::KeyInit as _};
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
fn identical_to_xtsmode_crate() {
    // xts-mode crate exists, but is tied to RustCrypto, which we can't necessarily use with
    // whatever C/assembly we need to use on embedded platforms. Thus, we have our own, but
    // we can compare our impl's results with theirs.

    let mut rng = seeded_rng();

    for _ in 0..100_000 {
        if rng.gen() {
            let mut key = [0; 32];
            rng.fill(&mut key);
            let xts_enc = XtsEncrypter::<<CryptoProviderImpl as CryptoProvider>::Aes128, _>::new(
                &XtsAes128Key::from(&key),
            );
            let xts_dec = XtsDecrypter::<<CryptoProviderImpl as CryptoProvider>::Aes128, _>::new(
                &XtsAes128Key::from(&key),
            );

            let primary_cipher =
                aes::Aes128::new(cipher::generic_array::GenericArray::from_slice(&key[0..16]));
            let tweak_cipher =
                aes::Aes128::new(cipher::generic_array::GenericArray::from_slice(&key[16..]));
            let other_xts = xts_mode::Xts128::new(primary_cipher, tweak_cipher);

            do_roundtrip(xts_enc, xts_dec, other_xts, &mut rng)
        } else {
            let mut key = [0; 64];
            rng.fill(&mut key);
            let xts_enc = XtsEncrypter::<<CryptoProviderImpl as CryptoProvider>::Aes256, _>::new(
                &XtsAes256Key::from(&key),
            );
            let xts_dec = XtsDecrypter::<<CryptoProviderImpl as CryptoProvider>::Aes256, _>::new(
                &XtsAes256Key::from(&key),
            );

            let primary_cipher =
                aes::Aes256::new(cipher::generic_array::GenericArray::from_slice(&key[0..32]));
            let tweak_cipher =
                aes::Aes256::new(cipher::generic_array::GenericArray::from_slice(&key[32..]));
            let other_xts = xts_mode::Xts128::new(primary_cipher, tweak_cipher);

            do_roundtrip(xts_enc, xts_dec, other_xts, &mut rng)
        };
    }

    fn do_roundtrip<
        A: Aes<Key = K::BlockCipherKey>,
        K: XtsKey,
        C: cipher::BlockEncrypt + cipher::BlockDecrypt + cipher::BlockCipher,
        R: rand::Rng,
    >(
        xts_enc: XtsEncrypter<A, K>,
        xts_dec: XtsDecrypter<A, K>,
        other_xts: xts_mode::Xts128<C>,
        rng: &mut R,
    ) {
        // 1-3 blocks
        let plaintext_len_range = distributions::Uniform::new_inclusive(BLOCK_SIZE, BLOCK_SIZE * 4);
        let mut plaintext = Vec::<u8>::new();
        plaintext.extend((0..rng.sample(plaintext_len_range)).map(|_| rng.gen::<u8>()));

        // encrypt with our impl
        let mut ciphertext = plaintext.clone();
        let tweak: Tweak = rng.gen::<u128>().into();
        xts_enc.encrypt_data_unit(tweak.clone(), &mut ciphertext).unwrap();

        // encrypt with the other impl
        let mut other_ciphertext = plaintext.clone();
        let tweak_bytes = tweak.le_bytes();
        other_xts.encrypt_sector(&mut other_ciphertext[..], tweak_bytes);

        assert_eq!(ciphertext, other_ciphertext);

        // decrypt ciphertext in place
        xts_dec.decrypt_data_unit(tweak, &mut ciphertext).unwrap();
        assert_eq!(plaintext, ciphertext);

        // and with the other impl
        other_xts.decrypt_sector(&mut other_ciphertext[..], tweak_bytes);

        assert_eq!(ciphertext, other_ciphertext);
    }
}
