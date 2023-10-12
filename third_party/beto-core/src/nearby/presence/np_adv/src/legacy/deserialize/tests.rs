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
use crate::legacy::actions::{ActionBits, ActionsDataElement};
use crate::shared_data::TxPower;
use crate::{
    de_type::IdentityDataElementType,
    legacy::{
        actions,
        de_type::DeActualLength,
        random_data_elements::{random_de_ciphertext, random_de_plaintext},
        serialize::{
            encode_de_header_actual_len, id_de_type_as_generic_de_type, AdvBuilder,
            DataElementBundle, Identity, LdtIdentity, ToDataElementBundle as _,
        },
        PacketFlavorEnum, BLE_ADV_SVC_CONTENT_LEN,
    },
    parse_adv_header, shared_data, AdvHeader, NoIdentity, PublicIdentity,
};
use array_view::ArrayView;
use crypto_provider_default::CryptoProviderImpl;
use init_with::InitWith as _;
use ldt_np_adv::LdtEncrypterXtsAes128;
use nom::error;
use rand_ext::rand::{prelude::SliceRandom, Rng as _};
use std::vec;
use strum::IntoEnumIterator as _;

#[test]
fn parse_empty_raw_adv() {
    let data_elements = parse_raw_adv_contents::<CryptoProviderImpl>(&[]).unwrap();
    assert_eq!(
        RawAdvertisement::Plaintext(PlaintextAdvRawContents {
            identity_type: PlaintextIdentityMode::None,
            data_elements: Vec::new()
        }),
        data_elements
    );
}

#[test]
fn parse_raw_adv_1_de_short_no_identity() {
    // battery uses the header length as is
    let adv = parse_raw_adv_contents::<CryptoProviderImpl>(&[0x36, 0x01, 0x02, 0x03]).unwrap();
    assert_eq!(
        RawAdvertisement::Plaintext(PlaintextAdvRawContents {
            identity_type: PlaintextIdentityMode::None,
            data_elements: vec![RawPlainDataElement {
                de_type: PlainDataElementType::Actions,
                contents: &[0x01, 0x02, 0x03]
            }],
        }),
        adv
    );
}

#[test]
fn parse_raw_adv_1_de_long_private_identity() {
    // 3 bytes of payload after metadata key
    let adv = parse_raw_adv_contents::<CryptoProviderImpl>(&[
        0x31, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
        0x0F, 0x10, 0x11, 0x12, 0x13,
    ])
    .unwrap();
    assert_eq!(
        RawAdvertisement::Ciphertext(EncryptedAdvContents {
            identity_type: EncryptedIdentityDataElementType::Private,
            salt_padder: ldt_np_adv::salt_padder::<16, CryptoProviderImpl>(
                ldt_np_adv::LegacySalt::from([0x01, 0x02])
            ),
            salt: ldt_np_adv::LegacySalt::from([0x01, 0x02]),
            ciphertext: &[
                0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                0x11, 0x12, 0x13
            ]
        }),
        adv
    );
}

#[test]
fn parse_raw_adv_3_de_public_identity() {
    let adv = parse_raw_adv_contents::<CryptoProviderImpl>(&[
        0x03, // public identity
        0x15, 0x05, // tx power 5
        0x36, 0x11, 0x12, 0x13, // actions
    ])
    .unwrap();
    assert_eq!(
        RawAdvertisement::Plaintext(PlaintextAdvRawContents {
            identity_type: PlaintextIdentityMode::Public,
            data_elements: vec![
                RawPlainDataElement { de_type: PlainDataElementType::TxPower, contents: &[0x05] },
                RawPlainDataElement {
                    de_type: PlainDataElementType::Actions,
                    contents: &[0x11, 0x12, 0x13]
                }
            ],
        }),
        adv
    );
}

#[test]
fn parse_raw_adv_0_de_public_identity() {
    let adv = parse_raw_adv_contents::<CryptoProviderImpl>(&[
        0x03, // public identity
    ])
    .unwrap();
    assert_eq!(
        RawAdvertisement::Plaintext(PlaintextAdvRawContents {
            identity_type: PlaintextIdentityMode::Public,
            data_elements: vec![],
        }),
        adv
    );
}

