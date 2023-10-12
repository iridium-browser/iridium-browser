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

use super::*;
use crate::extended::serialize::AddSectionError::MaxSectionCountExceeded;
use crate::{
    extended::data_elements::{ContextSyncSeqNumDataElement, GenericDataElement},
    shared_data::ContextSyncSeqNum,
};
use crypto_provider::aes::ctr::AES_CTR_NONCE_LEN;
use crypto_provider_default::CryptoProviderImpl;
use np_hkdf::v1_salt::V1Salt;
use rand::rngs::StdRng;
use rand::{prelude::SliceRandom as _, Rng as _, SeedableRng as _};
use std::{prelude::rust_2021::*, vec};
use strum::IntoEnumIterator as _;

type KeyPair = np_ed25519::KeyPair<CryptoProviderImpl>;

#[test]
fn public_identity_section_empty() {
    let mut adv_builder = AdvBuilder::new();
    let section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    assert_eq!(&[1_u8, 0x03], section_builder.into_section().as_slice());
}

#[test]
fn mic_encrypted_identity_section_empty() {
    do_mic_encrypted_identity_fixed_key_material_test::<DummyDataElement>(&[]);
}

#[test]
fn mic_encrypted_identity_section_random_des() {
    let mut rng = StdRng::from_entropy();
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    for _ in 0..1_000 {
        let num_des = rng.gen_range(1..=5);

        let extra_des = (0..num_des)
            .map(|_| {
                let de_len = rng.gen_range(0..=30);
                DummyDataElement {
                    de_type: rng.gen_range(0_u32..=u32::MAX).into(),
                    data: rand_ext::random_vec_rc(&mut rng, de_len),
                }
            })
            .collect::<Vec<_>>();

        let metadata_key = rng.gen();
        let key_seed = rng.gen();
        let identity_type =
            *EncryptedIdentityDataElementType::iter().collect::<Vec<_>>().choose(&mut rng).unwrap();
        let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);

        let mut adv_builder = AdvBuilder::new();

        let mut section_builder = adv_builder
            .section_builder(MicEncrypted::new_random_salt(
                &mut crypto_rng,
                identity_type,
                &metadata_key,
                &key_seed_hkdf,
            ))
            .unwrap();
        let section_salt =
            V1Salt::<CryptoProviderImpl>::from(*section_builder.identity.salt.as_array_ref());

        for de in extra_des.iter() {
            section_builder.add_de(|_| de).unwrap();
        }

        // length 53: 19 for encryption info, 18 for identity, 16 for MIC
        let section_length = 53
            + extra_des
                .iter()
                .map(|de| {
                    de.de_header().serialize().len() as u8 + de.de_header().len.as_usize() as u8
                })
                .sum::<u8>();

        let encryption_info = [
            &[
                0x91, 0x10, // header
                0x00, // scheme (mic)
            ],
            section_salt.as_slice(),
        ]
        .concat();

        let identity_de_header =
            DeHeader { len: 16_u8.try_into().unwrap(), de_type: identity_type.type_code() };

        let mut hmac = np_hkdf::UnsignedSectionKeys::hmac_key(&key_seed_hkdf).build_hmac();
        // just to be sure, we'll construct our test hmac all in one update() call
        let mut hmac_input = vec![];
        hmac_input.extend_from_slice(NP_SVC_UUID.as_slice());
        hmac_input.push(0b00100000); // v1

        // section header
        hmac_input.push(section_length);
        hmac_input.extend_from_slice(&encryption_info);
        let nonce = section_salt.derive::<{ AES_CTR_NONCE_LEN }>(Some(1.into())).unwrap();
        hmac_input.extend_from_slice(nonce.as_slice());

        hmac_input.extend_from_slice(identity_de_header.serialize().as_slice());

        let mut cipher = <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
            &np_hkdf::UnsignedSectionKeys::aes_key(&key_seed_hkdf),
            NonceAndCounter::from_nonce(nonce),
        );
        let mut plaintext = metadata_key.as_slice().to_vec();

        for de in extra_des {
            plaintext.extend_from_slice(de.de_header().serialize().as_slice());
            de.write_de_contents(&mut plaintext);
        }

        cipher.encrypt(&mut plaintext);
        let ciphertext = plaintext;

        hmac_input.extend_from_slice(&ciphertext);

        hmac.update(&hmac_input);
        let mic = hmac.finalize();

        let mut expected = vec![];
        expected.push(section_length);
        expected.extend_from_slice(&encryption_info);
        expected.extend_from_slice(identity_de_header.serialize().as_slice());
        expected.extend_from_slice(&ciphertext);
        expected.extend_from_slice(&mic[..16]);

        assert_eq!(&expected, section_builder.into_section().as_slice());
    }
}

