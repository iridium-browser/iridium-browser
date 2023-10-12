// Copyright 2023 Google LLC
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
use alloc::vec::Vec;
use core::marker;

use hex_literal::hex;
use rstest_reuse::template;

pub use crate::prelude;
use crypto_provider::aes::{Aes128Key, Aes256Key};

use crypto_provider::aead::aes_gcm_siv::AesGcmSiv;

/// Test AES-GCM-SIV-128 encryption/decryption
pub fn aes_128_gcm_siv_test<A: AesGcmSiv<Key = Aes128Key>>(_marker: marker::PhantomData<A>) {
    // https://github.com/google/wycheproof/blob/master/testvectors/aes_gcm_siv_test.json
    // TC1
    let test_key = hex!("01000000000000000000000000000000");
    let nonce = hex!("030000000000000000000000");
    let aes = A::new(&test_key.into());
    let msg = hex!("");
    let mut buf = Vec::from(msg.as_slice());
    let tag = hex!("dc20e2d83f25705bb49e439eca56de25");
    assert!(aes.encrypt(&mut buf, b"", &nonce).is_ok());
    assert_eq!(&buf[..], &tag);
    // TC2
    let msg = hex!("0100000000000000");
    let ct = hex!("b5d839330ac7b786");
    let tag = hex!("578782fff6013b815b287c22493a364c");
    let mut buf = Vec::from(msg.as_slice());
    assert!(aes.encrypt(&mut buf, b"", &nonce).is_ok());
    assert_eq!(&buf[..8], &ct);
    assert_eq!(&buf[8..], &tag);
    assert_eq!(A::TAG_SIZE, buf[8..].len());
    assert!(aes.decrypt(&mut buf, b"", &nonce).is_ok());
    assert_eq!(&buf[..], &msg);
}

/// Test AES-256-GCM-SIV encryption/decryption
pub fn aes_256_gcm_siv_test<A: AesGcmSiv<Key = Aes256Key>>(_marker: marker::PhantomData<A>) {
    // https://github.com/google/wycheproof/blob/master/testvectors/aes_gcm_siv_test.json
    // TC77
    let test_key = hex!("0100000000000000000000000000000000000000000000000000000000000000");
    let nonce = hex!("030000000000000000000000");
    let aes = A::new(&test_key.into());
    let msg = hex!("0100000000000000");
    let mut buf = Vec::new();
    buf.extend_from_slice(&msg);
    let ct = hex!("c2ef328e5c71c83b");
    let tag = hex!("843122130f7364b761e0b97427e3df28");
    assert!(aes.encrypt(&mut buf, b"", &nonce).is_ok());
    assert_eq!(&buf[..8], &ct);
    assert_eq!(&buf[8..], &tag);
    assert_eq!(A::TAG_SIZE, buf[8..].len());
    assert!(aes.decrypt(&mut buf, b"", &nonce).is_ok());
    assert_eq!(&buf[..], &msg);
    // TC78
    let msg = hex!("010000000000000000000000");
    let ct = hex!("9aab2aeb3faa0a34aea8e2b1");
    let tag = hex!("8ca50da9ae6559e48fd10f6e5c9ca17e");
    let mut buf = Vec::from(msg.as_slice());
    assert!(aes.encrypt(&mut buf, b"", &nonce).is_ok());
    assert_eq!(&buf[..12], &ct);
    assert_eq!(&buf[12..], &tag);
    assert!(aes.decrypt(&mut buf, b"", &nonce).is_ok());
    assert_eq!(&buf[..], &msg);
}

/// Generates the test cases to validate the AES-128-GCM-SIV implementation.
/// For example, to test `MyAesGcmSiv128Impl`:
///
/// ```
/// use crypto_provider::aes::aes_gcm_siv::testing::*;
///
/// mod tests {
///     #[apply(aes_128_gcm_siv_test_cases)]
///     fn aes_128_gcm_siv_tests(testcase: CryptoProviderTestCase<MyAesGcmSivImpl>) {
///         testcase(MyAesGcmSiv128Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::encrypt(aes_128_gcm_siv_test)]
#[case::decrypt(aes_128_gcm_siv_test)]
fn aes_128_gcm_siv_test_cases<F: AesGcmSivFactory<Key = Aes128Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}

/// Generates the test cases to validate the AES-256-GCM-SIV implementation.
/// For example, to test `MyAesGcmSiv256Impl`:
///
/// ```
/// use crypto_provider::aes::aes_gcm_siv::testing::*;
///
/// mod tests {
///     #[apply(aes_256_gcm_siv_test_cases)]
///     fn aes_256_gcm_siv_tests(testcase: CryptoProviderTestCase<MyAesGcmSiv256Impl>) {
///         testcase(MyAesGcmSiv256Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::encrypt(aes_256_gcm_siv_test)]
#[case::decrypt(aes_256_gcm_siv_test)]
fn aes_256_gcm_siv_test_cases<F: AesGcmSivFactory<Key = Aes256Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}