#[test]
fn parse_raw_adv_1_de_length_overrun() {
    // battery uses the header length as is
    let input = &[0xFB, 0x01, 0x02, 0x03];
    assert_eq!(
        nom::Err::Error(error::Error { input: input.as_slice(), code: error::ErrorKind::Eof }),
        parse_data_elements(input).unwrap_err()
    );
}

#[test]
fn parse_raw_adv_public_identity_containing_public_identity() {
    let input = &[
        0x03, // public identity
        0x03, // another public identity
        0x15, 0x03, // tx power de
    ];
    assert_eq!(
        AdvDeserializeError::InvalidDataElementHierarchy,
        parse_raw_adv_contents::<CryptoProviderImpl>(input).unwrap_err()
    );
}

#[test]
fn parse_raw_adv_no_identity_containing_public_identity() {
    let input = &[
        0x15, 0x03, // tx power de
        0x03, // public identity -- since it's not the first, this is a no identity adv
        0x15, 0x03, // tx power de
    ];
    assert_eq!(
        AdvDeserializeError::InvalidDataElementHierarchy,
        parse_raw_adv_contents::<CryptoProviderImpl>(input).unwrap_err()
    );
}

#[test]
fn parse_raw_adv_1_de_long_private_identity_with_another_top_level_de() {
    let input = &[
        0x31, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
        0x0F, 0x10, 0x11, 0x12, 0x13, // private identity
        0x15, 0x03, // tx power de
    ];
    assert_eq!(
        AdvDeserializeError::TooManyTopLevelDataElements,
        parse_raw_adv_contents::<CryptoProviderImpl>(input).unwrap_err()
    );
}

#[test]
fn parse_raw_adv_private_identity_ciphertext_min_length() {
    let short_input = &[
        // private identity w/ salt, len = 1
        0x11, 0x10, 0x11, // 15 ciphertext bytes, + 2 salt = 17 total
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
    ];
    assert_eq!(
        AdvDeserializeError::AdvertisementDeserializeError,
        parse_raw_adv_contents::<CryptoProviderImpl>(short_input).unwrap_err()
    );

    let ok_input = &[
        // private identity w/ salt
        0x21, 0x10, 0x11, // 16 ciphertext bytes, 18 bytes total de len, encoded len 2
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
        0x2F,
    ];
    assert_eq!(
        RawAdvertisement::Ciphertext(EncryptedAdvContents {
            identity_type: EncryptedIdentityDataElementType::Private,
            salt: [0x10, 0x11].into(),
            salt_padder: ldt_np_adv::salt_padder::<16, CryptoProviderImpl>([0x10, 0x11].into()),
            ciphertext: &ok_input[3..]
        }),
        parse_raw_adv_contents::<CryptoProviderImpl>(ok_input).unwrap()
    );
}

#[test]
fn parse_raw_adv_private_identity_ciphertext_too_long() {
    let long_input = &[
        // private identity w/ salt, len = 7
        0x71, 0x10, 0x11, // 21 ciphertext bytes, + 2 salt
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
        0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
    ];
    assert_eq!(
        AdvDeserializeError::AdvertisementDeserializeError,
        parse_raw_adv_contents::<CryptoProviderImpl>(long_input).unwrap_err()
    );

    // removing 1 byte makes it work
    let ok_input = &[
        // private identity w/ salt, len = 6
        0x61, 0x10, 0x11, // 20 ciphertext bytes, + 2 salt
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
        0x2F, 0x30, 0x31, 0x32, 0x33,
    ];
    assert_eq!(
        RawAdvertisement::Ciphertext(EncryptedAdvContents {
            identity_type: EncryptedIdentityDataElementType::Private,
            salt_padder: ldt_np_adv::salt_padder::<16, CryptoProviderImpl>([0x10, 0x11].into()),
            salt: [0x10, 0x11].into(),
            ciphertext: &ok_input[3..]
        }),
        parse_raw_adv_contents::<CryptoProviderImpl>(ok_input).unwrap()
    );
}