#[test]
fn signature_encrypted_identity_section_empty() {
    do_signature_encrypted_identity_fixed_key_material_test::<DummyDataElement>(&[]);
}

#[test]
fn signature_encrypted_identity_section_random_des() {
    let mut rng = StdRng::from_entropy();
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    for _ in 0..1_000 {
        let num_des = rng.gen_range(1..=5);

        let metadata_key = rng.gen();
        let key_seed = rng.gen();
        let identity_type =
            *EncryptedIdentityDataElementType::iter().collect::<Vec<_>>().choose(&mut rng).unwrap();
        let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);

        let mut adv_builder = AdvBuilder::new();
        let key_pair = KeyPair::generate();

        let mut section_builder = adv_builder
            .section_builder(SignedEncrypted::new_random_salt(
                &mut crypto_rng,
                identity_type,
                &metadata_key,
                &key_pair,
                &key_seed_hkdf,
            ))
            .unwrap();
        let section_salt =
            V1Salt::<CryptoProviderImpl>::from(*section_builder.identity.salt.as_array_ref());

        let extra_des = (0..num_des)
            .map(|_| {
                let de_len = rng.gen_range(0..=30);
                DummyDataElement {
                    de_type: rng.gen_range(0_u32..=u32::MAX).into(),
                    data: rand_ext::random_vec_rc(&mut rng, de_len),
                }
            })
            // the DEs might not all fit; keep those that do
            .filter_map(|de| section_builder.add_de(|_| de.clone()).ok().map(|_| de))
            .collect::<Vec<_>>();

        // 19 for encryption info, 18 for identity, 64 for signature
        let section_length = 19
            + 18
            + 64
            + extra_des
                .iter()
                .map(|de| {
                    de.de_header().serialize().len() as u8 + de.de_header().len.as_usize() as u8
                })
                .sum::<u8>();

        let encryption_info = [
            &[
                0x91, 0x10, // header
                0x08, // scheme (signature)
            ],
            section_salt.as_slice(),
        ]
        .concat();
        let encryption_info: [u8; EncryptionInfo::TOTAL_DE_LEN] =
            encryption_info.try_into().unwrap();

        let identity_de_header =
            DeHeader { len: 16_u8.try_into().unwrap(), de_type: identity_type.type_code() };
        let identity_de_header: [u8; 2] =
            identity_de_header.serialize().as_slice().try_into().unwrap();

        let nonce = section_salt.derive::<{ AES_CTR_NONCE_LEN }>(Some(1.into())).unwrap();

        let mut plaintext = metadata_key.as_slice().to_vec();

        for de in extra_des {
            plaintext.extend_from_slice(de.de_header().serialize().as_slice());
            de.write_de_contents(&mut plaintext);
        }

        let sig_payload = SectionSignaturePayload::from_deserialized_parts(
            0x20,
            section_length,
            &encryption_info,
            &nonce,
            identity_de_header,
            &plaintext,
        );

        plaintext.extend_from_slice(&sig_payload.sign(&key_pair).to_bytes());

        <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
            &key_seed_hkdf.extended_signed_section_aes_key(),
            NonceAndCounter::from_nonce(nonce),
        )
        .encrypt(&mut plaintext);
        let ciphertext = plaintext;

        let mut expected = vec![section_length];
        expected.extend_from_slice(&encryption_info);
        expected.extend_from_slice(&identity_de_header);
        expected.extend_from_slice(&ciphertext);

        assert_eq!(&expected, section_builder.into_section().as_slice());
    }
}

