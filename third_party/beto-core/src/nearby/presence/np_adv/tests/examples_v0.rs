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

use crypto_provider_default::CryptoProviderImpl;
use ldt_np_adv::*;
use np_adv::legacy::data_elements::TxPowerDataElement;
use np_adv::{
    credential::{
        simple::SimpleV0Credential, source::SliceCredentialSource,
        v0::MinimumFootprintV0CryptoMaterial,
    },
    de_type::*,
    deserialize_v0_advertisement,
    legacy::deserialize::*,
    shared_data::*,
    *,
};

#[test]
fn v0_deser_plaintext() {
    let creds =
        SliceCredentialSource::<SimpleV0Credential<MinimumFootprintV0CryptoMaterial, ()>>::new(&[]);
    let adv = deserialize_v0_advertisement::<_, _, CryptoProviderImpl>(
        &[
            0x00, // adv header
            0x03, // public identity
            0x15, 0x03, // Length 1 Tx Power DE with value 3
        ],
        &creds,
    )
    .unwrap();

    match adv {
        V0AdvContents::Plaintext(p) => {
            assert_eq!(PlaintextIdentityMode::Public, p.identity());
            assert_eq!(
                vec![&PlainDataElement::TxPower(TxPowerDataElement::from(
                    TxPower::try_from(3).unwrap()
                )),],
                p.data_elements().collect::<Vec<_>>()
            );
        }
        _ => panic!("this example is plaintext"),
    }
}

#[test]
fn v0_deser_ciphertext() {
    let key_seed = [0x11_u8; 32];
    let metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN] = [0x33; NP_LEGACY_METADATA_KEY_LEN];

    let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
    let metadata_key_hmac: [u8; 32] =
        hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&metadata_key);

    // output of building a packet using AdvBuilder
    let adv = &[
        0x00, // adv header
        0x21, // private DE w/ a 2 byte payload
        0x22, 0x22, // salt
        // ciphertext for metadata key & txpower DE
        0x85, 0xBF, 0xA8, 0x83, 0x58, 0x7C, 0x50, 0xCF, 0x98, 0x38, 0xA7, 0x8A, 0xC0, 0x1C, 0x96,
        0xF9,
    ];

    let credentials: [SimpleV0Credential<_, [u8; 32]>; 1] = [SimpleV0Credential::new(
        MinimumFootprintV0CryptoMaterial::new(key_seed, metadata_key_hmac),
        key_seed,
    )];
    let cred_source = SliceCredentialSource::new(credentials.as_slice());

    let matched = match deserialize_v0_advertisement::<_, _, CryptoProviderImpl>(adv, &cred_source)
        .unwrap()
    {
        V0AdvContents::Decrypted(c) => c,
        _ => panic!("this examples is ciphertext"),
    };

    assert_eq!(&key_seed, matched.matched_credential().matched_data());
    let decrypted = matched.contents();

    assert_eq!(EncryptedIdentityDataElementType::Private, decrypted.identity_type());

    assert_eq!(&metadata_key, decrypted.metadata_key());

    assert_eq!(
        vec![&PlainDataElement::TxPower(TxPowerDataElement::from(TxPower::try_from(3).unwrap())),],
        decrypted.data_elements().collect::<Vec<_>>()
    );
}