/// Test method body that creates an array, deserializes it into a DE, serializes it,
/// and asserts that the same bytes are produced.
///
/// Evaluates to the deserialized DE.
macro_rules! de_roundtrip_test {
    ($de_type:ty, $type_variant:ident, $de_variant:ident, $flavor:ty,  $bytes:expr) => {{
        let parsed_de_enum = parse_plain_de::<$flavor>(PlainDataElementType::$type_variant, $bytes);
        if let PlainDataElement::$de_variant(de) = parsed_de_enum {
            let expected = <$de_type>::deserialize::<$flavor>($bytes).unwrap();
            assert_eq!(expected, de);

            let de_bundle: DataElementBundle<$flavor> = de.to_de_bundle();
            assert_eq!($bytes, de_bundle.contents_as_slice());

            de
        } else {
            panic!("Unexpected variant: {:?}", parsed_de_enum);
        }
    }};
}

#[test]
fn actions_de_contents_roundtrip_plaintext() {
    let actions = actions::tests::all_plaintext_actions();
    let bundle = actions::ActionsDataElement::from(actions).to_de_bundle();

    de_roundtrip_test!(
        actions::ActionsDataElement::<Plaintext>,
        Actions,
        Actions,
        Plaintext,
        bundle.contents_as_slice()
    );
}

#[test]
fn actions_de_contents_roundtrip_ciphertext() {
    let actions = actions::tests::all_ciphertext_actions();
    let bundle = actions::ActionsDataElement::from(actions).to_de_bundle();

    de_roundtrip_test!(
        actions::ActionsDataElement::<Ciphertext>,
        Actions,
        Actions,
        Ciphertext,
        bundle.contents_as_slice()
    );
}

#[test]
fn tx_power_de_contents_roundtrip_plaintext() {
    let tx = shared_data::TxPower::try_from(-10).unwrap();
    let bundle: DataElementBundle<Plaintext> = TxPowerDataElement::from(tx).to_de_bundle();

    de_roundtrip_test!(TxPowerDataElement, TxPower, TxPower, Plaintext, bundle.contents_as_slice());
}

#[test]
fn tx_power_de_contents_roundtrip_ciphertext() {
    let tx = shared_data::TxPower::try_from(-10).unwrap();
    let bundle: DataElementBundle<Ciphertext> = TxPowerDataElement::from(tx).to_de_bundle();

    de_roundtrip_test!(
        TxPowerDataElement,
        TxPower,
        TxPower,
        Ciphertext,
        bundle.contents_as_slice()
    );
}

#[test]
fn parse_de_invalid_de_len_error() {
    let input = &[
        // bogus 6-byte battery de -- only allows length = 3
        0x6B, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    ];

    assert_eq!(
        nom::Err::Error(error::Error { input: input.as_slice(), code: error::ErrorKind::MapOpt }),
        parse_de(&input[..]).unwrap_err()
    );
}

#[test]
fn plain_data_elements_matches_plain_des() {
    assert_eq!(
        vec![
            RawPlainDataElement { de_type: PlainDataElementType::TxPower, contents: &[0x01] },
            RawPlainDataElement { de_type: PlainDataElementType::Actions, contents: &[0x02] }
        ],
        plain_data_elements(&[
            RawDataElement { de_type: DataElementType::TxPower, contents: &[0x01] },
            RawDataElement { de_type: DataElementType::Actions, contents: &[0x02] }
        ])
        .unwrap()
    );
}

#[test]
fn plain_data_elements_rejects_identity_de_error() {
    for idet in IdentityDataElementType::iter() {
        assert_eq!(
            AdvDeserializeError::InvalidDataElementHierarchy,
            plain_data_elements(&[
                RawDataElement { de_type: DataElementType::TxPower, contents: &[0x01] },
                RawDataElement { de_type: id_de_type_as_generic_de_type(idet), contents: &[0x02] }
            ])
            .unwrap_err()
        );
    }
}

#[test]
fn parse_encrypted_identity_contents_too_short_error() {
    // 2 byte salt + 15 byte ciphertext: 1 too short
    let input = <[u8; 17]>::init_with_indices(|i| i as u8);
    assert_eq!(
        nom::Err::Error(error::Error { input: &input[2..], code: error::ErrorKind::TakeWhileMN }),
        parse_encrypted_identity_de_contents(&input).unwrap_err()
    );
}

#[test]
fn parse_encrypted_identity_contents_ok() {
    // 2 byte salt + minimum 16 byte ciphertext
    let input = <[u8; 18]>::init_with_indices(|i| i as u8);
    assert_eq!(
        ([].as_slice(), (ldt_np_adv::LegacySalt::from([0, 1]), &input[2..])),
        parse_encrypted_identity_de_contents(&input).unwrap()
    );
}