#[test]
fn section_builder_too_full_doesnt_advance_de_index() {
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    let mut adv_builder = AdvBuilder::new();

    let key_seed = [22; 32];
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let metadata_key = [33; 16];
    let mut section_builder = adv_builder
        .section_builder(MicEncrypted::new_random_salt(
            &mut crypto_rng,
            EncryptedIdentityDataElementType::Trusted,
            &metadata_key,
            &key_seed_hkdf,
        ))
        .unwrap();
    let salt = V1Salt::<CryptoProviderImpl>::from(*section_builder.identity.salt.as_array_ref());

    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 100_u32.into(),
            data: de_salt.derive::<100>().unwrap().to_vec(),
        })
        .unwrap();

    // this write won't advance the de offset or the internal section buffer length
    assert_eq!(
        AddDataElementError::InsufficientSectionSpace,
        section_builder
            .add_de(|de_salt| DummyDataElement {
                de_type: 101_u32.into(),
                data: de_salt.derive::<100>().unwrap().to_vec(),
            })
            .unwrap_err()
    );

    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 102_u32.into(),
            data: de_salt.derive::<10>().unwrap().to_vec(),
        })
        .unwrap();

    section_builder.add_to_advertisement();

    let mut expected = vec![];
    // metadata key
    expected.extend_from_slice(&metadata_key);
    // de header
    expected.extend_from_slice(&[0x80 + 100, 100]);
    // section 0 de 2
    expected.extend_from_slice(&salt.derive::<100>(Some(2.into())).unwrap());
    // de header
    expected.extend_from_slice(&[0x80 + 10, 102]);
    // section 0 de 3
    expected.extend_from_slice(&salt.derive::<10>(Some(3.into())).unwrap());

    let mut cipher = <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
        &np_hkdf::UnsignedSectionKeys::aes_key(&key_seed_hkdf),
        NonceAndCounter::from_nonce(salt.derive(Some(1.into())).unwrap()),
    );

    cipher.encrypt(&mut expected);

    let adv_bytes = adv_builder.into_advertisement();
    // ignoring the MIC, etc, since that's tested elsewhere
    let ciphertext_end = adv_bytes.as_slice().len() - 16;
    assert_eq!(&expected, &adv_bytes.as_slice()[1 + 1 + 19 + 2..ciphertext_end])
}

#[test]
fn section_de_fits_exactly() {
    // leave room for initial filler section's header and the identities
    for section_contents_capacity in 1..NP_ADV_MAX_SECTION_LEN - 3 {
        let mut adv_builder = AdvBuilder::new();

        // fill up space to produce desired capacity
        let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();
        // leave space for adv header, 2 section headers, the section identities, and desired second section capacity
        fill_section_builder(
            BLE_ADV_SVC_CONTENT_LEN - 1 - 2 - 2 - section_contents_capacity,
            &mut section_builder,
        );
        section_builder.add_to_advertisement();

        let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

        // fill it almost full
        fill_section_builder(section_contents_capacity - 1, &mut section_builder);

        assert_eq!(
            1,
            section_builder.section.capacity() - section_builder.section.len(),
            "capacity: {section_contents_capacity}"
        );

        // can't add a 2 byte DE
        assert_eq!(
            Err(AddDataElementError::InsufficientSectionSpace),
            section_builder.add_de_res(|_| GenericDataElement::try_from(1_u32.into(), &[0xFF])),
            "capacity: {section_contents_capacity}"
        );

        // can add a 1 byte DE
        section_builder.add_de_res(|_| GenericDataElement::try_from(1_u32.into(), &[])).unwrap();
    }
}

