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

use anyhow::anyhow;
use crypto_provider_default::CryptoProviderImpl;
use ldt::{DefaultPadder, LdtDecryptCipher, LdtEncryptCipher, LdtKey, Swap, XorPadder};
use std::{fs, io::Read as _};
use test_helper::{extract_key_array, extract_key_vec};
use xts_aes::XtsAes128;

#[test]
fn aluykx_test_vectors() -> Result<(), anyhow::Error> {
    let full_path = test_helper::get_data_file(
        "presence/ldt/resources/test/aluykx-test-vectors/ldt_testvectors.json",
    );
    let mut file = fs::File::open(full_path)?;
    let mut data = String::new();
    file.read_to_string(&mut data)?;
    let test_cases = match serde_json::de::from_str(&data)? {
        serde_json::Value::Array(a) => a,
        _ => return Err(anyhow!("bad json")),
    };

    assert_eq!(320, test_cases.len());

    for tc in test_cases {
        let expected_ciphertext = extract_key_vec(&tc, "ciphertext");
        let expected_plaintext = extract_key_vec(&tc, "plaintext");
        let key: [u8; 64] = extract_key_array(&tc, "key");
        // ignoring the type -- we confirm both encryption and decryption for each test
        // case

        assert_eq!(expected_plaintext.len(), expected_ciphertext.len());
        let len = expected_plaintext.len();
        assert!(len >= crypto_provider::aes::BLOCK_SIZE);
        assert!(len < crypto_provider::aes::BLOCK_SIZE * 2);

        let ldt_enc = LdtEncryptCipher::<16, XtsAes128<CryptoProviderImpl>, Swap>::new(
            &LdtKey::from_concatenated(&key),
        );
        let ldt_dec = LdtDecryptCipher::<16, XtsAes128<CryptoProviderImpl>, Swap>::new(
            &LdtKey::from_concatenated(&key),
        );

        let mut plaintext = [0; 31];
        plaintext[..len].copy_from_slice(&expected_ciphertext);
        ldt_dec.decrypt(&mut plaintext[..len], &DefaultPadder).unwrap();
        assert_eq!(&expected_plaintext, &plaintext[..len]);

        let mut ciphertext = [0; 31];
        ciphertext[..len].copy_from_slice(&expected_plaintext);
        ldt_enc.encrypt(&mut ciphertext[..len], &DefaultPadder).unwrap();
        assert_eq!(&expected_ciphertext, &ciphertext[..len]);
    }

    Ok(())
}

#[test]
fn xor_pad_test_vectors() -> Result<(), anyhow::Error> {
    let full_path =
        test_helper::get_data_file("presence/ldt/resources/test/ldt-xor-pad-testvectors.json");
    let mut file = fs::File::open(full_path)?;
    let mut data = String::new();
    file.read_to_string(&mut data)?;
    let test_cases = match serde_json::de::from_str(&data)? {
        serde_json::Value::Array(a) => a,
        _ => return Err(anyhow!("bad json")),
    };

    assert_eq!(1000, test_cases.len());

    for tc in test_cases {
        let expected_ciphertext = extract_key_vec(&tc, "ciphertext");
        let expected_plaintext = extract_key_vec(&tc, "plaintext");
        let key: [u8; 64] = extract_key_array(&tc, "key");
        let xor_pad: [u8; crypto_provider::aes::BLOCK_SIZE] =
            extract_key_vec(&tc, "xor_pad").try_into().unwrap();
        // ignoring the type -- we confirm both encryption and decryption for each test
        // case

        assert_eq!(expected_plaintext.len(), expected_ciphertext.len());
        let len = expected_plaintext.len();
        assert!(len >= crypto_provider::aes::BLOCK_SIZE);
        assert!(len < crypto_provider::aes::BLOCK_SIZE * 2);

        let ldt_enc = LdtEncryptCipher::<16, XtsAes128<CryptoProviderImpl>, Swap>::new(
            &LdtKey::from_concatenated(&key),
        );
        let ldt_dec = LdtDecryptCipher::<16, XtsAes128<CryptoProviderImpl>, Swap>::new(
            &LdtKey::from_concatenated(&key),
        );

        let mut plaintext = [0; 31];
        plaintext[..len].copy_from_slice(&expected_ciphertext);
        ldt_dec.decrypt(&mut plaintext[..len], &XorPadder::from(xor_pad)).unwrap();
        assert_eq!(&expected_plaintext, &plaintext[..len]);

        let mut ciphertext = [0; 31];
        ciphertext[..len].copy_from_slice(&expected_plaintext);
        ldt_enc.encrypt(&mut ciphertext[..len], &XorPadder::from(xor_pad)).unwrap();
        assert_eq!(&expected_ciphertext, &ciphertext[..len]);
    }

    Ok(())
}