#[test]
fn plaintext_random_adv_contents_round_trip_public() {
    plaintext_random_adv_contents_round_trip(PublicIdentity::default, PlaintextIdentityMode::Public)
}

#[test]
fn plaintext_random_adv_contents_round_trip_no_identity() {
    plaintext_random_adv_contents_round_trip(NoIdentity::default, PlaintextIdentityMode::None)
}

#[test]
fn ciphertext_random_adv_contents_round_trip() {
    let mut rng = rand_ext::seeded_rng();
    let de_types: Vec<PlainDataElementType> = PlainDataElementType::iter()
        .filter(|t| t.supports_flavor(PacketFlavorEnum::Ciphertext))
        .collect();
    let identity_de_types: Vec<EncryptedIdentityDataElementType> =
        EncryptedIdentityDataElementType::iter().collect();

    for _ in 0..10_000 {
        let mut des = Vec::new();

        let identity_type = *identity_de_types.choose(&mut rng).unwrap();
        let key_seed: [u8; 32] = rng.gen();
        let salt: ldt_np_adv::LegacySalt = rng.gen::<[u8; 2]>().into();
        let metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN] = rng.gen();
        let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
        let ldt_key = hkdf.legacy_ldt_key();
        let metadata_key_hmac: [u8; 32] =
            hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&metadata_key);
        let cipher = ldt_np_adv::build_np_adv_decrypter_from_key_seed(&hkdf, metadata_key_hmac);
        let mut builder = AdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
            identity_type,
            salt,
            metadata_key,
            LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&ldt_key),
        ));

        loop {
            let de_type = *de_types.choose(&mut rng).unwrap();
            let (de, bundle) = random_de_ciphertext(de_type, &mut rng).unwrap();

            if builder.add_data_element(bundle.clone()).ok().is_none() {
                // out of room
                if des.is_empty() {
                    // need at least one, so try again
                    continue;
                } else {
                    // there's at least one so proceed to serialization
                    break;
                }
            }

            des.push(de);
        }

        let serialized = builder.into_advertisement().unwrap();
        let (remaining, header) = parse_adv_header(serialized.as_slice()).unwrap();
        let parsed_adv = deserialize_adv_contents::<CryptoProviderImpl>(remaining).unwrap();

        assert_eq!(AdvHeader::V0, header);
        if let IntermediateAdvContents::Ciphertext(eac) = parsed_adv {
            assert_eq!(
                EncryptedAdvContents {
                    identity_type,
                    salt_padder: ldt_np_adv::salt_padder::<16, CryptoProviderImpl>(salt),
                    salt,
                    // skip adv header, de header, salt
                    ciphertext: &serialized.as_slice()[4..]
                },
                eac
            );

            assert_eq!(
                DecryptedAdvContents { identity_type, metadata_key, salt, data_elements: des },
                eac.try_decrypt(&cipher).unwrap()
            )
        } else {
            panic!("Unexpected variant: {:?}", parsed_adv);
        }
    }
}

