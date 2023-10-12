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
use crate::{
    credential::{
        simple::SimpleV1Credential,
        source::{CredentialSource, SliceCredentialSource},
        MatchableCredential, V1Credential,
    },
    extended::{
        data_elements::GenericDataElement,
        deserialize::{
            convert_data_elements,
            test_stubs::{HkdfCryptoMaterial, IntermediateSectionExt},
            OffsetDataElement,
        },
        serialize::{
            self, AdvBuilder, MicEncrypted, SectionBuilder, SignedEncrypted, WriteDataElement,
        },
        MAX_DE_LEN,
    },
    parse_adv_header, AdvHeader, PublicIdentity,
};
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_default::CryptoProviderImpl;
use rand::{seq::SliceRandom as _, Rng as _, SeedableRng as _};
use std::prelude::rust_2021::*;
use std::vec;

type KeyPair = np_ed25519::KeyPair<CryptoProviderImpl>;

#[test]
fn deserialize_public_identity_section() {
    do_deserialize_section_unencrypted::<PublicIdentity>(
        PublicIdentity::default(),
        PlaintextIdentityMode::Public,
        1,
        1,
    );
}

// due to lifetime issues, this is somewhat challenging to share with the 90% identical signature
// test, but if someone feels like putting in the tinkering to make it happen, please do
#[test]
fn deserialize_mic_encrypted_rand_identities_finds_correct_one() {
    let mut rng = rand::rngs::StdRng::from_entropy();
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();
    for _ in 0..100 {
        let identities = (0..100).map(|_| (rng.gen(), KeyPair::generate())).collect::<Vec<_>>();

        let chosen_index = rng.gen_range(0..identities.len());
        let (chosen_key_seed, _chosen_key_pair) = &identities[chosen_index];

        // share a metadata key to emphasize that we're _only_ using the identity to
        // differentiate
        let metadata_key: [u8; 16] = rng.gen();

        let creds = identities
            .iter()
            .enumerate()
            .map(|(index, (key_seed, key_pair))| {
                SimpleV1Credential::new(
                    HkdfCryptoMaterial::new(key_seed, &metadata_key, key_pair.public()),
                    index,
                )
            })
            .collect::<Vec<_>>();
        let cred_source = SliceCredentialSource::new(&creds);

        let identity_type =
            *EncryptedIdentityDataElementType::iter().collect::<Vec<_>>().choose(&mut rng).unwrap();

        let mut adv_builder = AdvBuilder::new();

        let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(chosen_key_seed);
        let mut section_builder = adv_builder
            .section_builder(MicEncrypted::new_random_salt(
                &mut crypto_rng,
                identity_type,
                &metadata_key,
                &hkdf,
            ))
            .unwrap();

        let (expected_de_data, expected_des, orig_des) =
            fill_section_random_des(&mut rng, &mut section_builder, 2);

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
        let (section, cred) = try_deserialize_all_creds::<_, _, CryptoProviderImpl>(
            sections[0].as_ciphertext().unwrap(),
            &cred_source,
        )
        .unwrap()
        .unwrap();

        assert_eq!(&chosen_index, cred.matched_data());

        assert_eq!(section.identity_type(), identity_type);
        assert_eq!(section.verification_mode(), VerificationMode::Mic);
        assert_eq!(section.metadata_key(), &metadata_key);
        assert_eq!(
            section.contents,
            SectionContents {
                section_header: (19 + 2 + 16 + total_de_len(&orig_des) + 16) as u8,
                de_data: ArrayView::try_from_slice(expected_de_data.as_slice()).unwrap(),
                data_elements: expected_des,
            }
        );
        assert_eq!(
            section
                .data_elements()
                .map(|de| GenericDataElement::try_from(de.de_type(), de.contents()).unwrap())
                .collect::<Vec<_>>(),
            orig_des
        );
    }
}