#[test]
fn section_builder_build_de_error_doesnt_advance_de_index() {
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    let mut adv_builder = AdvBuilder::new();

    let key_seed = [22; 32];
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let metadata_key = [33; 16];
    let mut section_builder = adv_builder
        .section_builder(MicEncrypted::new_random_salt(
            &mut crypto_rng,
            EncryptedIdentityDataElementType::Trusted,
            &metadata_key,
            &key_seed_hkdf,
        ))
        .unwrap();
    let salt = V1Salt::<CryptoProviderImpl>::from(*section_builder.identity.salt.as_array_ref());

    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 100_u32.into(),
            data: de_salt.derive::<100>().unwrap().to_vec(),
        })
        .unwrap();

    assert_eq!(
        AddDataElementError::BuildDeError(()),
        section_builder.add_de_res(|_| Err::<DummyDataElement, _>(())).unwrap_err()
    );

    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 103_u32.into(),
            data: de_salt.derive::<10>().unwrap().to_vec(),
        })
        .unwrap();

    section_builder.add_to_advertisement();

    let mut expected = vec![];
    // metadata key
    expected.extend_from_slice(&metadata_key);
    // de header
    expected.extend_from_slice(&[0x80 + 100, 100]);
    // section 0 de 2
    expected.extend_from_slice(&salt.derive::<100>(Some(2.into())).unwrap());
    // de header
    expected.extend_from_slice(&[0x80 + 10, 103]);
    // section 0 de 3
    expected.extend_from_slice(&salt.derive::<10>(Some(3.into())).unwrap());

    let mut cipher = <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
        &np_hkdf::UnsignedSectionKeys::aes_key(&key_seed_hkdf),
        NonceAndCounter::from_nonce(salt.derive(Some(1.into())).unwrap()),
    );

    cipher.encrypt(&mut expected);

    let adv_bytes = adv_builder.into_advertisement();
    // ignoring the MIC, etc, since that's tested elsewhere
    let ciphertext_end = adv_bytes.as_slice().len() - 16;
    assert_eq!(&expected, &adv_bytes.as_slice()[1 + 1 + 19 + 2..ciphertext_end])
}

#[test]
fn add_multiple_de_correct_de_offsets_mic_encrypted_identity() {
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    let mut adv_builder = AdvBuilder::new();

    let key_seed = [22; 32];
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let metadata_key = [33; 16];
    let mut section_builder = adv_builder
        .section_builder(MicEncrypted::new_random_salt(
            &mut crypto_rng,
            EncryptedIdentityDataElementType::Trusted,
            &metadata_key,
            &key_seed_hkdf,
        ))
        .unwrap();
    let salt = V1Salt::<CryptoProviderImpl>::from(*section_builder.identity.salt.as_array_ref());

    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 64_u32.into(),
            data: de_salt.derive::<16>().unwrap().to_vec(),
        })
        .unwrap();
    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 65_u32.into(),
            data: de_salt.derive::<16>().unwrap().to_vec(),
        })
        .unwrap();

    section_builder.add_to_advertisement();

    let mut expected = vec![];
    // metadata key
    expected.extend_from_slice(&metadata_key);
    // de header
    expected.extend_from_slice(&[0x90, 0x40]);
    // section 0 de 2
    expected.extend_from_slice(&salt.derive::<16>(Some(2.into())).unwrap());
    // de header
    expected.extend_from_slice(&[0x90, 0x41]);
    // section 0 de 3
    expected.extend_from_slice(&salt.derive::<16>(Some(3.into())).unwrap());

    let mut cipher = <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
        &np_hkdf::UnsignedSectionKeys::aes_key(&key_seed_hkdf),
        NonceAndCounter::from_nonce(salt.derive(Some(1.into())).unwrap()),
    );

    cipher.encrypt(&mut expected);

    let adv_bytes = adv_builder.into_advertisement();
    // ignoring the MIC, etc, since that's tested elsewhere
    let ciphertext_end = adv_bytes.as_slice().len() - 16;
    assert_eq!(&expected, &adv_bytes.as_slice()[1 + 1 + 19 + 2..ciphertext_end])
}

