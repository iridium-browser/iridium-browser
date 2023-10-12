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

use crate::elliptic_curve::EphemeralSecretForTesting;
pub use crate::prelude::*;
use crate::TestError;
use core::marker::PhantomData;
use crypto_provider::x25519::X25519;
use crypto_provider::{
    elliptic_curve::{EcdhProvider, EphemeralSecret, PublicKey},
    CryptoRng,
};
use hex_literal::hex;
use rstest_reuse::template;

/// An ECDH provider that provides associated types for testing purposes. This can be mostly
/// considered "aliases" for the otherwise long fully-qualified associated types.
pub trait EcdhProviderForX25519Test {
    /// The ECDH Provider that is "wrapped" by this type.
    type EcdhProvider: EcdhProvider<
        X25519,
        PublicKey = <Self as EcdhProviderForX25519Test>::PublicKey,
        EphemeralSecret = <Self as EcdhProviderForX25519Test>::EphemeralSecret,
        SharedSecret = <Self as EcdhProviderForX25519Test>::SharedSecret,
    >;
    /// The public key type.
    type PublicKey: PublicKey<X25519>;
    /// The ephemeral secret type.
    type EphemeralSecret: EphemeralSecretForTesting<X25519, Impl = Self::EcdhProvider>;
    /// The shared secret type.
    type SharedSecret: Into<[u8; 32]>;
}

impl<E> EcdhProviderForX25519Test for E
where
    E: EcdhProvider<X25519>,
    E::PublicKey: PublicKey<X25519>,
    E::EphemeralSecret: EphemeralSecretForTesting<X25519>,
{
    type EcdhProvider = E;
    type PublicKey = E::PublicKey;
    type EphemeralSecret = E::EphemeralSecret;
    type SharedSecret = E::SharedSecret;
}

/// Test for `PublicKey<X25519>::to_bytes`
pub fn x25519_to_bytes_test<E: EcdhProviderForX25519Test>(_: PhantomData<E>) {
    let public_key_bytes = hex!("504a36999f489cd2fdbc08baff3d88fa00569ba986cba22548ffde80f9806829");
    let public_key = E::PublicKey::from_bytes(&public_key_bytes).unwrap();
    assert_eq!(public_key_bytes.to_vec(), public_key.to_bytes());
}

/// Random test for `PublicKey<X25519>::to_bytes`
pub fn x25519_to_bytes_random_test<E: EcdhProviderForX25519Test>(_: PhantomData<E>) {
    for _ in 1..100 {
        let public_key_bytes =
            E::EphemeralSecret::generate_random(&mut <E::EphemeralSecret as EphemeralSecret<
                X25519,
            >>::Rng::new())
            .public_key_bytes();
        let public_key = E::PublicKey::from_bytes(&public_key_bytes).unwrap();
        assert_eq!(
            E::PublicKey::from_bytes(&public_key.to_bytes()).unwrap(),
            public_key,
            "from_bytes should return the same key for `{public_key_bytes:?}`",
        );
    }
}

/// Test for X25519 Diffie-Hellman key exchange.
pub fn x25519_ecdh_test<E: EcdhProviderForX25519Test>(_: PhantomData<E>) {
    // From wycheproof ecdh_secx25519r1_ecpoint_test.json, tcId 1
    // http://google3/third_party/wycheproof/testvectors/ecdh_secx25519r1_ecpoint_test.json;l=22;rcl=375894991
    // sec1 public key manually extracted from the ASN encoded test data
    let public_key = hex!("504a36999f489cd2fdbc08baff3d88fa00569ba986cba22548ffde80f9806829");
    let private = hex!("c8a9d5a91091ad851c668b0736c1c9a02936c0d3ad62670858088047ba057475");
    let expected_shared_secret =
        hex!("436a2c040cf45fea9b29a0cb81b1f41458f863d0d61b453d0a982720d6d61320");
    let result = x25519_ecdh_test_impl::<E>(&public_key, &private).unwrap();
    assert_eq!(expected_shared_secret, result.into());
}

fn x25519_ecdh_test_impl<E: EcdhProviderForX25519Test>(
    public_key: &[u8],
    private: &[u8; 32],
) -> Result<E::SharedSecret, TestError> {
    let public_key = E::PublicKey::from_bytes(public_key).map_err(TestError::new)?;
    let ephemeral_secret = E::EphemeralSecret::from_private_components(private, &public_key)
        .map_err(TestError::new)?;
    ephemeral_secret.diffie_hellman(&public_key).map_err(TestError::new)
}

/// Wycheproof test for X25519 Diffie-Hellman.
pub fn wycheproof_x25519_test<E: EcdhProviderForX25519Test>(_: PhantomData<E>) {
    // Test cases from https://github.com/randombit/wycheproof-rs/blob/master/src/data/x25519_test.json
    let test_set = wycheproof::xdh::TestSet::load(wycheproof::xdh::TestName::X25519).unwrap();
    for test_group in test_set.test_groups {
        for test in test_group.tests {
            let result = x25519_ecdh_test_impl::<E>(
                &test.public_key,
                &test.private_key.try_into().expect("Private keys should be 32 bytes long"),
            );
            match test.result {
                wycheproof::TestResult::Valid => {
                    let shared_secret =
                        result.unwrap_or_else(|_| panic!("Test {} should succeed", test.tc_id));
                    assert_eq!(&test.shared_secret, &shared_secret.into());
                }
                wycheproof::TestResult::Invalid => {
                    result.err().unwrap_or_else(|| panic!("Test {} should fail", test.tc_id));
                }
                wycheproof::TestResult::Acceptable => {
                    if let Ok(shared_secret) = result {
                        assert_eq!(test.shared_secret, shared_secret.into());
                    }
                    // Test passes if `result` is an error because this test is "acceptable"
                }
            }
        }
    }
}

/// Generates the test cases to validate the x25519 implementation.
/// For example, to test `MyCryptoProvider`:
///
/// ```
/// use crypto_provider::x25519::testing::*;
///
/// mod tests {
///     #[apply(x25519_test_cases)]
///     fn x25519_tests(testcase: CryptoProviderTestCase<MyCryptoProvider>) {
///         testcase(PhantomData::<MyCryptoProvider>);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::x25519_to_bytes(x25519_to_bytes_test)]
#[case::x25519_to_bytes_random(x25519_to_bytes_random_test)]
#[case::x25519_ecdh(x25519_ecdh_test)]
#[case::wycheproof_x25519(wycheproof_x25519_test)]
fn x25519_test_cases<C: CryptoProvider>(#[case] testcase: CryptoProviderTestCase<C>) {}
