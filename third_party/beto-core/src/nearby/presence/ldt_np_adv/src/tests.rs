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
#![allow(clippy::indexing_slicing, clippy::unwrap_used, clippy::panic, clippy::expect_used)]

extern crate alloc;

use crate::{
    build_np_adv_decrypter_from_key_seed, salt_padder, LdtAdvDecryptError, LdtEncrypterXtsAes128,
    LdtNpAdvDecrypterXtsAes128, LdtXtsAes128Decrypter, LegacySalt, LDT_XTS_AES_MAX_LEN,
    NP_LEGACY_METADATA_KEY_LEN,
};
use alloc::vec::Vec;
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_default::CryptoProviderImpl;
use ldt::{DefaultPadder, LdtError, LdtKey, XorPadder};
use np_hkdf::NpKeySeedHkdf;
use rand::Rng;
use rand_ext::{random_bytes, random_vec, seeded_rng};

#[test]
fn decrypt_matches_correct_ciphertext() {
    let mut rng = CryptoRng::new();
    for _ in 0..1_000 {
        let test_state = make_test_components::<CryptoProviderImpl>(&mut rng);

        let cipher = build_np_adv_decrypter_from_key_seed(&test_state.hkdf, test_state.hmac);
        let decrypted =
            cipher.decrypt_and_verify(&test_state.ciphertext, &test_state.padder).unwrap();

        assert_eq!(&test_state.plaintext, decrypted.as_ref());
    }
}

#[test]
fn decrypt_doesnt_match_when_ciphertext_mangled() {
    let mut rng = CryptoRng::new();
    for _ in 0..1_000 {
        let mut test_state = make_test_components::<CryptoProviderImpl>(&mut rng);

        // mangle the ciphertext
        test_state.ciphertext[0] ^= 0xAA;

        let cipher = build_np_adv_decrypter_from_key_seed(&test_state.hkdf, test_state.hmac);
        assert_eq!(
            Err(LdtAdvDecryptError::MacMismatch),
            cipher.decrypt_and_verify(&test_state.ciphertext, &test_state.padder)
        );
    }
}

#[test]
fn decrypt_doesnt_match_when_plaintext_doesnt_match_mac() {
    let mut rng = CryptoRng::new();
    for _ in 0..1_000 {
        let mut test_state = make_test_components::<CryptoProviderImpl>(&mut rng);

        // mangle the mac
        test_state.hmac[0] ^= 0xAA;

        let cipher = build_np_adv_decrypter_from_key_seed(&test_state.hkdf, test_state.hmac);
        assert_eq!(
            Err(LdtAdvDecryptError::MacMismatch),
            cipher.decrypt_and_verify(&test_state.ciphertext, &test_state.padder)
        );
    }
}

#[test]
#[allow(deprecated)]
fn encrypt_works() {
    let mut rng = CryptoRng::new();
    for _ in 0..1_000 {
        let test_state = make_test_components::<CryptoProviderImpl>(&mut rng);

        let cipher = test_state.ldt_enc;

        let mut plaintext_copy = test_state.plaintext.clone();
        cipher.encrypt(&mut plaintext_copy[..], &test_state.padder).unwrap();

        assert_eq!(test_state.ciphertext, plaintext_copy);
    }
}

#[test]
#[allow(deprecated)]
fn encrypt_too_short_err() {
    let ldt_enc =
        LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&LdtKey::from_concatenated(&[0; 64]));

    let mut payload = [0; 7];
    assert_eq!(Err(LdtError::InvalidLength(7)), ldt_enc.encrypt(&mut payload, &DefaultPadder));
}

#[test]
#[allow(deprecated)]
fn encrypt_too_long_err() {
    let ldt_enc =
        LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&LdtKey::from_concatenated(&[0; 64]));

    let mut payload = [0; 40];
    assert_eq!(Err(LdtError::InvalidLength(40)), ldt_enc.encrypt(&mut payload, &DefaultPadder));
}

#[test]
fn decrypt_too_short_err() {
    let adv_cipher = LdtNpAdvDecrypterXtsAes128 {
        ldt_decrypter: LdtXtsAes128Decrypter::<CryptoProviderImpl>::new(
            &LdtKey::from_concatenated(&[0; 64]),
        ),
        metadata_key_tag: [0; 32],
        metadata_key_hmac_key: np_hkdf::NpHmacSha256Key::<CryptoProviderImpl>::from([0; 32]),
    };

    let payload = [0; 7];
    assert_eq!(
        Err(LdtAdvDecryptError::InvalidLength(7)),
        adv_cipher.decrypt_and_verify(&payload, &DefaultPadder)
    );
}

#[test]
fn decrypt_too_long_err() {
    let adv_cipher = LdtNpAdvDecrypterXtsAes128 {
        ldt_decrypter: LdtXtsAes128Decrypter::<CryptoProviderImpl>::new(
            &LdtKey::from_concatenated(&[0; 64]),
        ),
        metadata_key_tag: [0; 32],
        metadata_key_hmac_key: np_hkdf::NpHmacSha256Key::<CryptoProviderImpl>::from([0; 32]),
    };

    let payload = [0; 40];
    assert_eq!(
        Err(LdtAdvDecryptError::InvalidLength(40)),
        adv_cipher.decrypt_and_verify(&payload, &DefaultPadder)
    );
}

/// Returns (plaintext, ciphertext, padder, hmac key, MAC, ldt)
fn make_test_components<C: crypto_provider::CryptoProvider>(
    rng: &mut C::CryptoRng,
) -> LdtAdvTestComponents<C> {
    // [1, 2) blocks of XTS-AES
    let mut rc_rng = seeded_rng();
    let payload_len = rc_rng
        .gen_range(crypto_provider::aes::BLOCK_SIZE..=(crypto_provider::aes::BLOCK_SIZE * 2 - 1));
    let plaintext = random_vec::<C>(rng, payload_len);

    let salt = LegacySalt { bytes: random_bytes::<2, C>(rng) };
    let padder = salt_padder::<16, C>(salt);

    let key_seed: [u8; 32] = random_bytes::<32, C>(rng);
    let hkdf = np_hkdf::NpKeySeedHkdf::new(&key_seed);
    let ldt_key = hkdf.legacy_ldt_key();
    let hmac_key = hkdf.legacy_metadata_key_hmac_key();
    let hmac: [u8; 32] = hmac_key.calculate_hmac(&plaintext[..NP_LEGACY_METADATA_KEY_LEN]);

    let ldt_enc = LdtEncrypterXtsAes128::<C>::new(&ldt_key);

    let mut ciphertext = [0_u8; LDT_XTS_AES_MAX_LEN];
    ciphertext[..plaintext.len()].copy_from_slice(&plaintext);
    ldt_enc.encrypt(&mut ciphertext[..plaintext.len()], &padder).unwrap();

    LdtAdvTestComponents {
        plaintext,
        ciphertext: ciphertext[..payload_len].to_vec(),
        padder,
        hmac,
        ldt_enc,
        hkdf,
    }
}

struct LdtAdvTestComponents<C: CryptoProvider> {
    plaintext: Vec<u8>,
    ciphertext: Vec<u8>,
    padder: XorPadder<{ crypto_provider::aes::BLOCK_SIZE }>,
    hmac: [u8; 32],
    ldt_enc: LdtEncrypterXtsAes128<C>,
    hkdf: NpKeySeedHkdf<C>,
}