#[test]
fn decrypt_and_deserialize_ciphertext_adv_canned() {
    let key_seed = [0x11_u8; 32];
    let salt: ldt_np_adv::LegacySalt = [0x22; 2].into();
    let metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN] = [0x33; NP_LEGACY_METADATA_KEY_LEN];

    let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let ldt_key = hkdf.legacy_ldt_key();
    let metadata_key_hmac: [u8; 32] =
        hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&metadata_key);
    let cipher = ldt_np_adv::build_np_adv_decrypter_from_key_seed(&hkdf, metadata_key_hmac);
    let mut builder = AdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
        EncryptedIdentityDataElementType::Private,
        salt,
        metadata_key,
        LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&ldt_key),
    ));

    let tx = shared_data::TxPower::try_from(3).unwrap();
    builder.add_data_element(TxPowerDataElement::from(tx)).unwrap();

    let serialized = builder.into_advertisement().unwrap();

    assert_eq!(
        &[
            0x0,  // adv header
            0x21, // private DE
            0x22, 0x22, // salt
            // ciphertext
            0x85, 0xBF, 0xA8, 0x83, 0x58, 0x7C, 0x50, 0xCF, 0x98, 0x38, 0xA7, 0x8A, 0xC0, 0x1C,
            0x96, 0xF9
        ],
        serialized.as_slice()
    );

    let (remaining, header) = parse_adv_header(serialized.as_slice()).unwrap();
    assert_eq!(AdvHeader::V0, header);

    let parsed_adv = deserialize_adv_contents::<CryptoProviderImpl>(remaining).unwrap();
    if let IntermediateAdvContents::Ciphertext(eac) = parsed_adv {
        assert_eq!(
            EncryptedAdvContents {
                identity_type: EncryptedIdentityDataElementType::Private,
                salt_padder: ldt_np_adv::salt_padder::<16, CryptoProviderImpl>(salt),
                salt,
                // skip adv header, de header, salt
                ciphertext: &serialized.as_slice()[4..]
            },
            eac
        );

        assert_eq!(
            DecryptedAdvContents {
                identity_type: EncryptedIdentityDataElementType::Private,
                metadata_key,
                salt,
                data_elements: vec![PlainDataElement::TxPower(TxPowerDataElement::from(
                    TxPower::try_from(3).unwrap()
                ))],
            },
            eac.try_decrypt(&cipher).unwrap()
        )
    } else {
        panic!("Unexpected variant: {:?}", parsed_adv);
    }
}

#[test]
fn decrypt_and_deserialize_plaintext_adv_canned() {
    let mut builder = AdvBuilder::new(PublicIdentity::default());

    let actions = ActionBits::default();
    builder.add_data_element(ActionsDataElement::from(actions)).unwrap();

    let serialized = builder.into_advertisement().unwrap();

    assert_eq!(
        &[
            0x0,  // adv header
            0x03, // public DE
            0x16, 0x00 // actions
        ],
        serialized.as_slice()
    );

    let (remaining, header) = parse_adv_header(serialized.as_slice()).unwrap();
    assert_eq!(AdvHeader::V0, header);

    let parsed_adv = deserialize_adv_contents::<CryptoProviderImpl>(remaining).unwrap();
    if let IntermediateAdvContents::Plaintext(parc) = parsed_adv {
        assert_eq!(
            PlaintextAdvContents {
                identity_type: PlaintextIdentityMode::Public,
                data_elements: vec![PlainDataElement::Actions(ActionsDataElement::from(
                    ActionBits::default()
                ))],
            },
            parc
        )
    } else {
        panic!("Unexpected variant: {:?}", parsed_adv);
    }
}

#[test]
fn decrypt_with_wrong_key_seed_error() {
    let salt = ldt_np_adv::LegacySalt::from([0x22; 2]);
    let metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN] = [0x33; NP_LEGACY_METADATA_KEY_LEN];
    let correct_key_seed = [0x11_u8; 32];

    let (adv_content, correct_cipher) =
        build_ciphertext_adv_contents(salt, &metadata_key, correct_key_seed);
    let eac = parse_ciphertext_adv_contents(&correct_cipher, &adv_content.as_slice()[1..]);

    // wrong key seed doesn't work (derives wrong ldt key, wrong hmac key)
    let wrong_key_seed_cipher = {
        let key_seed = [0x22_u8; 32];

        let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
        let metadata_key_hmac: [u8; 32] =
            hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&metadata_key);
        ldt_np_adv::build_np_adv_decrypter_from_key_seed(&hkdf, metadata_key_hmac)
    };

    assert_eq!(
        DecryptError::DecryptOrVerifyError,
        eac.try_decrypt(&wrong_key_seed_cipher,).unwrap_err()
    );
}

#[test]
fn decrypt_with_wrong_hmac_key_error() {
    let salt = ldt_np_adv::LegacySalt::from([0x22; 2]);
    let metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN] = [0x33; NP_LEGACY_METADATA_KEY_LEN];
    let correct_key_seed = [0x11_u8; 32];

    let (adv_content, correct_cipher_config) =
        build_ciphertext_adv_contents(salt, &metadata_key, correct_key_seed);
    let eac = parse_ciphertext_adv_contents(&correct_cipher_config, &adv_content.as_slice()[1..]);

    let wrong_hmac_key_cipher = {
        let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&[0x10_u8; 32]);
        let metadata_key_hmac: [u8; 32] =
            hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&metadata_key);

        ldt_np_adv::build_np_adv_decrypter_from_key_seed(&hkdf, metadata_key_hmac)
    };

    assert_eq!(
        DecryptError::DecryptOrVerifyError,
        eac.try_decrypt::<CryptoProviderImpl>(&wrong_hmac_key_cipher,).unwrap_err()
    );
}

