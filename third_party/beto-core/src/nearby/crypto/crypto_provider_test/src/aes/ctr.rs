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
use crate::aes::{Aes128Key, Aes256Key};
pub use crate::prelude;
use core::marker;
use crypto_provider::aes::ctr::{AesCtr, NonceAndCounter};
use hex_literal::hex;
use rstest_reuse::template;

/// Test AES-128-CTR encryption
pub fn aes_128_ctr_test_encrypt<A: AesCtr<Key = Aes128Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.5.1
    let key: Aes128Key = hex!("2b7e151628aed2a6abf7158809cf4f3c").into();
    let iv = hex!("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    let mut block: [u8; 16];
    let mut cipher = A::new(&key, NonceAndCounter::from_block(iv));

    block = hex!("6bc1bee22e409f96e93d7e117393172a");
    cipher.encrypt(&mut block);
    let expected_ciphertext_1 = hex!("874d6191b620e3261bef6864990db6ce");
    assert_eq!(expected_ciphertext_1, block);

    block = hex!("ae2d8a571e03ac9c9eb76fac45af8e51");
    cipher.encrypt(&mut block);
    let expected_ciphertext_2 = hex!("9806f66b7970fdff8617187bb9fffdff");
    assert_eq!(expected_ciphertext_2, block);

    block = hex!("30c81c46a35ce411e5fbc1191a0a52ef");
    cipher.encrypt(&mut block);
    let expected_ciphertext_3 = hex!("5ae4df3edbd5d35e5b4f09020db03eab");
    assert_eq!(expected_ciphertext_3, block);

    block = hex!("f69f2445df4f9b17ad2b417be66c3710");
    cipher.encrypt(&mut block);
    let expected_ciphertext_3 = hex!("1e031dda2fbe03d1792170a0f3009cee");
    assert_eq!(expected_ciphertext_3, block);
}

/// Test AES-128-CTR decryption
pub fn aes_128_ctr_test_decrypt<A: AesCtr<Key = Aes128Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.5.2
    let key: Aes128Key = hex!("2b7e151628aed2a6abf7158809cf4f3c").into();
    let iv = hex!("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    let mut block: [u8; 16];
    let mut cipher = A::new(&key, NonceAndCounter::from_block(iv));

    block = hex!("874d6191b620e3261bef6864990db6ce");
    cipher.decrypt(&mut block);
    let expected_plaintext_1 = hex!("6bc1bee22e409f96e93d7e117393172a");
    assert_eq!(expected_plaintext_1, block);

    block = hex!("9806f66b7970fdff8617187bb9fffdff");
    cipher.decrypt(&mut block);
    let expected_plaintext_2 = hex!("ae2d8a571e03ac9c9eb76fac45af8e51");
    assert_eq!(expected_plaintext_2, block);

    block = hex!("5ae4df3edbd5d35e5b4f09020db03eab");
    cipher.decrypt(&mut block);
    let expected_plaintext_3 = hex!("30c81c46a35ce411e5fbc1191a0a52ef");
    assert_eq!(expected_plaintext_3, block);

    block = hex!("1e031dda2fbe03d1792170a0f3009cee");
    cipher.decrypt(&mut block);
    let expected_plaintext_3 = hex!("f69f2445df4f9b17ad2b417be66c3710");
    assert_eq!(expected_plaintext_3, block);
}

/// Test AES-256-CTR encryption
pub fn aes_256_ctr_test_encrypt<A: AesCtr<Key = Aes256Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.5.5
    let key: Aes256Key =
        hex!("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4").into();
    let iv = hex!("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    let mut block: [u8; 16];
    let mut cipher = A::new(&key, NonceAndCounter::from_block(iv));

    block = hex!("6bc1bee22e409f96e93d7e117393172a");
    cipher.encrypt(&mut block);
    let expected_ciphertext_1 = hex!("601ec313775789a5b7a7f504bbf3d228");
    assert_eq!(expected_ciphertext_1, block);

    block = hex!("ae2d8a571e03ac9c9eb76fac45af8e51");
    cipher.encrypt(&mut block);
    let expected_ciphertext_2 = hex!("f443e3ca4d62b59aca84e990cacaf5c5");
    assert_eq!(expected_ciphertext_2, block);

    block = hex!("30c81c46a35ce411e5fbc1191a0a52ef");
    cipher.encrypt(&mut block);
    let expected_ciphertext_3 = hex!("2b0930daa23de94ce87017ba2d84988d");
    assert_eq!(expected_ciphertext_3, block);

    block = hex!("f69f2445df4f9b17ad2b417be66c3710");
    cipher.encrypt(&mut block);
    let expected_ciphertext_3 = hex!("dfc9c58db67aada613c2dd08457941a6");
    assert_eq!(expected_ciphertext_3, block);
}

/// Test AES-256-CTR decryption
pub fn aes_256_ctr_test_decrypt<A: AesCtr<Key = Aes256Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.5.6
    let key: Aes256Key =
        hex!("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4").into();
    let iv = hex!("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
    let mut block: [u8; 16];
    let mut cipher = A::new(&key, NonceAndCounter::from_block(iv));

    block = hex!("601ec313775789a5b7a7f504bbf3d228");
    cipher.decrypt(&mut block);
    let expected_plaintext_1 = hex!("6bc1bee22e409f96e93d7e117393172a");
    assert_eq!(expected_plaintext_1, block);

    block = hex!("f443e3ca4d62b59aca84e990cacaf5c5");
    cipher.decrypt(&mut block);
    let expected_plaintext_2 = hex!("ae2d8a571e03ac9c9eb76fac45af8e51");
    assert_eq!(expected_plaintext_2, block);

    block = hex!("2b0930daa23de94ce87017ba2d84988d");
    cipher.decrypt(&mut block);
    let expected_plaintext_3 = hex!("30c81c46a35ce411e5fbc1191a0a52ef");
    assert_eq!(expected_plaintext_3, block);

    block = hex!("dfc9c58db67aada613c2dd08457941a6");
    cipher.decrypt(&mut block);
    let expected_plaintext_3 = hex!("f69f2445df4f9b17ad2b417be66c3710");
    assert_eq!(expected_plaintext_3, block);
}

/// Generates the test cases to validate the AES-128-CTR implementation.
/// For example, to test `MyAesCtr128Impl`:
///
/// ```
/// use crypto_provider::aes::ctr::testing::*;
///
/// mod tests {
///     #[apply(aes_128_ctr_test_cases)]
///     fn aes_128_ctr_tests(testcase: CryptoProviderTestCase<MyAesCtr128Impl>) {
///         testcase(MyAesCtr128Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::encrypt(aes_128_ctr_test_encrypt)]
#[case::decrypt(aes_128_ctr_test_decrypt)]
fn aes_128_ctr_test_cases<F: AesCtrFactory<Key = Aes128Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}

/// Generates the test cases to validate the AES-256-CTR implementation.
/// For example, to test `MyAesCtr256Impl`:
///
/// ```
/// use crypto_provider::aes::ctr::testing::*;
///
/// mod tests {
///     #[apply(aes_256_ctr_test_cases_impl)]
///     fn aes_256_ctr_tests(testcase: CryptoProviderTestCase<MyAesCtr256Impl>) {
///         testcase(MyAesCtr256Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::encrypt(aes_256_ctr_test_encrypt)]
#[case::decrypt(aes_256_ctr_test_decrypt)]
fn aes_256_ctr_test_cases<F: AesCtrFactory<Key = Aes256Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}