#[test]
fn deserialize_signature_encrypted_rand_identities_finds_correct_one() {
    let mut rng = rand::rngs::StdRng::from_entropy();
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();
    for _ in 0..100 {
        let identities = (0..100).map(|_| (rng.gen(), KeyPair::generate())).collect::<Vec<_>>();

        let chosen_index = rng.gen_range(0..identities.len());
        let (chosen_key_seed, chosen_key_pair) = &identities[chosen_index];

        // share a metadata key to emphasize that we're _only_ using the identity to
        // differentiate
        let metadata_key: [u8; 16] = rng.gen();

        let creds = identities
            .iter()
            .enumerate()
            .map(|(index, (key_seed, key_pair))| {
                let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(key_seed);
                let unsigned =
                    hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(&metadata_key);
                let signed =
                    hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(&metadata_key);
                SimpleV1Credential::new(
                    HkdfCryptoMaterial {
                        hkdf: *key_seed,
                        expected_unsigned_metadata_key_hmac: unsigned,
                        expected_signed_metadata_key_hmac: signed,
                        pub_key: key_pair.public().to_bytes(),
                    },
                    index,
                )
            })
            .collect::<Vec<_>>();
        let cred_source = SliceCredentialSource::new(&creds);

        let identity_type =
            *EncryptedIdentityDataElementType::iter().collect::<Vec<_>>().choose(&mut rng).unwrap();

        let mut adv_builder = AdvBuilder::new();

        let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(chosen_key_seed);
        let mut section_builder = adv_builder
            .section_builder(SignedEncrypted::new_random_salt(
                &mut crypto_rng,
                identity_type,
                &metadata_key,
                chosen_key_pair,
                &hkdf,
            ))
            .unwrap();

        let (expected_de_data, expected_des, orig_des) =
            fill_section_random_des(&mut rng, &mut section_builder, 2);

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
        let (section, cred) = try_deserialize_all_creds::<_, _, CryptoProviderImpl>(
            sections[0].as_ciphertext().unwrap(),
            &cred_source,
        )
        .unwrap()
        .unwrap();

        assert_eq!(&chosen_index, cred.matched_data());

        assert_eq!(section.identity_type(), identity_type);
        assert_eq!(section.verification_mode(), VerificationMode::Signature);
        assert_eq!(section.metadata_key(), &metadata_key);
        assert_eq!(
            section.contents,
            SectionContents {
                section_header: (19 + 2 + 16 + 64 + total_de_len(&orig_des)) as u8,
                de_data: ArrayView::try_from_slice(expected_de_data.as_slice()).unwrap(),
                data_elements: expected_des,
            }
        );
        assert_eq!(
            section
                .data_elements()
                .map(|de| GenericDataElement::try_from(de.de_type(), de.contents()).unwrap())
                .collect::<Vec<_>>(),
            orig_des
        );
    }
}

