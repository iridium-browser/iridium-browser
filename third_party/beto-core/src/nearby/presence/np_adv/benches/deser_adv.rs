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

use core::marker::PhantomData;
use criterion::{black_box, criterion_group, criterion_main, Bencher, Criterion};
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_default::CryptoProviderImpl;
use ldt_np_adv::{LdtEncrypterXtsAes128, LegacySalt};

use np_adv::{
    credential::{simple::*, source::*, v0::*, v1::*},
    de_type::EncryptedIdentityDataElementType,
    deserialize_advertisement, deserialize_v0_advertisement, deserialize_v1_advertisement,
    extended::{
        data_elements::{GenericDataElement, TxPowerDataElement},
        deserialize::VerificationMode,
        serialize::{
            AdvBuilder as ExtendedAdvBuilder, MicEncrypted, SectionBuilder, SectionIdentity,
            SignedEncrypted,
        },
    },
    legacy::{
        actions::{ActionBits, ActionsDataElement},
        serialize::{AdvBuilder as LegacyAdvBuilder, LdtIdentity},
    },
    shared_data::{ContextSyncSeqNum, TxPower},
    PublicIdentity,
};
use rand::{Rng as _, SeedableRng as _};
use strum::IntoEnumIterator;

pub fn deser_adv_v1_encrypted(c: &mut Criterion) {
    let mut crypto_rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();

    for crypto_type in CryptoMaterialType::iter() {
        for &identity_type in &[VerificationMode::Mic, VerificationMode::Signature] {
            for &num_identities in &[10, 100, 1000] {
                for &num_sections in &[1, 2] {
                    // measure worst-case performance -- the correct identities will be the last
                    // num_sections of the identities to be tried
                    c.bench_function(
                        &format!(
                            "Deser V1 encrypted: crypto={crypto_type:?}/mode={identity_type:?}/ids={num_identities}/sections={num_sections}"
                        ),
                        |b| {
                            let identities = (0..num_identities)
                                .map(|_| V1Identity::random(&mut crypto_rng))
                                .collect::<Vec<_>>();

                            let mut adv_builder = ExtendedAdvBuilder::new();

                            // take the first n identities, one section per identity
                            for identity in identities.iter().take(num_sections) {
                                let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(
                                    &identity.key_seed,
                                );
                                match identity_type {
                                    VerificationMode::Mic => {
                                        let mut sb = adv_builder
                                            .section_builder(MicEncrypted::new_random_salt(
                                                &mut crypto_rng,
                                                EncryptedIdentityDataElementType::Private,
                                                &identity.extended_metadata_key,
                                                &hkdf,
                                            ))
                                            .unwrap();

                                        add_des(&mut sb);
                                        sb.add_to_advertisement();
                                    }
                                    VerificationMode::Signature => {
                                        let mut sb = adv_builder
                                            .section_builder(SignedEncrypted::new_random_salt(
                                                &mut crypto_rng,
                                                EncryptedIdentityDataElementType::Private,
                                                &identity.extended_metadata_key,
                                                &identity.key_pair,
                                                &hkdf,
                                            ))
                                            .unwrap();

                                        add_des(&mut sb);
                                        sb.add_to_advertisement();
                                    }
                                }
                            }

                            let adv = adv_builder.into_advertisement();

                            match crypto_type {
                                CryptoMaterialType::MinFootprint => run_with_v1_creds::<
                                    MinimumFootprintV1CryptoMaterial,
                                    CryptoProviderImpl,
                                >(
                                    b, identities, adv.as_slice()
                                ),
                                CryptoMaterialType::Precalculated => {
                                    run_with_v1_creds::<
                                        PrecalculatedV1CryptoMaterial,
                                        CryptoProviderImpl,
                                    >(
                                        b, identities, adv.as_slice()
                                    )
                                }
                            }
                        },
                    );
                }
            }
        }
    }
}

pub fn deser_adv_v1_plaintext(c: &mut Criterion) {
    let _rng = rand::rngs::StdRng::from_entropy();

    for &num_sections in &[1, 2, 5, 8] {
        // measure worst-case performance -- the correct identities will be the last
        // num_sections of the identities to be tried
        c.bench_function(&format!("Deser V1 plaintext: sections={num_sections}"), |b| {
            let mut adv_builder = ExtendedAdvBuilder::new();

            // take the first n identities, one section per identity
            for _ in 0..num_sections {
                let mut sb = adv_builder.section_builder(PublicIdentity::default()).unwrap();

                add_des(&mut sb);
                sb.add_to_advertisement();
            }

            let adv = adv_builder.into_advertisement();

            run_with_v1_creds::<MinimumFootprintV1CryptoMaterial, CryptoProviderImpl>(
                b,
                vec![],
                adv.as_slice(),
            )
        });
    }
}

