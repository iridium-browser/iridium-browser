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

extern crate std;
use crate::elliptic_curve::EphemeralSecretForTesting;
pub use crate::prelude::*;
use crate::TestError;
use core::marker::PhantomData;
use crypto_provider::p256::{P256PublicKey, P256};
use crypto_provider::{
    elliptic_curve::{EcdhProvider, EphemeralSecret, PublicKey},
    CryptoRng,
};
use hex_literal::hex;
use rstest_reuse::template;

/// An ECDH provider that provides associated types for testing purposes. This can be mostly
/// considered "aliases" for the otherwise long fully-qualified associated types.
pub trait EcdhProviderForP256Test {
    /// The ECDH Provider that is "wrapped" by this type.
    type EcdhProvider: EcdhProvider<
        P256,
        PublicKey = <Self as EcdhProviderForP256Test>::PublicKey,
        EphemeralSecret = <Self as EcdhProviderForP256Test>::EphemeralSecret,
        SharedSecret = <Self as EcdhProviderForP256Test>::SharedSecret,
    >;
    /// The public key type.
    type PublicKey: P256PublicKey;
    /// The ephemeral secret type.
    type EphemeralSecret: EphemeralSecretForTesting<P256, Impl = Self::EcdhProvider>;
    /// The shared secret type.
    type SharedSecret: Into<[u8; 32]>;
}

impl<E> EcdhProviderForP256Test for E
where
    E: EcdhProvider<P256>,
    E::PublicKey: P256PublicKey,
    E::EphemeralSecret: EphemeralSecretForTesting<P256>,
{
    type EcdhProvider = E;
    type PublicKey = E::PublicKey;
    type EphemeralSecret = E::EphemeralSecret;
    type SharedSecret = E::SharedSecret;
}

/// Test for P256PublicKey::to_bytes
pub fn to_bytes_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    let sec1_bytes = hex!(
        "04756c07ba5b596fa96c9099e6619dc62deac4297a8fc1d803d74dc5caa9197c09f0b6da270d2a58a06022
             8bbe76c6dc1643088107636deff8aa79e8002a157b92"
    );
    let key = E::PublicKey::from_sec1_bytes(&sec1_bytes).unwrap();
    let sec1_bytes_compressed =
        hex!("02756c07ba5b596fa96c9099e6619dc62deac4297a8fc1d803d74dc5caa9197c09");
    assert_eq!(sec1_bytes_compressed.to_vec(), key.to_bytes());
}

/// Random test for P256PublicKey::to_bytes
pub fn to_bytes_random_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    for _ in 1..100 {
        let public_key_bytes =
            E::EphemeralSecret::generate_random(&mut <E::EphemeralSecret as EphemeralSecret<
                P256,
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

/// Test for P256PublicKey::from_affine_coordinates
pub fn from_affine_coordinates_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    // https://www.secg.org/sec1-v2.pdf, section 2.3.3
    let x = hex!("756c07ba5b596fa96c9099e6619dc62deac4297a8fc1d803d74dc5caa9197c09");
    let y = hex!("f0b6da270d2a58a060228bbe76c6dc1643088107636deff8aa79e8002a157b92");
    let sec1 = hex!(
        "04756c07ba5b596fa96c9099e6619dc62deac4297a8fc1d803d74dc5caa9197c09f0b6da270d2a58a06022
             8bbe76c6dc1643088107636deff8aa79e8002a157b92"
    );
    let expected_key = E::PublicKey::from_sec1_bytes(&sec1).unwrap();
    assert!(
        E::PublicKey::from_affine_coordinates(&x, &y).unwrap() == expected_key,
        "Public key does not match"
    );
}

/// Test for P256PublicKey::from_affine_coordinates
pub fn from_affine_coordinates_not_on_curve_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    // (Invalid) coordinate from wycheproof ecdh_secp256r1_ecpoint_test.json, tcId 193
    let x = hex!("0000000000000000000000000000000000000000000000000000000000000000");
    let y = hex!("0000000000000000000000000000000000000000000000000000000000000000");
    let result = E::PublicKey::from_affine_coordinates(&x, &y);
    assert!(result.is_err(), "Creating public key from invalid affine coordinate should fail");
}

/// Test for P256PublicKey::from_sec1_bytes
pub fn from_sec1_bytes_not_on_curve_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    // (Invalid) sec1 encoding from wycheproof ecdh_secp256r1_ecpoint_test.json, tcId 193
    let sec1 = hex!(
        "04000000000000000000000000000000000000000000000000000000000000000000000000000000000000
             00000000000000000000000000000000000000000000"
    );
    let result = E::PublicKey::from_sec1_bytes(&sec1);
    assert!(result.is_err(), "Creating public key from point not on curve should fail");
}