#[test]
fn deserialize_encrypted_no_matching_identities_finds_nothing() {
    let mut rng = rand::rngs::StdRng::from_entropy();
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();
    for _ in 0..100 {
        let signed = rng.gen();
        let mut identities = (0..100).map(|_| (rng.gen(), KeyPair::generate())).collect::<Vec<_>>();

        let chosen_index = rng.gen_range(0..identities.len());
        // remove so they won't be found later
        let (chosen_key_seed, chosen_key_pair) = identities.remove(chosen_index);

        // share a metadata key to emphasize that we're _only_ using the identity to
        // differentiate
        let metadata_key: [u8; 16] = rng.gen();

        let credentials = identities
            .iter()
            .enumerate()
            .map(|(index, (key_seed, key_pair))| {
                SimpleV1Credential::new(
                    HkdfCryptoMaterial::new(key_seed, &metadata_key, key_pair.public()),
                    index,
                )
            })
            .collect::<Vec<_>>();
        let cred_source = SliceCredentialSource::new(&credentials);

        let identity_type =
            *EncryptedIdentityDataElementType::iter().collect::<Vec<_>>().choose(&mut rng).unwrap();

        let mut adv_builder = AdvBuilder::new();

        let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&chosen_key_seed);

        // awkward split because SectionIdentity isn't object-safe, so we can't just have a
        // Box<dyn SectionIdentity> and use that in one code path
        if signed {
            let identity = SignedEncrypted::new_random_salt(
                &mut crypto_rng,
                identity_type,
                &metadata_key,
                &chosen_key_pair,
                &hkdf,
            );
            let mut section_builder = adv_builder.section_builder(identity).unwrap();
            let _ = fill_section_random_des(&mut rng, &mut section_builder, 2);
            section_builder.add_to_advertisement();
        } else {
            let identity =
                MicEncrypted::new_random_salt(&mut crypto_rng, identity_type, &metadata_key, &hkdf);
            let mut section_builder = adv_builder.section_builder(identity).unwrap();
            let _ = fill_section_random_des(&mut rng, &mut section_builder, 2);
            section_builder.add_to_advertisement();
        };

        let adv = adv_builder.into_advertisement();
        let (remaining, header) = parse_adv_header(adv.as_slice()).unwrap();
        let v1_header = if let AdvHeader::V1(h) = header {
            h
        } else {
            panic!("incorrect header");
        };

        let sections = parse_sections(&v1_header, remaining).unwrap();
        assert_eq!(1, sections.len());

        assert!(try_deserialize_all_creds::<_, _, CryptoProviderImpl>(
            sections[0].as_ciphertext().unwrap(),
            &cred_source
        )
        .unwrap()
        .is_none());
    }
}

#[test]
fn convert_data_elements_empty() {
    let orig_des = vec![];

    let (des, data) = convert_data_elements(&orig_des);

    assert_eq!(Vec::<OffsetDataElement>::new(), des);
    assert_eq!(&Vec::<u8>::new(), data.as_slice());
}

#[test]
fn convert_data_elements_just_fits() {
    // this is actually longer than any real section's worth of DEs could be since we aren't putting
    // DE headers in the array
    let orig_data = vec![0x33; 1000];

    let orig_des = vec![
        RefDataElement {
            offset: 2.into(),
            header_len: 2,
            de_type: 100_u32.into(),
            contents: &orig_data[0..10],
        },
        RefDataElement {
            offset: 3.into(),
            header_len: 2,
            de_type: 101_u32.into(),
            contents: &orig_data[10..100],
        },
        RefDataElement {
            offset: 4.into(),
            header_len: 2,
            de_type: 102_u32.into(),
            contents: &orig_data[100..NP_ADV_MAX_SECTION_LEN],
        },
    ];

    let (des, data) = convert_data_elements(&orig_des);

    assert_eq!(
        &[
            OffsetDataElement {
                offset: 2.into(),
                de_type: 100_u32.into(),
                start_of_contents: 0,
                contents_len: 10
            },
            OffsetDataElement {
                offset: 3.into(),
                de_type: 101_u32.into(),
                start_of_contents: 10,
                contents_len: 90
            },
            OffsetDataElement {
                offset: 4.into(),
                de_type: 102_u32.into(),
                start_of_contents: 100,
                contents_len: NP_ADV_MAX_SECTION_LEN - 100,
            },
        ],
        &des[..]
    );
    assert_eq!(&[0x33; NP_ADV_MAX_SECTION_LEN], data.as_slice());
}

#[test]
#[should_panic]
fn convert_data_elements_doesnt_fit_panic() {
    let orig_data = vec![0x33; 1000];
    let orig_des = vec![
        RefDataElement {
            offset: 2.into(),
            header_len: 2,
            de_type: 100_u32.into(),
            contents: &orig_data[0..10],
        },
        // impossibly large DE
        RefDataElement {
            offset: 3.into(),
            header_len: 2,
            de_type: 101_u32.into(),
            contents: &orig_data[10..500],
        },
    ];

    let _ = convert_data_elements(&orig_des);
}

