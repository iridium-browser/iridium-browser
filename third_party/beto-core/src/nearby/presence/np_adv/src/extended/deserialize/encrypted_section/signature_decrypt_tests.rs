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

use crate::extended::data_elements::TxPowerDataElement;
use crate::extended::deserialize::{DecryptedSection, EncryptionInfo, RawV1Salt};
use crate::shared_data::TxPower;
use crate::{
    credential::v1::*,
    de_type::EncryptedIdentityDataElementType,
    extended::{
        deserialize::{
            encrypted_section::{
                SignatureEncryptedSection, SignatureEncryptedSectionDeserializationError,
            },
            parse_sections,
            test_stubs::IntermediateSectionExt,
            CiphertextSection, EncryptedIdentityMetadata, OffsetDataElement, SectionContents,
            VerificationMode,
        },
        section_signature_payload::*,
        serialize::{
            AdvBuilder, CapacityLimitedVec, SignedEncrypted, SingleTypeDataElement,
            WriteDataElement,
        },
        NP_ADV_MAX_SECTION_LEN,
    },
    parse_adv_header, AdvHeader, Section,
};
use array_view::ArrayView;
use crypto_provider::{aes::ctr::AesCtrNonce, CryptoProvider};
use crypto_provider_default::CryptoProviderImpl;
use np_hkdf::v1_salt;
use np_hkdf::v1_salt::V1Salt;
use sink::Sink;
use std::{prelude::rust_2021::*, vec};

type KeyPair = np_ed25519::KeyPair<CryptoProviderImpl>;

#[test]
fn deserialize_signature_encrypted_correct_keys() {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let raw_salt = RawV1Salt([3; 16]);
    let section_salt = V1Salt::<CryptoProviderImpl>::from(raw_salt);
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

    let txpower_de = TxPowerDataElement::from(TxPower::try_from(7).unwrap());
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
    let contents = if let CiphertextSection::SignatureEncryptedIdentity(contents) = &enc_section {
        contents
    } else {
        panic!("incorrect flavor");
    };

    let mut encryption_info_bytes = [0_u8; 19];
    encryption_info_bytes[0..2].copy_from_slice(&[0x91, 0x10]);
    encryption_info_bytes[2] = 0x08;
    encryption_info_bytes[3..].copy_from_slice(section_salt.as_slice());

    let section_len = 19 + 2 + 16 + 2 + 64;
    assert_eq!(
        &SignatureEncryptedSection {
            section_header: section_len,
            adv_header: &adv_header,
            encryption_info: EncryptionInfo { bytes: encryption_info_bytes },
            identity: EncryptedIdentityMetadata {
                header_bytes: [0x90, 0x1],
                offset: 1.into(),
                identity_type: EncryptedIdentityDataElementType::Private,
            },
            // adv header + salt + section header + encryption info + identity header
            all_ciphertext: &adv.as_slice()[1 + 1 + 19 + 2..],
        },
        contents
    );

    // plaintext is correct
    {
        let crypto_material = MinimumFootprintV1CryptoMaterial::new::<CryptoProviderImpl>(
            key_seed,
            key_seed_hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            key_seed_hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            key_pair.public(),
        );
        let signed_identity_resolution_material =
            crypto_material.signed_identity_resolution_material::<CryptoProviderImpl>();
        let decrypted = contents
            .try_decrypt::<CryptoProviderImpl>(&signed_identity_resolution_material)
            .unwrap();

        let mut expected = metadata_key.as_slice().to_vec();
        // battery de
        expected.extend_from_slice(txpower_de.de_header().serialize().as_slice());
        txpower_de.write_de_contents(&mut expected);

        let nonce: AesCtrNonce = section_salt.derive(Some(1.into())).unwrap();

        let mut encryption_info = vec![0x91, 0x10, 0x08];
        encryption_info.extend_from_slice(section_salt.as_slice());
        let encryption_info: [u8; EncryptionInfo::TOTAL_DE_LEN] =
            encryption_info.try_into().unwrap();

        let sig_payload = SectionSignaturePayload::from_deserialized_parts(
            0x20,
            section_len,
            &encryption_info,
            &nonce,
            [0x90, 0x1],
            &expected,
        );

        expected.extend_from_slice(&sig_payload.sign(&key_pair).to_bytes());

        assert_eq!(&expected, decrypted.as_slice());
    }

    // deserialization to Section works
    {
        let crypto_material = MinimumFootprintV1CryptoMaterial::new::<CryptoProviderImpl>(
            key_seed,
            key_seed_hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            key_seed_hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            key_pair.public(),
        );
        let signed_identity_resolution_material =
            crypto_material.signed_identity_resolution_material::<CryptoProviderImpl>();
        let signed_verification_material =
            crypto_material.signed_verification_material::<CryptoProviderImpl>();

        let section = contents
            .try_deserialize::<CryptoProviderImpl>(
                &signed_identity_resolution_material,
                &signed_verification_material,
            )
            .unwrap();

        assert_eq!(
            DecryptedSection::new(
                EncryptedIdentityDataElementType::Private,
                VerificationMode::Signature,
                metadata_key,
                raw_salt,
                SectionContents {
                    section_header: 19 + 2 + 16 + 1 + 1 + 64,
                    // battery DE
                    de_data: ArrayView::try_from_slice(&[7]).unwrap(),
                    data_elements: vec![OffsetDataElement {
                        offset: 2.into(),
                        de_type: 0x05_u8.into(),
                        start_of_contents: 0,
                        contents_len: 1,
                    }],
                },
            ),
            section
        );

        assert_eq!(
            vec![(
                v1_salt::DataElementOffset::from(2_usize),
                TxPowerDataElement::DE_TYPE,
                vec![7u8]
            )],
            section
                .data_elements()
                .map(|de| (de.offset(), de.de_type(), de.contents().to_vec()))
                .collect::<Vec<_>>()
        );
    }
}