/// Test for P256PublicKey::from_sec1_bytes
pub fn from_sec1_bytes_at_infinity_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    // A single [0] byte represents a point at infinity.
    let sec1 = hex!("00");
    let result = E::PublicKey::from_sec1_bytes(&sec1);
    assert!(result.is_err(), "Creating public key from point at infinity should fail");
}

/// Test for P256PublicKey::to_affine_coordinates
pub fn public_key_to_affine_coordinates_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    // https://www.secg.org/sec1-v2.pdf, section 2.3.3
    let expected_x = hex!("756c07ba5b596fa96c9099e6619dc62deac4297a8fc1d803d74dc5caa9197c09");
    let expected_y = hex!("f0b6da270d2a58a060228bbe76c6dc1643088107636deff8aa79e8002a157b92");
    let sec1 = hex!(
        "04756c07ba5b596fa96c9099e6619dc62deac4297a8fc1d803d74dc5caa9197c09f0b6da270d2a58a06022
             8bbe76c6dc1643088107636deff8aa79e8002a157b92"
    );
    let public_key = E::PublicKey::from_sec1_bytes(&sec1).unwrap();
    let (actual_x, actual_y) = public_key.to_affine_coordinates().unwrap();
    assert_eq!(actual_x, expected_x);
    assert_eq!(actual_y, expected_y);
}

/// Test for P256PublicKey::to_affine_coordinates with compressed point with 0x02 octet prefix.
/// Support for compressed points is optional according to the specs, but both openssl and
/// rustcrypto implementations support it.
pub fn public_key_to_affine_coordinates_compressed02_test<E: EcdhProviderForP256Test>(
    _: PhantomData<E>,
) {
    // https://www.secg.org/sec1-v2.pdf, section 2.3.3
    let expected_x = hex!("21238e877c2400f15f9ea7d4353ac0a63dcb5d13168a96fcfc93bdc66031ed1c");
    let expected_y = hex!("fa339bd0886602e91b9d2aa9b43213f660b680b1c97ef09cb1cacdc14e9d85ee");
    let sec1 = hex!("0221238e877c2400f15f9ea7d4353ac0a63dcb5d13168a96fcfc93bdc66031ed1c");
    let public_key = E::PublicKey::from_sec1_bytes(&sec1).unwrap();
    let (actual_x, actual_y) = public_key.to_affine_coordinates().unwrap();
    assert_eq!(actual_x, expected_x);
    assert_eq!(actual_y, expected_y);
}

/// Test for P256PublicKey::to_affine_coordinates with compressed point with 0x03 octet prefix
/// Support for compressed points is optional according to the specs, but both openssl and
/// rustcrypto implementations support it.
pub fn public_key_to_affine_coordinates_compressed03_test<E: EcdhProviderForP256Test>(
    _: PhantomData<E>,
) {
    // https://www.secg.org/sec1-v2.pdf, section 2.3.3
    let expected_x = hex!("f557ef33d52e540e6aa4e6fcbb62a314adcb051cc8a1fefc69d004c282af81ff");
    let expected_y = hex!("96cd4c6ed5cbf00bb3184e5cd983c3442160310c8519b4c4d16292be83eec539");
    let sec1 = hex!("03f557ef33d52e540e6aa4e6fcbb62a314adcb051cc8a1fefc69d004c282af81ff");
    let public_key = E::PublicKey::from_sec1_bytes(&sec1).unwrap();
    let (actual_x, actual_y) = public_key.to_affine_coordinates().unwrap();
    assert_eq!(actual_x, expected_x);
    assert_eq!(actual_y, expected_y);
}

/// Test for P256PublicKey::to_affine_coordinates with the top byte being zero
pub fn public_key_to_affine_coordinates_zero_top_byte_test<E: EcdhProviderForP256Test>(
    _: PhantomData<E>,
) {
    // https://www.secg.org/sec1-v2.pdf, section 2.3.3
    let expected_x = hex!("00f24fe76679c57bc6c2f025af92e6c0b2058fb15fa41014775987587400ed48");
    let expected_y = hex!("e09f6fa9979a60f578a62dca805ad75b9e6b89403f2ebb934869e3697ac590e9");
    let sec1 = hex!("0400f24fe76679c57bc6c2f025af92e6c0b2058fb15fa41014775987587400ed48e09f6fa9979a60f578a62dca805ad75b9e6b89403f2ebb934869e3697ac590e9");
    let public_key = E::PublicKey::from_sec1_bytes(&sec1).unwrap();
    let (actual_x, actual_y) = public_key.to_affine_coordinates().unwrap();
    assert_eq!(actual_x, expected_x);
    assert_eq!(actual_y, expected_y);
}

