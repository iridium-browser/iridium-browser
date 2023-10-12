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
use crate::extended::data_elements::TxPowerDataElement;
use crate::extended::serialize::{SingleTypeDataElement, WriteDataElement};
use crate::shared_data::TxPower;
use crate::{
    credential::{simple::SimpleV1Credential, v1::MinimumFootprintV1CryptoMaterial},
    de_type::EncryptedIdentityDataElementType,
    extended::{
        deserialize::{
            encrypted_section::MicEncryptedSectionDeserializationError,
            parse_sections,
            test_stubs::{HkdfCryptoMaterial, IntermediateSectionExt},
            CiphertextSection, OffsetDataElement, RawV1Salt,
        },
        serialize::{AdvBuilder, CapacityLimitedVec, MicEncrypted},
    },
    parse_adv_header, AdvHeader, Section,
};
use crypto_provider_default::CryptoProviderImpl;
use np_hkdf::v1_salt::DataElementOffset;
use sink::Sink;
use std::{prelude::rust_2021::*, vec};

#[test]
fn deserialize_mic_encrypted_correct_keys() {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let raw_salt = RawV1Salt([3; 16]);
    let section_salt = V1Salt::<CryptoProviderImpl>::from(raw_salt);
    let identity_type = EncryptedIdentityDataElementType::Private;
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let mut adv_builder = AdvBuilder::new();

    let mut section_builder = adv_builder
        .section_builder(MicEncrypted::new_for_testing(
            identity_type,
            V1Salt::from(*section_salt.as_array_ref()),
            &metadata_key,
            &key_seed_hkdf,
        ))
        .unwrap();

    let txpower_de = TxPowerDataElement::from(TxPower::try_from(5).unwrap());
    section_builder.add_de(|_| txpower_de.clone()).unwrap();
    section_builder.add_to_advertisement();

    let adv = adv_builder.into_advertisement();

    let (remaining, header) = parse_adv_header(adv.as_slice()).unwrap();

    let adv_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };

    let sections = parse_sections(&adv_header, remaining).unwrap();
    assert_eq!(1, sections.len());

    let section = sections.into_iter().next().unwrap();
    let enc_section = section.as_ciphertext().unwrap();
    let keypair = np_ed25519::KeyPair::<CryptoProviderImpl>::generate();

    // deserializing to Section works
    let credential = SimpleV1Credential::new(
        MinimumFootprintV1CryptoMaterial::new::<CryptoProviderImpl>(
            key_seed,
            key_seed_hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            key_seed_hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            keypair.public(),
        ),
        (),
    );
    let section = enc_section.try_deserialize::<_, CryptoProviderImpl>(&credential).unwrap();

    assert_eq!(
        DecryptedSection::new(
            EncryptedIdentityDataElementType::Private,
            VerificationMode::Mic,
            metadata_key,
            raw_salt,
            SectionContents {
                section_header: 19 // encryption info de
                    + 2 // de header
                    + 16 // metadata key
                    + 2 // de contents
                    + 16, // mic hmac tag
                // battery DE
                de_data: ArrayView::try_from_slice(&[5]).unwrap(),
                data_elements: vec![OffsetDataElement {
                    offset: 2.into(),
                    de_type: 0x05_u8.into(),
                    start_of_contents: 0,
                    contents_len: 1
                }]
            }
        ),
        section
    );

    assert_eq!(
        vec![(DataElementOffset::from(2_usize), TxPowerDataElement::DE_TYPE, vec![5_u8])],
        section
            .data_elements()
            .map(|de| (de.offset(), de.de_type(), de.contents().to_vec()))
            .collect::<Vec<_>>()
    );

    let contents = if let CiphertextSection::MicEncryptedIdentity(contents) = &enc_section {
        contents
    } else {
        panic!("incorrect flavor");
    };

    let mut encryption_info_bytes = [0_u8; 19];
    encryption_info_bytes[0..2].copy_from_slice(&[0x91, 0x10]);
    encryption_info_bytes[2] = 0x00;
    encryption_info_bytes[3..].copy_from_slice(section_salt.as_slice());

    let ciphertext_end = adv.as_slice().len() - 16;
    assert_eq!(
        &MicEncryptedSection {
            section_header: 19 // encryption info de
                    + 2 // de header
                    + 16 // metadata key
                    + 2 // de contents
                    + 16, // mic hmac tag
            adv_header: &adv_header,
            encryption_info: EncryptionInfo { bytes: encryption_info_bytes },
            identity: EncryptedIdentityMetadata {
                header_bytes: [0x90, 0x1],
                offset: 1.into(),
                identity_type: EncryptedIdentityDataElementType::Private,
            },
            all_ciphertext: &adv.as_slice()[1 + 1 + 19 + 2..ciphertext_end],
            mic: SectionMic { mic: adv.as_slice()[ciphertext_end..].try_into().unwrap() }
        },
        contents
    );

    // plaintext is correct
    {
        let crypto_material = MinimumFootprintV1CryptoMaterial::new::<CryptoProviderImpl>(
            key_seed,
            key_seed_hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            key_seed_hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            keypair.public(),
        );
        let identity_resolution_material =
            crypto_material.unsigned_identity_resolution_material::<CryptoProviderImpl>();
        let verification_material =
            crypto_material.unsigned_verification_material::<CryptoProviderImpl>();

        let decrypted = contents
            .try_decrypt::<CryptoProviderImpl>(
                &identity_resolution_material,
                &verification_material,
            )
            .unwrap();

        let mut expected = metadata_key.as_slice().to_vec();
        // battery de
        expected.extend_from_slice(txpower_de.clone().de_header().serialize().as_slice());
        txpower_de.write_de_contents(&mut expected);

        assert_eq!(&expected, decrypted.as_slice());
    }
}