#[test]
fn deserialize_signature_encrypted_incorrect_aes_key_error() {
    // bad aes key -> bad metadata key plaintext
    do_bad_deserialize_params::<CryptoProviderImpl>(
        SignatureEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        Some([0xFF; 16].into()),
        None,
        None,
        None,
    );
}

#[test]
fn deserialize_signature_encrypted_incorrect_metadata_key_hmac_key_error() {
    // bad metadata key hmac key -> bad calculated metadata key mac
    do_bad_deserialize_params::<CryptoProviderImpl>(
        SignatureEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        None,
        Some([0xFF; 32].into()),
        None,
        None,
    );
}

#[test]
fn deserialize_signature_encrypted_incorrect_expected_metadata_key_hmac_error() {
    // bad expected metadata key mac
    do_bad_deserialize_params::<CryptoProviderImpl>(
        SignatureEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        None,
        None,
        Some([0xFF; 32]),
        None,
    );
}

#[test]
fn deserialize_signature_encrypted_incorrect_pub_key_error() {
    // a random pub key will lead to signature mismatch
    do_bad_deserialize_params::<CryptoProviderImpl>(
        SignatureEncryptedSectionDeserializationError::SignatureMismatch,
        None,
        None,
        None,
        Some(KeyPair::generate().public()),
    );
}

#[test]
fn deserialize_signature_encrypted_incorrect_salt_error() {
    // bad salt -> bad iv -> bad metadata key plaintext
    do_bad_deserialize_tampered(
        SignatureEncryptedSectionDeserializationError::MetadataKeyMacMismatch,
        None,
        |_| {},
        |adv_mut| adv_mut[5..21].copy_from_slice(&[0xFF; 16]),
    )
}

#[test]
fn deserialize_signature_encrypted_tampered_signature_error() {
    do_bad_deserialize_tampered(
        SignatureEncryptedSectionDeserializationError::SignatureMismatch,
        None,
        |_| {},
        // flip a bit in the middle of the signature
        |adv_mut| {
            let len = adv_mut.len();
            adv_mut[len - 30] ^= 0x1
        },
    )
}

#[test]
fn deserialize_signature_encrypted_tampered_ciphertext_error() {
    do_bad_deserialize_tampered(
        SignatureEncryptedSectionDeserializationError::SignatureMismatch,
        None,
        |_| {},
        // flip a bit outside of the signature
        |adv_mut| {
            let len = adv_mut.len();
            adv_mut[len - 1 - 64] ^= 0x1
        },
    )
}

#[test]
fn deserialize_signature_encrypted_missing_signature_de_error() {
    let section_len = 19 + 2 + 16 + 1 + 1;
    do_bad_deserialize_tampered(
        SignatureEncryptedSectionDeserializationError::SignatureMissing,
        Some(section_len),
        |_| {},
        |adv_mut| {
            // chop off signature DE
            adv_mut.truncate(adv_mut.len() - 64);
            // fix section length
            adv_mut[1] = section_len;
        },
    )
}

#[test]
fn deserialize_signature_encrypted_des_wont_parse() {
    do_bad_deserialize_tampered(
        SignatureEncryptedSectionDeserializationError::DeParseError,
        Some(19 + 2 + 16 + 1 + 1 + 64 + 1),
        // add an impossible DE
        |section| section.try_push(0xFF).unwrap(),
        |_| {},
    )
}