/// Test for P256 Diffie-Hellman key exchange.
pub fn p256_ecdh_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    // From wycheproof ecdh_secp256r1_ecpoint_test.json, tcId 1
    // http://google3/third_party/wycheproof/testvectors/ecdh_secp256r1_ecpoint_test.json;l=22;rcl=375894991
    // sec1 public key manually extracted from the ASN encoded test data
    let public_key_sec1 = hex!(
        "0462d5bd3372af75fe85a040715d0f502428e07046868b0bfdfa61d731afe44f
            26ac333a93a9e70a81cd5a95b5bf8d13990eb741c8c38872b4a07d275a014e30cf"
    );
    let private = hex!("0612465c89a023ab17855b0a6bcebfd3febb53aef84138647b5352e02c10c346");
    let expected_shared_secret =
        hex!("53020d908b0219328b658b525f26780e3ae12bcd952bb25a93bc0895e1714285");
    let actual_shared_secret = p256_ecdh_test_impl::<E>(&public_key_sec1, &private).unwrap();
    assert_eq!(actual_shared_secret.into(), expected_shared_secret);
}

fn p256_ecdh_test_impl<E: EcdhProviderForP256Test>(
    public_key_sec1: &[u8],
    private: &[u8; 32],
) -> Result<E::SharedSecret, TestError> {
    let public_key = E::PublicKey::from_sec1_bytes(public_key_sec1).map_err(TestError::new)?;
    let ephemeral_secret = E::EphemeralSecret::from_private_components(private, &public_key)
        .map_err(TestError::new)?;
    ephemeral_secret.diffie_hellman(&public_key).map_err(TestError::new)
}

/// Wycheproof test for P256 Diffie-Hellman.
pub fn wycheproof_p256_test<E: EcdhProviderForP256Test>(_: PhantomData<E>) {
    // Test cases from https://github.com/randombit/wycheproof-rs/blob/master/src/data/ecdh_secp256r1_ecpoint_test.json
    let test_set =
        wycheproof::ecdh::TestSet::load(wycheproof::ecdh::TestName::EcdhSecp256r1Ecpoint).unwrap();
    for test_group in test_set.test_groups {
        for test in test_group.tests {
            if test.private_key.len() != 32 {
                // Some Wycheproof test cases have private key length that are not 32 bytes, but
                // the RustCrypto implementation doesn't support that (it always take 32 bytes
                // from the given RNG when generating a new key).
                continue;
            };
            let result = p256_ecdh_test_impl::<E>(
                &test.public_key,
                &test.private_key.try_into().expect("Private key should be 32 bytes long"),
            );
            match test.result {
                wycheproof::TestResult::Valid => {
                    let shared_secret =
                        result.unwrap_or_else(|_| panic!("Test {} should succeed", test.tc_id));
                    assert_eq!(test.shared_secret, shared_secret.into());
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

/// Generates the test cases to validate the P256 implementation.
/// For example, to test `MyCryptoProvider`:
///
/// ```
/// use crypto_provider::p256::testing::*;
///
/// mod tests {
///     #[apply(p256_test_cases)]
///     fn p256_tests(testcase: CryptoProviderTestCase<MyCryptoProvider> {
///         testcase(PhantomData::<MyCryptoProvider>);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::to_bytes(to_bytes_test)]
#[case::to_bytes_random(to_bytes_random_test)]
#[case::from_sec1_bytes_not_on_curve(from_sec1_bytes_not_on_curve_test)]
#[case::from_sec1_bytes_not_on_infinity(from_sec1_bytes_at_infinity_test)]
#[case::from_affine_coordinates(from_affine_coordinates_test)]
#[case::from_affine_coordinates_not_on_curve(from_affine_coordinates_not_on_curve_test)]
#[case::public_key_to_affine_coordinates(public_key_to_affine_coordinates_test)]
#[case::public_key_to_affine_coordinates_compressed02(
    public_key_to_affine_coordinates_compressed02_test
)]
#[case::public_key_to_affine_coordinates_compressed03(
    public_key_to_affine_coordinates_compressed03_test
)]
#[case::public_key_to_affine_coordinates_zero_top_byte(
    public_key_to_affine_coordinates_zero_top_byte_test
)]
#[case::p256_ecdh(p256_ecdh_test)]
#[case::wycheproof_p256(wycheproof_p256_test)]
fn p256_test_cases<C: CryptoProvider>(#[case] testcase: CryptoProviderTestCase<C>) {}