#[test]
fn add_multiple_de_correct_de_offsets_signature_encrypted_identity() {
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    let mut adv_builder = AdvBuilder::new();

    let key_seed = [22; 32];
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let metadata_key = [33; 16];
    let key_pair = KeyPair::generate();
    let mut section_builder = adv_builder
        .section_builder(SignedEncrypted::new_random_salt(
            &mut crypto_rng,
            EncryptedIdentityDataElementType::Trusted,
            &metadata_key,
            &key_pair,
            &key_seed_hkdf,
        ))
        .unwrap();
    let salt = V1Salt::<CryptoProviderImpl>::from(*section_builder.identity.salt.as_array_ref());

    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 64_u32.into(),
            data: de_salt.derive::<16>().unwrap().to_vec(),
        })
        .unwrap();
    section_builder
        .add_de(|de_salt| DummyDataElement {
            de_type: 65_u32.into(),
            data: de_salt.derive::<16>().unwrap().to_vec(),
        })
        .unwrap();

    section_builder.add_to_advertisement();

    let mut expected = vec![];
    // metadata key
    expected.extend_from_slice(&metadata_key);
    // de header
    expected.extend_from_slice(&[0x90, 0x40]);
    // section 0 de 2
    expected.extend_from_slice(&salt.derive::<16>(Some(2.into())).unwrap());
    // de header
    expected.extend_from_slice(&[0x90, 0x41]);
    // section 0 de 3
    expected.extend_from_slice(&salt.derive::<16>(Some(3.into())).unwrap());

    <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
        &key_seed_hkdf.extended_signed_section_aes_key(),
        NonceAndCounter::from_nonce(salt.derive(Some(1.into())).unwrap()),
    )
    .encrypt(&mut expected);

    let adv_bytes = adv_builder.into_advertisement();
    // ignoring the signature since that's tested elsewhere
    assert_eq!(
        &expected,
        // adv header + salt + section header + encryption info + identity header
        &adv_bytes.as_slice()[1 + 1 + 19 + 2..adv_bytes.as_slice().len() - 64]
    )
}

#[test]
fn signature_encrypted_section_de_lengths_allow_room_for_suffix() {
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    let mut adv_builder = AdvBuilder::new();

    let key_seed = [22; 32];
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let metadata_key = [33; 16];
    let key_pair = KeyPair::generate();
    let mut section_builder = adv_builder
        .section_builder(SignedEncrypted::new_random_salt(
            &mut crypto_rng,
            EncryptedIdentityDataElementType::Trusted,
            &metadata_key,
            &key_pair,
            &key_seed_hkdf,
        ))
        .unwrap();

    // section header + identity + signature
    let max_total_de_len = NP_ADV_MAX_SECTION_LEN - 1 - 2 - 16 - 2 - 64;

    // take up 100 bytes to put us within 1 DE of the limit
    section_builder
        .add_de(|_| DummyDataElement { de_type: 100_u32.into(), data: vec![0; 98] })
        .unwrap();

    // one byte too many won't fit
    assert_eq!(
        AddDataElementError::InsufficientSectionSpace,
        section_builder
            .add_de(|_| DummyDataElement {
                de_type: 100_u32.into(),
                data: vec![0; max_total_de_len - 100 - 1]
            })
            .unwrap_err()
    );

    // but this will, as it allows 2 bytes for this DE's header
    assert_eq!(
        AddDataElementError::InsufficientSectionSpace,
        section_builder
            .add_de(|_| DummyDataElement {
                de_type: 100_u32.into(),
                data: vec![0; max_total_de_len - 100 - 2]
            })
            .unwrap_err()
    );
}

#[test]
fn serialize_max_number_of_sections() {
    let mut adv_builder = AdvBuilder::new();
    for _ in 0..NP_V1_ADV_MAX_SECTION_COUNT {
        let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();
        section_builder.add_de(|_| context_sync_de(1)).unwrap();
        section_builder.add_to_advertisement();
    }
    assert_eq!(
        adv_builder.section_builder(PublicIdentity::default()).unwrap_err(),
        MaxSectionCountExceeded
    );
}

