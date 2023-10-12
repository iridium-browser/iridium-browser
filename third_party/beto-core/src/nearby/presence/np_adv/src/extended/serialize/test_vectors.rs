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

extern crate std;

use crate::{
    de_type::EncryptedIdentityDataElementType,
    extended::serialize::{
        section_tests::{DummyDataElement, SectionBuilderExt},
        AdvBuilder, MicEncrypted,
    },
};
use anyhow::anyhow;
use crypto_provider::{aes::ctr::AES_CTR_NONCE_LEN, aes::AesKey};
use crypto_provider_default::CryptoProviderImpl;
use np_hkdf::v1_salt;
use np_hkdf::v1_salt::V1Salt;
use rand_ext::rand::{prelude::SliceRandom as _, Rng as _, SeedableRng as _};
use serde_json::json;
use std::{fs, io::Read as _, prelude::rust_2021::*, println};
use strum::IntoEnumIterator as _;
use test_helper::extract_key_array;

#[test]
fn mic_encrypted_test_vectors() -> Result<(), anyhow::Error> {
    let full_path = test_helper::get_data_file(
        "presence/np_adv/resources/test/mic-encrypted-test-vectors.json",
    );
    let mut file = fs::File::open(full_path)?;
    let mut data = String::new();
    file.read_to_string(&mut data)?;

    let test_cases = match serde_json::de::from_str(&data)? {
        serde_json::Value::Array(a) => a,
        _ => return Err(anyhow!("bad json")),
    };

    for tc in test_cases {
        {
            let key_seed = extract_key_array::<32>(&tc, "key_seed");
            let metadata_key = extract_key_array::<16>(&tc, "metadata_key");
            let adv_header_byte = extract_key_array::<1>(&tc, "adv_header_byte")[0];
            let section_salt = v1_salt::V1Salt::<CryptoProviderImpl>::from(
                extract_key_array::<16>(&tc, "section_salt"),
            );
            let identity_type = tc["identity_type"].as_str().map(identity_type_from_label).unwrap();
            let data_elements = tc["data_elements"]
                .as_array()
                .unwrap()
                .iter()
                .map(|json_de| DummyDataElement {
                    data: hex::decode(json_de["contents"].as_str().unwrap()).unwrap(),
                    de_type: (json_de["de_type"].as_u64().unwrap() as u32).into(),
                })
                .collect::<Vec<_>>();

            let hkdf = np_hkdf::NpKeySeedHkdf::new(&key_seed);

            assert_eq!(
                extract_key_array::<16>(&tc, "aes_key").as_slice(),
                np_hkdf::UnsignedSectionKeys::aes_key(&hkdf).as_slice()
            );
            assert_eq!(
                extract_key_array::<32>(&tc, "section_mic_hmac_key").as_slice(),
                np_hkdf::UnsignedSectionKeys::hmac_key(&hkdf).as_bytes()
            );
            assert_eq!(
                extract_key_array::<{ AES_CTR_NONCE_LEN }>(&tc, "nonce").as_slice(),
                section_salt.derive::<{ AES_CTR_NONCE_LEN }>(Some(1.into())).unwrap()
            );

            // make an adv builder in the configuration we need
            let mut adv_builder = AdvBuilder::new();
            assert_eq!(adv_header_byte, adv_builder.header_byte());

            let mut section_builder = adv_builder
                .section_builder(MicEncrypted::new(
                    identity_type,
                    section_salt,
                    &metadata_key,
                    &hkdf,
                ))
                .unwrap();

            for de in data_elements {
                section_builder.add_de(|_| de).unwrap();
            }

            assert_eq!(
                hex::decode(tc["encoded_section"].as_str().unwrap()).unwrap(),
                section_builder.into_section().as_slice()
            );
        }
    }

    Ok(())
}

#[ignore]
#[test]
fn gen_mic_encrypted_test_vectors() {
    let mut rng = rand::rngs::StdRng::from_entropy();

    let mut array = Vec::<serde_json::Value>::new();

    for _ in 0..100 {
        let num_des = rng.gen_range(0..=5);

        let extra_des = (0..num_des)
            .map(|_| {
                let de_len = rng.gen_range(0..=30);
                DummyDataElement {
                    de_type: rng.gen_range(0_u32..=1_000).into(),
                    data: rand_ext::random_vec_rc(&mut rng, de_len),
                }
            })
            .collect::<Vec<_>>();

        let metadata_key = rng.gen();
        let key_seed = rng.gen();
        let adv_header_byte = 0b00100000;
        let identity_type =
            *EncryptedIdentityDataElementType::iter().collect::<Vec<_>>().choose(&mut rng).unwrap();

        let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);

        let mut adv_builder = AdvBuilder::new();

        let section_salt = v1_salt::V1Salt::<CryptoProviderImpl>::from(rng.gen::<[u8; 16]>());
        let mut section_builder = adv_builder
            .section_builder(MicEncrypted::new(
                identity_type,
                V1Salt::from(*section_salt.as_array_ref()),
                &metadata_key,
                &key_seed_hkdf,
            ))
            .unwrap();

        for de in extra_des.iter() {
            section_builder.add_de(|_| de).unwrap();
        }

        let nonce = section_salt.derive::<{ AES_CTR_NONCE_LEN }>(Some(1.into())).unwrap();

        array
            .push(json!({
                "key_seed": hex::encode_upper(key_seed),
                "metadata_key": hex::encode_upper(metadata_key),
                "adv_header_byte": hex::encode_upper([adv_header_byte]),
                "section_salt": hex::encode_upper(section_salt.as_slice()),
                "identity_type": identity_type_label(identity_type),
                "data_elements": extra_des.iter().map(|de| json!({
                    "de_type": de.de_type.as_u32(),
                    "contents": hex::encode_upper(&de.data)
                })).collect::<Vec<_>>(),
                "aes_key": hex::encode_upper(np_hkdf::UnsignedSectionKeys::aes_key(&key_seed_hkdf).as_slice()),
                "nonce": hex::encode_upper(nonce),
                "section_mic_hmac_key": hex::encode_upper(np_hkdf::UnsignedSectionKeys::hmac_key(&key_seed_hkdf).as_bytes()),
                "encoded_section": hex::encode_upper(section_builder.into_section().as_slice())
            }));
    }

    println!("{}", serde_json::ser::to_string_pretty(&array).unwrap());
}

fn identity_type_label(t: EncryptedIdentityDataElementType) -> &'static str {
    match t {
        EncryptedIdentityDataElementType::Private => "private",
        EncryptedIdentityDataElementType::Trusted => "trusted",
        EncryptedIdentityDataElementType::Provisioned => "provisioned",
    }
}

fn identity_type_from_label(label: &str) -> EncryptedIdentityDataElementType {
    match label {
        "private" => EncryptedIdentityDataElementType::Private,
        "trusted" => EncryptedIdentityDataElementType::Trusted,
        "provisioned" => EncryptedIdentityDataElementType::Provisioned,
        _ => panic!("unknown label: {}", label),
    }
}
