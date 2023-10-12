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

use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_default::CryptoProviderImpl;
use np_adv::extended::data_elements::TxPowerDataElement;
use np_adv::extended::serialize::SingleTypeDataElement;
use np_adv::shared_data::TxPower;
use np_adv::{
    credential::{
        simple::SimpleV1Credential, source::SliceCredentialSource,
        v1::MinimumFootprintV1CryptoMaterial,
    },
    de_type::*,
    deserialize_v1_advertisement,
    extended::{
        deserialize::{Section, VerificationMode},
        serialize::{AdvBuilder, SignedEncrypted},
        NP_V1_ADV_MAX_SECTION_COUNT,
    },
    PlaintextIdentityMode, *,
};
use np_hkdf::v1_salt;

#[test]
fn v1_deser_plaintext() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();
    section_builder
        .add_de(|_salt| TxPowerDataElement::from(TxPower::try_from(6).unwrap()))
        .unwrap();
    section_builder.add_to_advertisement();
    let adv = adv_builder.into_advertisement();

    let creds =
        SliceCredentialSource::<SimpleV1Credential<MinimumFootprintV1CryptoMaterial, ()>>::new(&[]);
    let contents =
        deserialize_v1_advertisement::<_, _, CryptoProviderImpl>(adv.as_slice(), &creds).unwrap();

    assert_eq!(0, contents.invalid_sections_count());

    let sections = contents.sections().collect::<Vec<_>>();

    assert_eq!(1, sections.len());

    let section = match &sections[0] {
        V1DeserializedSection::Plaintext(s) => s,
        _ => panic!("this is a plaintext adv"),
    };
    assert_eq!(PlaintextIdentityMode::Public, section.identity());
    let data_elements = section.data_elements().collect::<Vec<_>>();
    assert_eq!(1, data_elements.len());

    let de = &data_elements[0];
    assert_eq!(v1_salt::DataElementOffset::from(1), de.offset());
    assert_eq!(TxPowerDataElement::DE_TYPE, de.de_type());
    assert_eq!(&[6], de.contents());
}

#[test]
fn v1_deser_ciphertext() {
    // identity material
    let mut rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();
    let metadata_key: [u8; 16] = rng.gen();
    let key_pair = np_ed25519::KeyPair::<CryptoProviderImpl>::generate();
    let key_seed = rng.gen();
    let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);

    // prepare advertisement
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder
        .section_builder(SignedEncrypted::new_random_salt(
            &mut rng,
            EncryptedIdentityDataElementType::Private,
            &metadata_key,
            &key_pair,
            &hkdf,
        ))
        .unwrap();
    section_builder
        .add_de(|_salt| TxPowerDataElement::from(TxPower::try_from(7).unwrap()))
        .unwrap();
    section_builder.add_to_advertisement();
    let adv = adv_builder.into_advertisement();

    let cred_array: [SimpleV1Credential<_, [u8; 32]>; 1] = [SimpleV1Credential::new(
        MinimumFootprintV1CryptoMaterial::new(
            key_seed,
            [0; 32], // Zeroing out MIC HMAC, since it's unused in examples here.
            hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key),
            key_pair.public(),
        ),
        key_seed,
    )];
    let creds = SliceCredentialSource::new(&cred_array);
    let contents =
        deserialize_v1_advertisement::<_, _, CryptoProviderImpl>(adv.as_slice(), &creds).unwrap();

    assert_eq!(0, contents.invalid_sections_count());

    let sections = contents.sections().collect::<Vec<_>>();
    assert_eq!(1, sections.len());

    let matched_credential = match &sections[0] {
        V1DeserializedSection::Decrypted(d) => d,
        _ => panic!("this is a ciphertext adv"),
    };

    assert_eq!(&key_seed, matched_credential.matched_credential().matched_data());
    let section = matched_credential.contents();

    assert_eq!(EncryptedIdentityDataElementType::Private, section.identity_type());
    assert_eq!(VerificationMode::Signature, section.verification_mode());
    assert_eq!(&metadata_key, section.metadata_key());

    let data_elements = section.data_elements().collect::<Vec<_>>();
    assert_eq!(1, data_elements.len());

    let de = &data_elements[0];
    assert_eq!(v1_salt::DataElementOffset::from(2), de.offset());
    assert_eq!(TxPowerDataElement::DE_TYPE, de.de_type());
    assert_eq!(&[7], de.contents());
}

#[test]
fn v1_deser_no_section() {
    let adv_builder = AdvBuilder::new();
    let adv = adv_builder.into_advertisement();
    let creds =
        SliceCredentialSource::<SimpleV1Credential<MinimumFootprintV1CryptoMaterial, ()>>::new(&[]);
    let v1_deserialize_error =
        deserialize_v1_advertisement::<_, _, CryptoProviderImpl>(adv.as_slice(), &creds)
            .expect_err(" Expected an error");
    assert_eq!(
        v1_deserialize_error,
        AdvDeserializationError::ParseError {
            details_hazmat: AdvDeserializationErrorDetailsHazmat::AdvertisementDeserializeError
        }
    );
}

#[test]
fn v1_deser_plaintext_over_max_sections() {
    let mut adv_builder = AdvBuilder::new();
    for _ in 0..NP_V1_ADV_MAX_SECTION_COUNT {
        let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();
        section_builder
            .add_de(|_salt| TxPowerDataElement::from(TxPower::try_from(7).unwrap()))
            .unwrap();
        section_builder.add_to_advertisement();
    }
    let mut adv = adv_builder.into_advertisement().as_slice().to_vec();
    // Push an extra section
    adv.extend_from_slice(
        [
            0x01, // Section header
            0x03, // Public identity
        ]
        .as_slice(),
    );
    let creds =
        SliceCredentialSource::<SimpleV1Credential<MinimumFootprintV1CryptoMaterial, ()>>::new(&[]);
    assert_eq!(
        deserialize_v1_advertisement::<_, _, CryptoProviderImpl>(adv.as_slice(), &creds)
            .unwrap_err(),
        AdvDeserializationError::ParseError {
            details_hazmat: AdvDeserializationErrorDetailsHazmat::AdvertisementDeserializeError
        }
    );
}
