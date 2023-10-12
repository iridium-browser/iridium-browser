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

use rand::{seq::SliceRandom as _, SeedableRng as _};

extern crate std;

use crate::{
    credential::{
        simple::{SimpleMatchedCredential, SimpleV0Credential},
        source::SliceCredentialSource,
        v0::MinimumFootprintV0CryptoMaterial,
    },
    de_type::EncryptedIdentityDataElementType,
    deserialize_v0_advertisement,
    legacy::{
        actions::{ActionBits, ActionsDataElement, ToActionElement},
        data_elements::DataElement,
        deserialize::PlainDataElement,
        serialize::{AdvBuilder, Identity, LdtIdentity},
        BLE_ADV_SVC_CONTENT_LEN,
    },
    shared_data::ContextSyncSeqNum,
    CredentialSource, NoIdentity, PlaintextIdentityMode, PublicIdentity, V0AdvContents,
    V0Credential,
};
use array_view::ArrayView;
use core::marker::PhantomData;
use crypto_provider::CryptoProvider;
use crypto_provider_default::CryptoProviderImpl;
use ldt_np_adv::{LdtEncrypterXtsAes128, LegacySalt};
use std::{prelude::rust_2021::*, vec};
use strum::IntoEnumIterator as _;

#[test]
fn v0_all_identities_resolvable() {
    let mut rng = rand::rngs::StdRng::from_entropy();
    for _ in 0..100 {
        let identities = (0..100).map(|_| TestIdentity::random(&mut rng)).collect::<Vec<_>>();

        let (adv, adv_config) = adv_random_identity(&mut rng, &identities);

        let creds = identities.iter().map(|i| i.credential()).collect::<Vec<_>>();
        let cred_source = SliceCredentialSource::new(&creds);

        let contents = deser_v0::<_, _, CryptoProviderImpl>(&cred_source, adv.as_slice());

        assert_adv_equals(&adv_config, &contents);
    }
}

#[test]
fn v0_only_non_matching_identities_available() {
    let mut rng = rand::rngs::StdRng::from_entropy();
    for _ in 0..100 {
        let identities = (0..100).map(|_| TestIdentity::random(&mut rng)).collect::<Vec<_>>();

        let (adv, adv_config) = adv_random_identity(&mut rng, &identities);

        let creds = identities
            .iter()
            .filter(|i| {
                // remove identity used, if any
                !adv_config.identity.map(|sci| sci.key_seed == i.key_seed).unwrap_or(false)
            })
            .map(|i| i.credential())
            .collect::<Vec<_>>();
        let cred_source = SliceCredentialSource::new(&creds);

        let contents = deser_v0::<_, _, CryptoProviderImpl>(&cred_source, adv.as_slice());

        match adv_config.identity {
            // we ended up generating plaintext, so it's fine
            None => assert_adv_equals(&adv_config, &contents),
            Some(_) => {
                // we generated an encrypted adv, but didn't include the credential
                assert_eq!(V0AdvContents::NoMatchingCredentials, contents);
            }
        }
    }
}

#[test]
fn v0_no_creds_available_error_if_encrypted() {
    let mut rng = rand::rngs::StdRng::from_entropy();
    for _ in 0..100 {
        let identities = (0..100).map(|_| TestIdentity::random(&mut rng)).collect::<Vec<_>>();

        let (adv, adv_config) = adv_random_identity(&mut rng, &identities);

        let creds = Vec::<SimpleV0Credential<MinimumFootprintV0CryptoMaterial, [u8; 32]>>::new();
        let cred_source = SliceCredentialSource::new(&creds);

        let contents = deser_v0::<_, _, CryptoProviderImpl>(&cred_source, adv.as_slice());

        match adv_config.identity {
            // we ended up generating plaintext, so it's fine
            None => assert_adv_equals(&adv_config, &contents),
            Some(_) => {
                // we generated an encrypted adv, but didn't include the credential
                assert_eq!(V0AdvContents::NoMatchingCredentials, contents);
            }
        }
    }
}

fn assert_adv_equals<'m>(
    adv_config: &AdvConfig,
    adv: &V0AdvContents<'m, SimpleMatchedCredential<'m, [u8; 32]>>,
) {
    match adv_config.identity {
        None => match adv {
            V0AdvContents::Plaintext(p) => {
                let mut action_bits = ActionBits::default();
                action_bits.set_action(ContextSyncSeqNum::try_from(3).unwrap());
                let de = ActionsDataElement::from(action_bits);

                assert_eq!(adv_config.plaintext_mode.unwrap(), p.identity());
                assert_eq!(
                    vec![&PlainDataElement::Actions(de)],
                    p.data_elements().collect::<Vec<_>>()
                )
            }
            _ => panic!("should be a plaintext adv"),
        },
        Some(_) => match adv {
            V0AdvContents::Decrypted(wmc) => {
                assert!(adv_config.plaintext_mode.is_none());

                // different generic type param, so can't re-use the DE from above
                let mut action_bits = ActionBits::default();
                action_bits.set_action(ContextSyncSeqNum::try_from(3).unwrap());
                let de = ActionsDataElement::from(action_bits);

                assert_eq!(
                    vec![&PlainDataElement::Actions(de)],
                    wmc.contents().data_elements().collect::<Vec<_>>()
                );
                assert_eq!(
                    adv_config.identity.unwrap().identity_type,
                    wmc.contents().identity_type()
                );
                assert_eq!(
                    &adv_config.identity.unwrap().legacy_metadata_key,
                    wmc.contents().metadata_key()
                );
            }
            _ => panic!("should be an encrypted adv"),
        },
    }
}