/// Attempt a deserialization that will fail when using the provided parameters for decryption only.
/// `None` means use the correct value for that parameter.
fn do_bad_deserialize_params<C: CryptoProvider>(
    error: SignatureEncryptedSectionDeserializationError,
    aes_key: Option<crypto_provider::aes::Aes128Key>,
    metadata_key_hmac_key: Option<np_hkdf::NpHmacSha256Key<C>>,
    expected_metadata_key_hmac: Option<[u8; 32]>,
    pub_key: Option<np_ed25519::PublicKey<C>>,
) {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let section_salt: v1_salt::V1Salt<C> = [3; 16].into();
    let identity_type = EncryptedIdentityDataElementType::Private;
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::new(&key_seed);
    let key_pair = np_ed25519::KeyPair::<C>::generate();

    let mut adv_builder = AdvBuilder::new();

    let mut section_builder = adv_builder
        .section_builder(SignedEncrypted::new(
            identity_type,
            section_salt,
            &metadata_key,
            &key_pair,
            &key_seed_hkdf,
        ))
        .unwrap();

    section_builder.add_de_res(|_| TxPower::try_from(2).map(TxPowerDataElement::from)).unwrap();

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
    let contents = if let CiphertextSection::SignatureEncryptedIdentity(contents) = &enc_section {
        contents
    } else {
        panic!("incorrect flavor");
    };

    let signed_identity_resolution_material =
        SignedSectionIdentityResolutionMaterial::from_raw(SectionIdentityResolutionMaterial {
            aes_key: aes_key.unwrap_or_else(|| key_seed_hkdf.extended_signed_section_aes_key()),

            metadata_key_hmac_key: *metadata_key_hmac_key
                .unwrap_or_else(|| key_seed_hkdf.extended_signed_metadata_key_hmac_key())
                .as_bytes(),
            expected_metadata_key_hmac: expected_metadata_key_hmac.unwrap_or_else(|| {
                key_seed_hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key)
            }),
        });

    let signed_verification_material = SignedSectionVerificationMaterial {
        pub_key: pub_key.unwrap_or_else(|| key_pair.public()).to_bytes(),
    };

    assert_eq!(
        error,
        contents
            .try_deserialize::<C>(
                &signed_identity_resolution_material,
                &signed_verification_material,
            )
            .unwrap_err()
    );
}

/// Run a test that mangles the advertisement contents before attempting to deserialize.
///
/// Since the advertisement is ciphertext, only changes outside
fn do_bad_deserialize_tampered<
    A: Fn(&mut Vec<u8>),
    S: Fn(&mut CapacityLimitedVec<u8, NP_ADV_MAX_SECTION_LEN>),
>(
    expected_error: SignatureEncryptedSectionDeserializationError,
    expected_section_len: Option<u8>,
    mangle_section: S,
    mangle_adv_contents: A,
) {
    let metadata_key = [1; 16];
    let key_seed = [2; 32];
    let section_salt: v1_salt::V1Salt<CryptoProviderImpl> = [3; 16].into();
    let identity_type = EncryptedIdentityDataElementType::Private;
    let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::new(&key_seed);
    let key_pair = KeyPair::generate();

    let mut adv_builder = AdvBuilder::new();

    let mut section_builder = adv_builder
        .section_builder(SignedEncrypted::new(
            identity_type,
            section_salt,
            &metadata_key,
            &key_pair,
            &key_seed_hkdf,
        ))
        .unwrap();

    section_builder.add_de_res(|_| TxPower::try_from(2).map(TxPowerDataElement::from)).unwrap();

    mangle_section(&mut section_builder.section);

    section_builder.add_to_advertisement();

    let adv = adv_builder.into_advertisement();
    let mut adv_mut = adv.as_slice().to_vec();
    mangle_adv_contents(&mut adv_mut);

    let (remaining, header) = parse_adv_header(&adv_mut).unwrap();

    let adv_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };

    let sections = parse_sections(&adv_header, remaining).unwrap();
    assert_eq!(1, sections.len());

    let section = sections.into_iter().next().unwrap();
    let enc_section = section.as_ciphertext().unwrap();
    let contents = if let CiphertextSection::SignatureEncryptedIdentity(contents) = &enc_section {
        contents
    } else {
        panic!("incorrect flavor");
    };

    let mut encryption_info_bytes = [0_u8; 19];
    encryption_info_bytes[0..2].copy_from_slice(&[0x91, 0x10]);
    encryption_info_bytes[2] = 0x08;
    encryption_info_bytes[3..].copy_from_slice(&adv_mut[5..21]);

    let section_len = 19 + 2 + 16 + 2 + 64;
    assert_eq!(
        &SignatureEncryptedSection {
            section_header: expected_section_len.unwrap_or(section_len),
            adv_header: &adv_header,
            encryption_info: EncryptionInfo { bytes: encryption_info_bytes },
            identity: EncryptedIdentityMetadata {
                header_bytes: [0x90, 0x1],
                offset: 1.into(),
                identity_type: EncryptedIdentityDataElementType::Private,
            },
            all_ciphertext: &adv_mut[1 + 1 + 19 + 2..],
        },
        contents
    );

    let crypto_material = MinimumFootprintV1CryptoMaterial::new::<CryptoProviderImpl>(
        key_seed,
        key_seed_hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(&metadata_key),
        key_seed_hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key),
        key_pair.public(),
    );
    let identity_resolution_material =
        crypto_material.signed_identity_resolution_material::<CryptoProviderImpl>();
    let verification_material =
        crypto_material.signed_verification_material::<CryptoProviderImpl>();

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