#[test]
fn decrypt_with_wrong_hmac_error() {
    let salt = ldt_np_adv::LegacySalt::from([0x22; 2]);
    let metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN] = [0x33; NP_LEGACY_METADATA_KEY_LEN];
    let correct_key_seed = [0x11_u8; 32];

    let (adv_content, correct_cipher_config) =
        build_ciphertext_adv_contents(salt, &metadata_key, correct_key_seed);
    let eac = parse_ciphertext_adv_contents(&correct_cipher_config, &adv_content.as_slice()[1..]);

    let wrong_hmac_key_cipher = {
        let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&correct_key_seed);

        ldt_np_adv::build_np_adv_decrypter_from_key_seed(&hkdf, [0x77; 32])
    };

    assert_eq!(
        DecryptError::DecryptOrVerifyError,
        eac.try_decrypt(&wrong_hmac_key_cipher,).unwrap_err()
    );
}

#[test]
fn decrypt_and_deserialize_ciphertext_with_public_adv_inside_error() {
    let key_seed = [0x11_u8; 32];
    let salt: ldt_np_adv::LegacySalt = [0x22; 2].into();
    let metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN] = [0x33; NP_LEGACY_METADATA_KEY_LEN];

    let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let ldt_key = hkdf.legacy_ldt_key();
    let metadata_key_hmac: [u8; 32] =
        hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&metadata_key);
    let cipher = ldt_np_adv::build_np_adv_decrypter_from_key_seed(&hkdf, metadata_key_hmac);

    let mut plaintext = vec![];
    plaintext.extend_from_slice(&metadata_key);

    let txpower_de = TxPowerDataElement::from(TxPower::try_from(5).unwrap());

    plaintext.push(
        encode_de_header_actual_len(DataElementType::TxPower, 1u8.try_into().unwrap()).unwrap(),
    );

    let plaintext_de_bundle: DataElementBundle<Plaintext> = txpower_de.to_de_bundle();
    plaintext.extend_from_slice(plaintext_de_bundle.contents_as_slice());
    // forge an otherwise impossible to express public identity
    plaintext.push(
        encode_de_header_actual_len(DataElementType::PublicIdentity, DeActualLength::ZERO).unwrap(),
    );

    assert_eq!(17, plaintext.len());

    let ldt = LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&ldt_key);
    ldt.encrypt(&mut plaintext, &ldt_np_adv::salt_padder::<16, CryptoProviderImpl>(salt)).unwrap();
    let ciphertext = plaintext;

    let mut adv = vec![];
    adv.push(0x00); // adv header
    adv.push(
        encode_de_header_actual_len(
            DataElementType::PrivateIdentity,
            (2 + ciphertext.len()).try_into().unwrap(),
        )
        .unwrap(),
    ); // private DE
    adv.extend_from_slice(&[0x22; 2]); // salt
    adv.extend_from_slice(&ciphertext);

    let parsed_adv = deserialize_adv_contents::<CryptoProviderImpl>(&adv[1..]).unwrap();
    if let IntermediateAdvContents::Ciphertext(eac) = parsed_adv {
        assert_eq!(
            DecryptError::DeserializeError(AdvDeserializeError::InvalidDataElementHierarchy),
            eac.try_decrypt(&cipher).unwrap_err()
        )
    } else {
        panic!("Unexpected variant: {:?}", parsed_adv);
    }
}

