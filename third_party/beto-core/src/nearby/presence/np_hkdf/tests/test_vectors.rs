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
use crypto_provider::aes::AesKey;
use crypto_provider_default::CryptoProviderImpl;
use np_hkdf::{v1_salt::V1Salt, *};
use rand::Rng as _;
use rand_ext::seeded_rng;
use serde_json::json;
use std::{fs, io::Read as _};
use test_helper::extract_key_array;

#[test]
fn hkdf_test_vectors() -> Result<(), anyhow::Error> {
    let full_path =
        test_helper::get_data_file("presence/np_hkdf/resources/test/hkdf-test-vectors.json");
    let mut file = fs::File::open(full_path)?;
    let mut data = String::new();
    file.read_to_string(&mut data)?;
    let test_cases = match serde_json::de::from_str(&data)? {
        serde_json::Value::Array(a) => a,
        _ => return Err(anyhow!("bad json")),
    };

    for tc in test_cases {
        {
            let group = &tc["key_seed_hkdf"];
            let key_seed = extract_key_array::<32>(group, "key_seed");
            let hkdf = NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
            assert_eq!(
                extract_key_array::<64>(group, "legacy_ldt_key"),
                hkdf.legacy_ldt_key().as_concatenated()
            );
            assert_eq!(
                &extract_key_array::<32>(group, "legacy_metadata_key_hmac_key"),
                hkdf.legacy_metadata_key_hmac_key().as_bytes()
            );
            assert_eq!(
                extract_key_array::<12>(group, "legacy_metadata_nonce"),
                hkdf.legacy_metadata_nonce()
            );
            assert_eq!(
                extract_key_array::<12>(group, "extended_metadata_nonce"),
                hkdf.extended_metadata_nonce()
            );
            assert_eq!(
                &extract_key_array::<32>(group, "extended_unsigned_metadata_key_hmac_key"),
                hkdf.extended_unsigned_metadata_key_hmac_key().as_bytes()
            );
            assert_eq!(
                extract_key_array::<16>(group, "extended_unsigned_section_aes_key"),
                *UnsignedSectionKeys::aes_key(&hkdf).as_array()
            );
            assert_eq!(
                &extract_key_array::<32>(group, "extended_unsigned_section_mic_hmac_key"),
                UnsignedSectionKeys::hmac_key(&hkdf).as_bytes()
            );
            assert_eq!(
                &extract_key_array::<32>(group, "extended_signed_metadata_key_hmac_key"),
                hkdf.extended_signed_metadata_key_hmac_key().as_bytes()
            );
            assert_eq!(
                extract_key_array::<16>(group, "extended_signed_section_aes_key"),
                *hkdf.extended_signed_section_aes_key().as_array()
            );
        }

        {
            let group = &tc["legacy_adv_salt_hkdf"];
            let ikm = extract_key_array::<2>(group, "adv_salt");
            assert_eq!(
                extract_key_array::<16>(group, "expanded_salt"),
                legacy_ldt_expanded_salt::<16, CryptoProviderImpl>(&ikm)
            )
        }

        {
            let group = &tc["legacy_metadata_key_hkdf"];
            let ikm = extract_key_array::<14>(group, "legacy_metadata_key");
            assert_eq!(
                extract_key_array::<16>(group, "expanded_key"),
                legacy_metadata_expanded_key::<CryptoProviderImpl>(&ikm)
            )
        }

        {
            let group = &tc["extended_section_salt_hkdf"];
            let ikm = extract_key_array::<16>(group, "section_salt");
            let salt = V1Salt::<CryptoProviderImpl>::from(ikm);
            assert_eq!(
                extract_key_array::<16>(group, "derived_salt_first_section_no_de"),
                salt.derive(None).unwrap(),
            );
            assert_eq!(
                extract_key_array::<16>(group, "derived_salt_first_section_first_de"),
                salt.derive(Some(0.into())).unwrap(),
            );
            assert_eq!(
                extract_key_array::<16>(group, "derived_salt_first_section_third_de"),
                salt.derive(Some(2.into())).unwrap(),
            );
        }
    }

    Ok(())
}

// disable unless you want to print out a new set of test vectors
#[ignore]
#[test]
fn gen_test_vectors() {
    let mut rng = seeded_rng();

    let mut array = Vec::<serde_json::Value>::new();

    for _ in 0..100 {
        let key_seed: [u8; 32] = rng.gen();
        let legacy_adv_salt: [u8; 2] = rng.gen();
        let legacy_metadata_key: [u8; 14] = rng.gen();
        let adv_salt_bytes: [u8; 16] = rng.gen();
        let extended_adv_salt = V1Salt::<CryptoProviderImpl>::from(adv_salt_bytes);

        let key_seed_hkdf = NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
        array
            .push(json!({
                "key_seed_hkdf": {
                    "key_seed": hex::encode_upper(key_seed),
                    "legacy_ldt_key": hex::encode_upper(key_seed_hkdf.legacy_ldt_key().as_concatenated()),
                    "legacy_metadata_key_hmac_key":
                        hex::encode_upper(key_seed_hkdf.legacy_metadata_key_hmac_key().as_bytes()),
                    "legacy_metadata_nonce": hex::encode_upper(key_seed_hkdf.legacy_metadata_nonce()),
                    "extended_metadata_nonce": hex::encode_upper(key_seed_hkdf.extended_metadata_nonce()),
                    "extended_unsigned_metadata_key_hmac_key": hex::encode_upper(key_seed_hkdf.extended_unsigned_metadata_key_hmac_key().as_bytes()),
                    "extended_unsigned_section_aes_key": hex::encode_upper(UnsignedSectionKeys::<CryptoProviderImpl>::aes_key(&key_seed_hkdf).as_array()),
                    "extended_unsigned_section_mic_hmac_key": hex::encode_upper(UnsignedSectionKeys::<CryptoProviderImpl>::hmac_key(&key_seed_hkdf).as_bytes()),
                    "extended_signed_metadata_key_hmac_key": hex::encode_upper(key_seed_hkdf.extended_signed_metadata_key_hmac_key().as_bytes()),
                    "extended_signed_section_aes_key": hex::encode_upper(key_seed_hkdf.extended_signed_section_aes_key().as_array()),
                },
                "legacy_adv_salt_hkdf": {
                    "adv_salt": hex::encode_upper(legacy_adv_salt),
                    "expanded_salt": hex::encode_upper(legacy_ldt_expanded_salt::<16, CryptoProviderImpl>(&legacy_adv_salt))
                },
                "legacy_metadata_key_hkdf": {
                    "legacy_metadata_key": hex::encode_upper(legacy_metadata_key),
                    "expanded_key":
                        hex::encode_upper(legacy_metadata_expanded_key::<CryptoProviderImpl>(&legacy_metadata_key))
                },
                "extended_section_salt_hkdf": {
                    "section_salt": hex::encode_upper(adv_salt_bytes),
                    // 0-based offsets -> 1-based indexing
                    "derived_salt_first_section_no_de": hex::encode_upper(extended_adv_salt.derive::<16>(None).unwrap()),
                    "derived_salt_first_section_first_de": hex::encode_upper(extended_adv_salt.derive::<16>(Some(0.into())).unwrap()),
                    "derived_salt_first_section_third_de": hex::encode_upper(extended_adv_salt.derive::<16>(Some(2.into())).unwrap()),
                }
            }));
    }

    println!("{}", serde_json::ser::to_string_pretty(&array).unwrap());
}