#[test]
fn deserialize_mic_encrypted_incorrect_aes_key_error() {
    // bad aes key -> bad metadata key plaintext
    do_bad_deserialize_params::<CryptoProviderImpl>(
        MicEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        Some([0xFF; 16].into()),
        None,
        None,
        None,
    );
}

#[test]
fn deserialize_mic_encrypted_incorrect_metadata_key_hmac_key_error() {
    // bad metadata key hmac key -> bad calculated metadata key mac
    do_bad_deserialize_params::<CryptoProviderImpl>(
        MicEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        None,
        Some([0xFF; 32].into()),
        None,
        None,
    );
}

#[test]
fn deserialize_mic_encrypted_incorrect_mic_hmac_key_error() {
    // bad mic hmac key -> bad calculated mic
    do_bad_deserialize_params::<CryptoProviderImpl>(
        MicEncryptedSectionDeserializationError::MicMismatch,
        None,
        None,
        Some([0xFF; 32].into()),
        None,
    );
}

#[test]
fn deserialize_mic_encrypted_incorrect_expected_metadata_key_hmac_error() {
    // bad expected metadata key mac
    do_bad_deserialize_params::<CryptoProviderImpl>(
        MicEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        None,
        None,
        None,
        Some([0xFF; 32]),
    );
}

#[test]
fn deserialize_mic_encrypted_incorrect_salt_error() {
    // bad salt -> bad iv -> bad metadata key plaintext
    do_bad_deserialize_tampered(
        MicEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        |_| {},
        |adv| adv[23..39].copy_from_slice(&[0xFF; 16]),
    );
}

#[test]
fn deserialize_mic_encrypted_de_that_wont_parse() {
    // add an extra byte to the section, leading it to try to parse a DE that doesn't exist
    do_bad_deserialize_tampered(
        MicEncryptedSectionDeserializationError::DeParseError,
        |sec| sec.try_push(0xFF).unwrap(),
        |_| {},
    );
}

#[test]
fn deserialize_mic_encrypted_tampered_mic_error() {
    // flip the a bit in the first MIC byte
    do_bad_deserialize_tampered(
        MicEncryptedSectionDeserializationError::MicMismatch,
        |_| {},
        |adv| {
            let mic_start = adv.len() - 16;
            adv[mic_start] ^= 0x01
        },
    );
}

#[test]
fn deserialize_mic_encrypted_tampered_payload_error() {
    // flip the last payload bit
    do_bad_deserialize_tampered(
        MicEncryptedSectionDeserializationError::MicMismatch,
        |_| {},
        |adv| *adv.last_mut().unwrap() ^= 0x01,
    );
}