fn build_ciphertext_adv_contents<C: CryptoProvider>(
    salt: ldt_np_adv::LegacySalt,
    metadata_key: &[u8; 14],
    correct_key_seed: [u8; 32],
) -> (ArrayView<u8, { BLE_ADV_SVC_CONTENT_LEN }>, ldt_np_adv::LdtNpAdvDecrypterXtsAes128<C>) {
    let hkdf = np_hkdf::NpKeySeedHkdf::<C>::new(&correct_key_seed);
    let ldt_key = hkdf.legacy_ldt_key();

    let metadata_key_hmac: [u8; 32] =
        hkdf.legacy_metadata_key_hmac_key().calculate_hmac(metadata_key.as_slice());

    let correct_cipher = ldt_np_adv::build_np_adv_decrypter_from_key_seed(&hkdf, metadata_key_hmac);

    let mut builder = AdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
        EncryptedIdentityDataElementType::Private,
        salt,
        *metadata_key,
        LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&ldt_key),
    ));
    builder.add_data_element(TxPowerDataElement::from(TxPower::try_from(3).unwrap())).unwrap();
    (builder.into_advertisement().unwrap(), correct_cipher)
}

fn parse_ciphertext_adv_contents<'b>(
    cipher: &ldt_np_adv::LdtNpAdvDecrypterXtsAes128<CryptoProviderImpl>,
    adv_content: &'b [u8],
) -> EncryptedAdvContents<'b> {
    let eac = match deserialize_adv_contents::<CryptoProviderImpl>(adv_content).unwrap() {
        IntermediateAdvContents::Plaintext(_) => panic!(),
        // quick confirmation that we did get something
        IntermediateAdvContents::Ciphertext(eac) => eac,
    };

    // correct cipher works
    assert!(eac.try_decrypt(cipher).is_ok());

    eac
}

/// Construct the serialized DE and parse it
fn parse_plain_de<F>(de_type: PlainDataElementType, contents: &[u8]) -> PlainDataElement<F>
where
    F: PacketFlavor,
    actions::ActionsDataElement<F>: DataElement,
{
    let mut buf = vec![];
    buf.push(
        encode_de_header_actual_len(
            de_type.as_generic_de_type(),
            contents.len().try_into().unwrap(),
        )
        .unwrap(),
    );
    buf.extend_from_slice(contents);

    let raw_de = combinator::all_consuming(parse_de)(&buf).map(|(_remaining, de)| de).unwrap();

    let plain_des = plain_data_elements(&[raw_de]).unwrap();
    assert_eq!(1, plain_des.len());
    plain_des.first().unwrap().try_deserialize().unwrap()
}

fn plaintext_random_adv_contents_round_trip<I: Identity<Flavor = Plaintext>, F: Fn() -> I>(
    mk_identity: F,
    identity_type: PlaintextIdentityMode,
) {
    let mut rng = rand_ext::seeded_rng();
    let de_types: Vec<PlainDataElementType> = PlainDataElementType::iter()
        .filter(|t| t.supports_flavor(PacketFlavorEnum::Plaintext))
        .collect();

    for _ in 0..10_000 {
        let mut de_tuples = Vec::new();
        let mut builder = AdvBuilder::new(mk_identity());

        loop {
            let de_type = *de_types.choose(&mut rng).unwrap();
            let (de, bundle) = random_de_plaintext(de_type, &mut rng).unwrap();

            if builder.add_data_element(bundle.clone()).ok().is_none() {
                // out of room
                break;
            }

            de_tuples.push((de, de_type, bundle));
        }

        let serialized = builder.into_advertisement().unwrap();

        let (_rem, header) =
            combinator::all_consuming(parse_adv_header)(&serialized.as_slice()[..1]).unwrap();
        let parsed_adv =
            parse_raw_adv_contents::<CryptoProviderImpl>(&serialized.as_slice()[1..]).unwrap();

        assert_eq!(AdvHeader::V0, header);
        if let RawAdvertisement::Plaintext(parc) = parsed_adv {
            assert_eq!(
                PlaintextAdvRawContents {
                    identity_type,
                    data_elements: de_tuples
                        .iter()
                        .map(|(_de, de_type, bundle)| RawPlainDataElement {
                            de_type: *de_type,
                            contents: bundle.contents_as_slice(),
                        })
                        .collect()
                },
                parc
            );

            assert_eq!(
                PlaintextAdvContents {
                    identity_type,
                    data_elements: de_tuples
                        .into_iter()
                        .map(|(de, _de_type, _bundle)| de)
                        .collect(),
                },
                parc.try_deserialize().unwrap()
            )
        } else {
            panic!("Unexpected variant: {:?}", parsed_adv);
        }
    }
}