pub fn deser_adv_v0_encrypted(c: &mut Criterion) {
    let mut rng = rand::rngs::StdRng::from_entropy();
    for crypto_type in CryptoMaterialType::iter() {
        for &num_identities in &[10, 100, 1000] {
            // measure worst-case performance -- the correct identities will be the last
            // num_sections of the identities to be tried
            c.bench_function(
                &format!("Deser V0 encrypted: crypto={crypto_type:?}/ids={num_identities}"),
                |b| {
                    let identities = (0..num_identities)
                        .map(|_| V0Identity::random(&mut rng))
                        .collect::<Vec<_>>();

                    let identity = &identities[0];
                    let hkdf =
                        np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&identity.key_seed);

                    let mut adv_builder =
                        LegacyAdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
                            EncryptedIdentityDataElementType::Private,
                            LegacySalt::from(rng.gen::<[u8; 2]>()),
                            identity.legacy_metadata_key,
                            LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(
                                &hkdf.legacy_ldt_key(),
                            ),
                        ));

                    let mut action_bits = ActionBits::default();
                    action_bits.set_action(ContextSyncSeqNum::try_from(3).unwrap());
                    adv_builder.add_data_element(ActionsDataElement::from(action_bits)).unwrap();

                    let adv = adv_builder.into_advertisement().unwrap();

                    match crypto_type {
                        CryptoMaterialType::MinFootprint => run_with_v0_creds::<
                            MinimumFootprintV0CryptoMaterial,
                            CryptoProviderImpl,
                        >(
                            b, identities, adv.as_slice()
                        ),
                        CryptoMaterialType::Precalculated => run_with_v0_creds::<
                            PrecalculatedV0CryptoMaterial,
                            CryptoProviderImpl,
                        >(
                            b, identities, adv.as_slice()
                        ),
                    }
                },
            );
        }
    }
}

type DefaultV0Credential = np_adv::credential::simple::SimpleV0Credential<
    np_adv::credential::v0::MinimumFootprintV0CryptoMaterial,
    (),
>;
type DefaultV1Credential = np_adv::credential::simple::SimpleV1Credential<
    np_adv::credential::v1::MinimumFootprintV1CryptoMaterial,
    (),
>;
type DefaultBothCredentialSource =
    np_adv::credential::source::OwnedBothCredentialSource<DefaultV0Credential, DefaultV1Credential>;

pub fn deser_adv_v0_plaintext(c: &mut Criterion) {
    let mut adv_builder = LegacyAdvBuilder::new(PublicIdentity::default());

    let mut action_bits = ActionBits::default();
    action_bits.set_action(ContextSyncSeqNum::try_from(3).unwrap());
    adv_builder.add_data_element(ActionsDataElement::from(action_bits)).unwrap();
    let adv = adv_builder.into_advertisement().unwrap();

    let cred_source: DefaultBothCredentialSource = DefaultBothCredentialSource::new_empty();

    for &num_advs in &[1, 10, 100, 1000] {
        c.bench_function(
            format!("Deser V0 plaintext with {num_advs} advertisements").as_str(),
            |b| {
                b.iter(|| {
                    for _ in 0..num_advs {
                        black_box(
                            deserialize_advertisement::<_, _, _, _, CryptoProviderImpl>(
                                black_box(adv.as_slice()),
                                black_box(&cred_source),
                            )
                            .expect("Should succeed"),
                        );
                    }
                })
            },
        );
    }
}

/// Benchmark decrypting a V0 advertisement with credentials built from the reversed list of
/// identities
fn run_with_v0_creds<M, C>(b: &mut Bencher, identities: Vec<V0Identity<C>>, adv: &[u8])
where
    M: V0CryptoMaterialExt,
    C: CryptoProvider,
{
    let mut creds = identities
        .into_iter()
        .map(|i| {
            let hkdf = np_hkdf::NpKeySeedHkdf::<C>::new(&i.key_seed);
            SimpleV0Credential::new(
                M::build_cred::<C>(
                    i.key_seed,
                    hkdf.legacy_metadata_key_hmac_key().calculate_hmac(&i.legacy_metadata_key),
                ),
                i.key_seed,
            )
        })
        .collect::<Vec<_>>();

    // reverse the identities so that we're scanning to the end of the
    // cred source for predictably bad performance
    creds.reverse();

    let cred_source = SliceCredentialSource::new(&creds);
    b.iter(|| {
        black_box(deserialize_v0_advertisement::<_, _, C>(adv, &cred_source).map(|_| 0_u8).unwrap())
    });
}