#[test]
fn section_des_expose_correct_data() {
    let mut orig_data = Vec::new();
    orig_data.resize(130, 0);
    for (index, byte) in orig_data.iter_mut().enumerate() {
        *byte = index as u8;
    }

    let orig_des = vec![
        OffsetDataElement {
            offset: 2.into(),
            de_type: 100_u32.into(),
            start_of_contents: 0,
            contents_len: 10,
        },
        OffsetDataElement {
            offset: 3.into(),
            de_type: 101_u32.into(),
            start_of_contents: 10,
            contents_len: 90,
        },
        OffsetDataElement {
            offset: 4.into(),
            de_type: 102_u32.into(),
            start_of_contents: 100,
            contents_len: 30,
        },
    ];

    let section = SectionContents {
        section_header: 99,
        de_data: ArrayView::try_from_slice(&orig_data).unwrap(),
        data_elements: orig_des,
    };

    // extract out the parts of the DE we care about
    let des = section
        .data_elements()
        .map(|de| (de.offset(), de.de_type(), de.contents().to_vec()))
        .collect::<Vec<_>>();
    assert_eq!(
        vec![
            (2.into(), 100_u32.into(), orig_data[0..10].to_vec()),
            (3.into(), 101_u32.into(), orig_data[10..100].to_vec()),
            (4.into(), 102_u32.into(), orig_data[100..].to_vec())
        ],
        des
    );
}

#[test]
fn do_deserialize_zero_section_header() {
    let mut adv: tinyvec::ArrayVec<[u8; 254]> = tinyvec::ArrayVec::new();
    adv.push(0x20); // V1 Advertisement
    adv.push(0x00); // Section header of 0
    let (remaining, header) = parse_adv_header(adv.as_slice()).unwrap();
    let v1_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };
    parse_sections(&v1_header, remaining).expect_err("Expected an error");
}

#[test]
fn do_deserialize_empty_section() {
    let adv_builder = AdvBuilder::new();
    let adv = adv_builder.into_advertisement();
    let (remaining, header) = parse_adv_header(adv.as_slice()).unwrap();
    let v1_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };
    parse_sections(&v1_header, remaining).expect_err("Expected an error");
}

#[test]
fn do_deserialize_max_number_of_sections() {
    let adv_builder = build_dummy_advertisement_sections(NP_V1_ADV_MAX_SECTION_COUNT);
    let adv = adv_builder.into_advertisement();
    let (remaining, header) = parse_adv_header(adv.as_slice()).unwrap();

    let v1_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };

    let sections = parse_sections(&v1_header, remaining).unwrap();
    assert_eq!(NP_V1_ADV_MAX_SECTION_COUNT, sections.len());
}

#[test]
fn try_deserialize_over_max_number_of_sections() {
    let adv_builder = build_dummy_advertisement_sections(NP_V1_ADV_MAX_SECTION_COUNT);
    let mut adv = adv_builder.into_advertisement().as_slice().to_vec();

    // Push an extra section
    adv.extend_from_slice(
        [
            0x01, // Section header
            0x03, // Public identity
        ]
        .as_slice(),
    );

    let (remaining, header) = parse_adv_header(&adv).unwrap();

    let v1_header = if let AdvHeader::V1(h) = header {
        h
    } else {
        panic!("incorrect header");
    };
    parse_sections(&v1_header, remaining)
        .expect_err("Expected an error because number of sections is over 16");
}

pub(crate) fn random_de<R: rand::Rng>(rng: &mut R) -> GenericDataElement {
    let mut array = [0_u8; MAX_DE_LEN];
    rng.fill(&mut array[..]);
    let data: ArrayView<u8, MAX_DE_LEN> =
        ArrayView::try_from_array(array, rng.gen_range(0..=MAX_DE_LEN)).unwrap();
    // skip the first few DEs that Google uses
    GenericDataElement::try_from(rng.gen_range(20_u32..1000).into(), data.as_slice()).unwrap()
}