fn deser_v0<'s, C, S, P>(
    cred_source: &'s S,
    adv: &[u8],
) -> V0AdvContents<'s, SimpleMatchedCredential<'s, [u8; 32]>>
where
    C: V0Credential<Matched<'s> = SimpleMatchedCredential<'s, [u8; 32]>> + 's,
    S: CredentialSource<C>,
    P: CryptoProvider,
{
    deserialize_v0_advertisement::<C, S, P>(adv, cred_source).unwrap()
}

/// Populate an advertisement with a randomly chosen identity and a DE
fn adv_random_identity<'a, R: rand::Rng>(
    mut rng: &mut R,
    identities: &'a Vec<TestIdentity<CryptoProviderImpl>>,
) -> (ArrayView<u8, { BLE_ADV_SVC_CONTENT_LEN }>, AdvConfig<'a>) {
    let identity = identities.choose(&mut rng).unwrap();
    match rng.gen_range(0_u8..=2) {
        0 => {
            let mut adv_builder = AdvBuilder::new(NoIdentity::default());
            add_de(&mut adv_builder);

            (
                adv_builder.into_advertisement().unwrap(),
                AdvConfig::new(None, Some(PlaintextIdentityMode::None)),
            )
        }
        1 => {
            let mut adv_builder = AdvBuilder::new(PublicIdentity::default());
            add_de(&mut adv_builder);

            (
                adv_builder.into_advertisement().unwrap(),
                AdvConfig::new(None, Some(PlaintextIdentityMode::Public)),
            )
        }
        2 => {
            let mut adv_builder = AdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
                identity.identity_type,
                LegacySalt::from(rng.gen::<[u8; 2]>()),
                identity.legacy_metadata_key,
                LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&identity.hkdf().legacy_ldt_key()),
            ));
            add_de(&mut adv_builder);

            (adv_builder.into_advertisement().unwrap(), AdvConfig::new(Some(identity), None))
        }

        _ => unreachable!(),
    }
}

fn add_de<I>(adv_builder: &mut AdvBuilder<I>)
where
    I: Identity,
    ActionsDataElement<I::Flavor>: DataElement,
    ContextSyncSeqNum: ToActionElement<I::Flavor>,
{
    let mut action_bits = ActionBits::default();
    action_bits.set_action(ContextSyncSeqNum::try_from(3).unwrap());
    let de = ActionsDataElement::from(action_bits);
    adv_builder.add_data_element(de).unwrap();
}

struct TestIdentity<C: CryptoProvider> {
    identity_type: EncryptedIdentityDataElementType,
    key_seed: [u8; 32],
    legacy_metadata_key: [u8; 14],
    _marker: PhantomData<C>,
}

impl<C: CryptoProvider> TestIdentity<C> {
    /// Generate a new identity with random crypto material
    fn random<R: rand::Rng + rand::CryptoRng>(rng: &mut R) -> Self {
        Self {
            identity_type: *EncryptedIdentityDataElementType::iter()
                .collect::<Vec<_>>()
                .choose(rng)
                .unwrap(),
            key_seed: rng.gen(),
            legacy_metadata_key: rng.gen(),
            _marker: PhantomData,
        }
    }

    /// Returns a credential using crypto material from this identity
    fn credential(&self) -> SimpleV0Credential<MinimumFootprintV0CryptoMaterial, [u8; 32]> {
        let hkdf = self.hkdf();
        SimpleV0Credential::new(
            MinimumFootprintV0CryptoMaterial::new(
                self.key_seed,
                hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&self.legacy_metadata_key),
            ),
            self.key_seed,
        )
    }

    fn hkdf(&self) -> np_hkdf::NpKeySeedHkdf<C> {
        np_hkdf::NpKeySeedHkdf::new(&self.key_seed)
    }
}

struct AdvConfig<'a> {
    /// `Some` iff an encrypted identity should be used
    identity: Option<&'a TestIdentity<CryptoProviderImpl>>,
    /// `Some` iff `identity` is `None`
    plaintext_mode: Option<PlaintextIdentityMode>,
}

impl<'a> AdvConfig<'a> {
    fn new(
        identity: Option<&'a TestIdentity<CryptoProviderImpl>>,
        plaintext_mode: Option<PlaintextIdentityMode>,
    ) -> Self {
        Self { identity, plaintext_mode }
    }
}