/// Benchmark decrypting a V1 advertisement with credentials built from the reversed list of
/// identities
fn run_with_v1_creds<M, C>(b: &mut Bencher, identities: Vec<V1Identity<C>>, adv: &[u8])
where
    M: V1CryptoMaterialExt,
    C: CryptoProvider,
{
    let mut creds = identities
        .into_iter()
        .map(|i| {
            let hkdf = np_hkdf::NpKeySeedHkdf::<C>::new(&i.key_seed);
            SimpleV1Credential::new(
                M::build_cred::<C>(
                    i.key_seed,
                    hkdf.extended_unsigned_metadata_key_hmac_key()
                        .calculate_hmac(&i.extended_metadata_key),
                    hkdf.extended_signed_metadata_key_hmac_key()
                        .calculate_hmac(&i.extended_metadata_key),
                    i.key_pair.public(),
                ),
                i.key_seed,
            )
        })
        .collect::<Vec<_>>();

    // reverse the identities so that we're scanning to the end of the
    // cred source for predictably bad performance
    creds.reverse();

    let cred_source = SliceCredentialSource::new(&creds);
    b.iter(|| {
        black_box(deserialize_v1_advertisement::<_, _, C>(adv, &cred_source).map(|_| 0_u8).unwrap())
    });
}
fn add_des<I: SectionIdentity>(sb: &mut SectionBuilder<I>) {
    sb.add_de_res(|_| TxPower::try_from(17).map(TxPowerDataElement::from)).unwrap();
    sb.add_de_res(|_| GenericDataElement::try_from(100_u32.into(), &[0; 10])).unwrap();
}
criterion_group!(
    benches,
    deser_adv_v1_encrypted,
    deser_adv_v1_plaintext,
    deser_adv_v0_encrypted,
    deser_adv_v0_plaintext
);
criterion_main!(benches);

struct V0Identity<C: CryptoProvider> {
    key_seed: [u8; 32],
    legacy_metadata_key: [u8; 14],
    _marker: PhantomData<C>,
}

impl<C: CryptoProvider> V0Identity<C> {
    /// Generate a new identity with random crypto material
    fn random<R: rand::Rng + rand::CryptoRng>(rng: &mut R) -> Self {
        Self { key_seed: rng.gen(), legacy_metadata_key: rng.gen(), _marker: PhantomData }
    }
}

struct V1Identity<C: CryptoProvider> {
    key_seed: [u8; 32],
    extended_metadata_key: [u8; 16],
    key_pair: np_ed25519::KeyPair<C>,
}

impl<C: CryptoProvider> V1Identity<C> {
    /// Generate a new identity with random crypto material
    fn random(rng: &mut C::CryptoRng) -> Self {
        Self {
            key_seed: rng.gen(),
            extended_metadata_key: rng.gen(),
            key_pair: np_ed25519::KeyPair::<C>::generate(),
        }
    }
}

#[derive(strum_macros::EnumIter, Debug)]
enum CryptoMaterialType {
    MinFootprint,
    Precalculated,
}

// if we get confident this is a valid shared way, could move this to the main trait
trait V0CryptoMaterialExt: V0CryptoMaterial {
    fn build_cred<C: CryptoProvider>(
        key_seed: [u8; 32],
        legacy_metadata_key_hmac: [u8; 32],
    ) -> Self;
}

trait V1CryptoMaterialExt: V1CryptoMaterial {
    fn build_cred<C: CryptoProvider>(
        key_seed: [u8; 32],
        unsigned_metadata_key_hmac: [u8; 32],
        signed_metadata_key_hmac: [u8; 32],
        pub_key: np_ed25519::PublicKey<C>,
    ) -> Self;
}

impl V0CryptoMaterialExt for MinimumFootprintV0CryptoMaterial {
    fn build_cred<C: CryptoProvider>(
        key_seed: [u8; 32],
        legacy_metadata_key_hmac: [u8; 32],
    ) -> Self {
        Self::new(key_seed, legacy_metadata_key_hmac)
    }
}

impl V0CryptoMaterialExt for PrecalculatedV0CryptoMaterial {
    fn build_cred<C: CryptoProvider>(
        key_seed: [u8; 32],
        legacy_metadata_key_hmac: [u8; 32],
    ) -> Self {
        Self::new::<C>(&key_seed, legacy_metadata_key_hmac)
    }
}

impl V1CryptoMaterialExt for MinimumFootprintV1CryptoMaterial {
    fn build_cred<C: CryptoProvider>(
        key_seed: [u8; 32],
        unsigned_metadata_key_hmac: [u8; 32],
        signed_metadata_key_hmac: [u8; 32],
        pub_key: np_ed25519::PublicKey<C>,
    ) -> Self {
        Self::new(key_seed, unsigned_metadata_key_hmac, signed_metadata_key_hmac, pub_key)
    }
}

impl V1CryptoMaterialExt for PrecalculatedV1CryptoMaterial {
    fn build_cred<C: CryptoProvider>(
        key_seed: [u8; 32],
        unsigned_metadata_key_hmac: [u8; 32],
        signed_metadata_key_hmac: [u8; 32],
        pub_key: np_ed25519::PublicKey<C>,
    ) -> Self {
        let min_foot = MinimumFootprintV1CryptoMaterial::new(
            key_seed,
            unsigned_metadata_key_hmac,
            signed_metadata_key_hmac,
            pub_key,
        );
        min_foot.to_precalculated::<C>()
    }
}