fn do_deserialize_section_unencrypted<I: serialize::SectionIdentity>(
    identity: I,
    expected_identity: PlaintextIdentityMode,
    prefix_len: usize,
    de_offset: usize,
) {
    let mut rng = rand::rngs::StdRng::from_entropy();

    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(identity).unwrap();

    let (expected_de_data, expected_des, orig_des) =
        fill_section_random_des(&mut rng, &mut section_builder, de_offset);

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
    let section = sections[0].as_plaintext().unwrap();

    assert_eq!(
        &PlaintextSection {
            identity: expected_identity,
            contents: SectionContents {
                section_header: (prefix_len + total_de_len(&orig_des)) as u8,
                de_data: ArrayView::try_from_slice(expected_de_data.as_slice()).unwrap(),
                data_elements: expected_des,
            }
        },
        section
    );
    assert_eq!(
        section
            .contents
            .data_elements()
            .map(|de| GenericDataElement::try_from(de.de_type(), de.contents()).unwrap())
            .collect::<Vec<_>>(),
        orig_des
    );
}

fn fill_section_random_des<R: rand::Rng, I: serialize::SectionIdentity>(
    mut rng: &mut R,
    section_builder: &mut SectionBuilder<I>,
    de_offset: usize,
) -> (Vec<u8>, Vec<OffsetDataElement>, Vec<GenericDataElement>) {
    let mut expected_de_data = vec![];
    let mut expected_des = vec![];
    let mut orig_des = vec![];

    for index in 0..rng.gen_range(1..10) {
        let de = random_de(&mut rng);

        let de_clone = de.clone();
        if section_builder.add_de(|_| de_clone).is_err() {
            break;
        }

        let orig_len = expected_de_data.len();
        de.write_de_contents(&mut expected_de_data).unwrap();
        let contents_len = expected_de_data.len() - orig_len;

        expected_des.push(OffsetDataElement {
            offset: (index as usize + de_offset).into(),
            de_type: de.de_header().de_type,
            contents_len,
            start_of_contents: orig_len,
        });
        orig_des.push(de);
    }
    (expected_de_data, expected_des, orig_des)
}

fn total_de_len(des: &[GenericDataElement]) -> usize {
    des.iter()
        .map(|de| {
            let mut buf = vec![];
            de.write_de_contents(&mut buf);
            de.de_header().serialize().len() + buf.len()
        })
        .sum()
}

type TryDeserOutput<'c, C> = Option<(DecryptedSection, <C as MatchableCredential>::Matched<'c>)>;

/// Returns:
/// - `Ok(Some)` if a matching credential was found
/// - `Ok(None)` if no matching credential was found, or if `cred_source` provides no credentials
/// - `Err` if an error occurred.
fn try_deserialize_all_creds<'c, C, S, P>(
    section: &CiphertextSection,
    cred_source: &'c S,
) -> Result<TryDeserOutput<'c, C>, BatchSectionDeserializeError>
where
    C: V1Credential,
    S: CredentialSource<C>,
    P: CryptoProvider,
{
    for c in cred_source.iter() {
        match section.try_deserialize::<C, P>(c) {
            Ok(s) => return Ok(Some((s, c.matched()))),
            Err(e) => match e {
                SectionDeserializeError::IncorrectCredential => continue,
                SectionDeserializeError::ParseError => {
                    return Err(BatchSectionDeserializeError::ParseError)
                }
            },
        }
    }

    Ok(None)
}

fn build_dummy_advertisement_sections(number_of_sections: usize) -> AdvBuilder {
    let mut adv_builder = AdvBuilder::new();
    for _ in 0..number_of_sections {
        let section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();
        section_builder.add_to_advertisement();
    }
    adv_builder
}

#[derive(Debug, PartialEq, Eq)]
enum BatchSectionDeserializeError {
    /// Advertisement data is malformed
    ParseError,
}
