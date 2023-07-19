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
extern crate alloc;

use alloc::{format, string::String};
use core::marker::PhantomData;

use crypto_provider::CryptoProvider;
use hex_literal::hex;
use rand::{Rng, RngCore};
use rstest_reuse::template;

pub use rstest_reuse;

pub mod aead;
pub mod aes;
pub mod ed25519;
pub mod elliptic_curve;
pub mod hkdf;
pub mod hmac;
pub mod p256;
pub mod sha2;
pub mod x25519;

/// Common items that needs to be imported to use these test cases
pub mod prelude {
    pub use super::CryptoProviderTestCase;
    pub use rstest::rstest;
    pub use rstest_reuse;
    pub use rstest_reuse::apply;
}

/// A test case for Crypto Provider. A test case is a function that panics if the test fails.
pub type CryptoProviderTestCase<T> = fn(PhantomData<T>);

#[derive(Debug)]
pub(crate) struct TestError(String);

impl TestError {
    pub(crate) fn new<D: core::fmt::Debug>(value: D) -> Self {
        Self(format!("{value:?}"))
    }
}

/// Test for `constant_time_eq` when the two inputs are equal.
pub fn constant_time_eq_test_equal<C: CryptoProvider>(_marker: PhantomData<C>) {
    assert!(C::constant_time_eq(&hex!("00010203040506070809"), &hex!("00010203040506070809")));
}

/// Test for `constant_time_eq` when the two inputs are not equal.
pub fn constant_time_eq_test_not_equal<C: CryptoProvider>(_marker: PhantomData<C>) {
    assert!(!C::constant_time_eq(&hex!("00010203040506070809"), &hex!("00000000000000000000")));
}

/// Random tests for `constant_time_eq`.
pub fn constant_time_eq_random_test<C: CryptoProvider>(_marker: PhantomData<C>) {
    let mut rng = rand::thread_rng();
    for _ in 1..100 {
        // Test using "oracle" of ==, with possibly different lengths for a and b
        let mut a = alloc::vec![0; rng.gen_range(1..1000)];
        rng.fill_bytes(&mut a);
        let mut b = alloc::vec![0; rng.gen_range(1..1000)];
        rng.fill_bytes(&mut b);
        assert_eq!(C::constant_time_eq(&a, &b), a == b);
    }

    for _ in 1..10000 {
        // Test using "oracle" of ==, with same lengths for a and b
        let len = rng.gen_range(1..1000);
        let mut a = alloc::vec![0; len];
        rng.fill_bytes(&mut a);
        let mut b = alloc::vec![0; len];
        rng.fill_bytes(&mut b);
        assert_eq!(C::constant_time_eq(&a, &b), a == b);
    }

    for _ in 1..10000 {
        // Clones and the original should always be equal
        let mut a = alloc::vec![0; rng.gen_range(1..1000)];
        rng.fill_bytes(&mut a);
        assert!(C::constant_time_eq(&a, &a.clone()));
    }
}

/// Generates the test cases to validate the P256 implementation.
/// For example, to test `MyCryptoProvider`:
///
/// ```
/// use crypto_provider::p256::testing::*;
///
/// mod tests {
///     #[apply(constant_time_eq_test_cases)]
///     fn constant_time_eq_tests(
///             testcase: CryptoProviderTestCase<MyCryptoProvider>) {
///         testcase(PhantomData);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::constant_time_eq_test_not_equal(constant_time_eq_test_not_equal)]
#[case::constant_time_eq_test_equal(constant_time_eq_test_equal)]
#[case::constant_time_eq_random_test(constant_time_eq_random_test)]
fn constant_time_eq_test_cases<C: CryptoProvider>(#[case] testcase: CryptoProviderTestCase<C>) {}