fn do_mic_encrypted_identity_fixed_key_material_test<W: WriteDataElement>(extra_des: &[W]) {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let adv_header_byte = 0b00100000;
    let section_salt: V1Salt<CryptoProviderImpl> = [3; 16].into();
    let identity_type = EncryptedIdentityDataElementType::Private;
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);

    let mut adv_builder = AdvBuilder::new();

    let mut section_builder = adv_builder
        .section_builder(MicEncrypted::new(
            identity_type,
            V1Salt::from(*section_salt.as_array_ref()),
            &metadata_key,
            &key_seed_hkdf,
        ))
        .unwrap();

    for de in extra_des {
        section_builder.add_de(|_| de).unwrap();
    }

    // length 53: 19 for encryption info, 18 for identity, 16 for MIC
    let section_length = 53
        + extra_des
            .iter()
            .map(|de| de.de_header().serialize().len() as u8 + de.de_header().len.as_usize() as u8)
            .sum::<u8>();

    let encryption_info = [
        &[
            0x91, 0x10, // header
            0x00, // scheme (mic)
        ],
        section_salt.as_slice(),
    ]
    .concat();

    let identity_de_header =
        DeHeader { len: 16_u8.try_into().unwrap(), de_type: identity_type.type_code() };

    let mut hmac = np_hkdf::UnsignedSectionKeys::hmac_key(&key_seed_hkdf).build_hmac();
    // just to be sure, we'll construct our test hmac all in one update() call
    let mut hmac_input = vec![];
    hmac_input.extend_from_slice(NP_SVC_UUID.as_slice());
    hmac_input.push(adv_header_byte);
    // section header
    hmac_input.push(section_length);
    hmac_input.extend_from_slice(&encryption_info);
    let nonce = section_salt.derive::<{ AES_CTR_NONCE_LEN }>(Some(1.into())).unwrap();
    hmac_input.extend_from_slice(nonce.as_slice());

    hmac_input.extend_from_slice(identity_de_header.serialize().as_slice());

    let mut cipher = <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
        &np_hkdf::UnsignedSectionKeys::aes_key(&key_seed_hkdf),
        NonceAndCounter::from_nonce(nonce),
    );
    let mut plaintext = metadata_key.as_slice().to_vec();

    for de in extra_des {
        plaintext.extend_from_slice(de.de_header().serialize().as_slice());
        de.write_de_contents(&mut plaintext);
    }

    cipher.encrypt(&mut plaintext);
    let ciphertext = plaintext;

    hmac_input.extend_from_slice(&ciphertext);

    hmac.update(&hmac_input);
    let mic = hmac.finalize();

    let mut expected = vec![];
    expected.push(section_length);
    expected.extend_from_slice(&encryption_info);
    expected.extend_from_slice(identity_de_header.serialize().as_slice());
    expected.extend_from_slice(&ciphertext);
    expected.extend_from_slice(&mic[..16]);

    assert_eq!(&expected, section_builder.into_section().as_slice());
}