/// Attempt a decryption that will fail when using the provided parameters for decryption only.
/// `None` means use the correct value for that parameter.
fn do_bad_deserialize_params<C: CryptoProvider>(
    error: MicEncryptedSectionDeserializationError,
    aes_key: Option<crypto_provider::aes::Aes128Key>,
    metadata_key_hmac_key: Option<np_hkdf::NpHmacSha256Key<C>>,
    mic_hmac_key: Option<np_hkdf::NpHmacSha256Key<C>>,
    expected_metadata_key_hmac: Option<[u8; 32]>,
) {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let section_salt: V1Salt<C> = [3; 16].into();
    let identity_type = EncryptedIdentityDataElementType::Private;
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::new(&key_seed);

    let mut adv_builder = AdvBuilder::new();

    let mut section_builder = adv_builder
        .section_builder(MicEncrypted::new_for_testing(
            identity_type,
            section_salt,
            &metadata_key,
            &key_seed_hkdf,
        ))
        .unwrap();

    section_builder.add_de(|_| TxPowerDataElement::from(TxPower::try_from(7).unwrap())).unwrap();

    section_builder.add_to_advertisement();

    let adv = adv_builder.into_advertisement();

    let (remaining, header) = parse_adv_header(adv.as_slice()).unwrap();

    let v1_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };

    let sections = parse_sections(&v1_header, remaining).unwrap();
    assert_eq!(1, sections.len());

    let section = sections.into_iter().next().unwrap();
    let enc_section = section.as_ciphertext().unwrap();
    let contents = if let CiphertextSection::MicEncryptedIdentity(contents) = &enc_section {
        contents
    } else {
        panic!("incorrect flavor");
    };

    let unsigned_identity_resolution_material = SectionIdentityResolutionMaterial {
        aes_key: aes_key.unwrap_or_else(|| np_hkdf::UnsignedSectionKeys::aes_key(&key_seed_hkdf)),
        metadata_key_hmac_key: *metadata_key_hmac_key
            .unwrap_or_else(|| key_seed_hkdf.extended_unsigned_metadata_key_hmac_key())
            .as_bytes(),
        expected_metadata_key_hmac: expected_metadata_key_hmac.unwrap_or_else(|| {
            key_seed_hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(&metadata_key)
        }),
    };
    let identity_resolution_material =
        UnsignedSectionIdentityResolutionMaterial::from_raw(unsigned_identity_resolution_material);

    let verification_material = UnsignedSectionVerificationMaterial {
        mic_hmac_key: *mic_hmac_key
            .unwrap_or_else(|| np_hkdf::UnsignedSectionKeys::hmac_key(&key_seed_hkdf))
            .as_bytes(),
    };

    assert_eq!(
        error,
        contents
            .try_deserialize::<C>(&identity_resolution_material, &verification_material)
            .unwrap_err()
    );
}

fn do_bad_deserialize_tampered<
    S: Fn(&mut CapacityLimitedVec<u8, NP_ADV_MAX_SECTION_LEN>),
    A: Fn(&mut Vec<u8>),
>(
    expected_error: MicEncryptedSectionDeserializationError,
    mangle_section: S,
    mangle_adv: A,
) {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let section_salt: V1Salt<CryptoProviderImpl> = [3; 16].into();
    let identity_type = EncryptedIdentityDataElementType::Private;
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::new(&key_seed);

    let mut adv_builder = AdvBuilder::new();

    let mut section_builder = adv_builder
        .section_builder(MicEncrypted::new_for_testing(
            identity_type,
            section_salt,
            &metadata_key,
            &key_seed_hkdf,
        ))
        .unwrap();

    section_builder.add_de(|_| TxPowerDataElement::from(TxPower::try_from(7).unwrap())).unwrap();

    mangle_section(&mut section_builder.section);

    section_builder.add_to_advertisement();

    let adv = adv_builder.into_advertisement();
    let mut adv_mut = adv.as_slice().to_vec();
    mangle_adv(&mut adv_mut);

    let (remaining, header) = parse_adv_header(&adv_mut).unwrap();

    let v1_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };

    let sections = parse_sections(&v1_header, remaining).unwrap();
    assert_eq!(1, sections.len());

    let section = sections.into_iter().next().unwrap();
    let enc_section = section.as_ciphertext().unwrap();
    let contents = if let CiphertextSection::MicEncryptedIdentity(contents) = &enc_section {
        contents
    } else {
        panic!("incorrect flavor");
    };

    // generate a random key pair since we need _some_ public key
    let key_pair = np_ed25519::KeyPair::<CryptoProviderImpl>::generate();

    let crypto_material = HkdfCryptoMaterial::new(&key_seed, &metadata_key, key_pair.public());
    let identity_resolution_material =
        crypto_material.unsigned_identity_resolution_material::<CryptoProviderImpl>();
    let verification_material =
        crypto_material.unsigned_verification_material::<CryptoProviderImpl>();

    assert_eq!(
        expected_error,
        contents
            .try_deserialize::<CryptoProviderImpl>(
                &identity_resolution_material,
                &verification_material,
            )
            .unwrap_err()
    );
}