fn do_signature_encrypted_identity_fixed_key_material_test<W: WriteDataElement>(extra_des: &[W]) {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let adv_header_byte = 0b00100000;
    let section_salt: V1Salt<CryptoProviderImpl> = [3; 16].into();
    let identity_type = EncryptedIdentityDataElementType::Private;
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let key_pair = KeyPair::generate();

    let mut adv_builder = AdvBuilder::new();

    let mut section_builder = adv_builder
        .section_builder(SignedEncrypted::new(
            identity_type,
            V1Salt::from(*section_salt.as_array_ref()),
            &metadata_key,
            &key_pair,
            &key_seed_hkdf,
        ))
        .unwrap();

    for de in extra_des {
        section_builder.add_de(|_| de).unwrap();
    }

    // 19 for encryption info, 18 for identity, 64 for sig
    let section_length = 19
        + 18
        + 64
        + extra_des
            .iter()
            .map(|de| de.de_header().serialize().len() as u8 + de.de_header().len.as_usize() as u8)
            .sum::<u8>();

    let encryption_info = [
        &[
            0x91, 0x10, // header
            0x08, // scheme (signature)
        ],
        section_salt.as_slice(),
    ]
    .concat();
    let encryption_info: [u8; EncryptionInfo::TOTAL_DE_LEN] = encryption_info.try_into().unwrap();

    let identity_de_header =
        DeHeader { len: 16_u8.try_into().unwrap(), de_type: identity_type.type_code() };
    let identity_de_header: [u8; 2] = identity_de_header.serialize().as_slice().try_into().unwrap();

    let mut plaintext = metadata_key.as_slice().to_vec();

    for de in extra_des {
        plaintext.extend_from_slice(de.de_header().serialize().as_slice());
        de.write_de_contents(&mut plaintext);
    }

    let nonce = section_salt.derive(Some(1.into())).unwrap();

    let sig_payload = SectionSignaturePayload::from_deserialized_parts(
        adv_header_byte,
        section_length,
        &encryption_info,
        &nonce,
        identity_de_header,
        &plaintext,
    );

    plaintext.extend_from_slice(&sig_payload.sign(&key_pair).to_bytes());

    <CryptoProviderImpl as CryptoProvider>::AesCtr128::new(
        &key_seed_hkdf.extended_signed_section_aes_key(),
        NonceAndCounter::from_nonce(nonce),
    )
    .encrypt(&mut plaintext);
    let ciphertext = plaintext;

    let mut expected = vec![section_length];

    expected.extend_from_slice(&encryption_info);
    expected.extend_from_slice(&identity_de_header);
    expected.extend_from_slice(&ciphertext);

    assert_eq!(&expected, section_builder.into_section().as_slice());
}

/// Write `section_contents_len` bytes of DE and header into `section_builder`
pub(crate) fn fill_section_builder<I: SectionIdentity>(
    section_contents_len: usize,
    section_builder: &mut SectionBuilder<I>,
) {
    // DEs can only go up to 127, so we'll need multiple for long sections
    for _ in 0..(section_contents_len / 100) {
        let de_contents = vec![0x33; 98];
        section_builder
            .add_de_res(|_| GenericDataElement::try_from(100_u32.into(), &de_contents))
            .unwrap();
    }

    let remainder_len = section_contents_len % 100;
    match remainder_len {
        0 => {
            // leave remainder empty
        }
        1 => {
            // 1 byte header
            section_builder
                .add_de_res(|_| GenericDataElement::try_from(3_u32.into(), &[]))
                .unwrap();
        }
        2 => {
            // 2 byte header
            section_builder
                .add_de_res(|_| GenericDataElement::try_from(100_u32.into(), &[]))
                .unwrap();
        }
        _ => {
            // 2 byte header + contents as needed
            // leave room for section and DE headers
            let de_contents = vec![0x44; remainder_len - 2];
            section_builder
                .add_de_res(|_| GenericDataElement::try_from(100_u32.into(), &de_contents))
                .unwrap();
        }
    }
}

#[derive(Clone)]
pub(crate) struct DummyDataElement {
    pub(crate) de_type: DeType,
    pub(crate) data: Vec<u8>,
}

impl WriteDataElement for DummyDataElement {
    fn de_header(&self) -> DeHeader {
        DeHeader { de_type: self.de_type, len: self.data.len().try_into().unwrap() }
    }

    fn write_de_contents<S: Sink<u8>>(&self, sink: &mut S) -> Option<()> {
        sink.try_extend_from_slice(&self.data)
    }
}

fn context_sync_de(num: u8) -> ContextSyncSeqNumDataElement {
    ContextSyncSeqNumDataElement::from(ContextSyncSeqNum::try_from(num).unwrap())
}

pub(crate) trait SectionBuilderExt {
    fn into_section(self) -> EncodedSection;
}

impl<'a, I: SectionIdentity> SectionBuilderExt for SectionBuilder<'a, I> {
    /// Convenience method for tests
    fn into_section(self) -> EncodedSection {
        Self::build_section(self.section.into_inner(), self.identity, self.adv_builder)
    }
}
