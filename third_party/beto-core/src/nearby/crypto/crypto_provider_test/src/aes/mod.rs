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

pub mod cbc;
pub mod ctr;

pub use crate::prelude::*;

use core::marker;
use crypto_provider::aes::*;
use hex_literal::hex;
use rstest_reuse::template;

/// Test encryption with AES-128
pub fn aes_128_test_encrypt<A: AesEncryptCipher<Key = Aes128Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.1.1
    let key: Aes128Key = hex!("2b7e151628aed2a6abf7158809cf4f3c").into();
    let mut block = [0_u8; 16];
    let enc_cipher = A::new(&key);

    block.copy_from_slice(&hex!("6bc1bee22e409f96e93d7e117393172a"));
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("3ad77bb40d7a3660a89ecaf32466ef97"), block);

    block.copy_from_slice(&hex!("ae2d8a571e03ac9c9eb76fac45af8e51"));
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("f5d3d58503b9699de785895a96fdbaaf"), block);

    block.copy_from_slice(&hex!("30c81c46a35ce411e5fbc1191a0a52ef"));
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("43b1cd7f598ece23881b00e3ed030688"), block);

    block.copy_from_slice(&hex!("f69f2445df4f9b17ad2b417be66c3710"));
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("7b0c785e27e8ad3f8223207104725dd4"), block);
}

/// Test decryption with AES-128
pub fn aes_128_test_decrypt<A: AesDecryptCipher<Key = Aes128Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.1.2
    let key: Aes128Key = hex!("2b7e151628aed2a6abf7158809cf4f3c").into();
    let mut block = [0_u8; 16];
    let dec_cipher = A::new(&key);

    block.copy_from_slice(&hex!("3ad77bb40d7a3660a89ecaf32466ef97"));
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("6bc1bee22e409f96e93d7e117393172a"), block);

    block.copy_from_slice(&hex!("f5d3d58503b9699de785895a96fdbaaf"));
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("ae2d8a571e03ac9c9eb76fac45af8e51"), block);

    block.copy_from_slice(&hex!("43b1cd7f598ece23881b00e3ed030688"));
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("30c81c46a35ce411e5fbc1191a0a52ef"), block);

    block.copy_from_slice(&hex!("7b0c785e27e8ad3f8223207104725dd4"));
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("f69f2445df4f9b17ad2b417be66c3710"), block);
}

/// Test encryption with AES-256
pub fn aes_256_test_encrypt<A: AesEncryptCipher<Key = Aes256Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.1.5
    let key: Aes256Key =
        hex!("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4").into();
    let mut block: [u8; 16];
    let enc_cipher = A::new(&key);

    block = hex!("6bc1bee22e409f96e93d7e117393172a");
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("f3eed1bdb5d2a03c064b5a7e3db181f8"), block);

    block = hex!("ae2d8a571e03ac9c9eb76fac45af8e51");
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("591ccb10d410ed26dc5ba74a31362870"), block);

    block = hex!("30c81c46a35ce411e5fbc1191a0a52ef");
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("b6ed21b99ca6f4f9f153e7b1beafed1d"), block);

    block = hex!("f69f2445df4f9b17ad2b417be66c3710");
    enc_cipher.encrypt(&mut block);
    assert_eq!(hex!("23304b7a39f9f3ff067d8d8f9e24ecc7"), block);
}

/// Test decryption with AES-256
pub fn aes_256_test_decrypt<A: AesDecryptCipher<Key = Aes256Key>>(_marker: marker::PhantomData<A>) {
    // https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf F.1.6
    let key: Aes256Key =
        hex!("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4").into();
    let mut block: [u8; 16];
    let dec_cipher = A::new(&key);

    block = hex!("f3eed1bdb5d2a03c064b5a7e3db181f8");
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("6bc1bee22e409f96e93d7e117393172a"), block);

    block = hex!("591ccb10d410ed26dc5ba74a31362870");
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("ae2d8a571e03ac9c9eb76fac45af8e51"), block);

    block = hex!("b6ed21b99ca6f4f9f153e7b1beafed1d");
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("30c81c46a35ce411e5fbc1191a0a52ef"), block);

    block = hex!("23304b7a39f9f3ff067d8d8f9e24ecc7");
    dec_cipher.decrypt(&mut block);
    assert_eq!(hex!("f69f2445df4f9b17ad2b417be66c3710"), block);
}

/// Generates the test cases to validate the AES-128 implementation.
/// For example, to test `MyAes128Impl`:
///
/// ```
/// use crypto_provider::aes::testing::*;
///
/// mod tests {
///     #[apply(aes_128_encrypt_test_cases)]
///     fn aes_128_tests(f: CryptoProviderTestCase<MyAes128Impl>) {
///         f(MyAes128Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::encrypt(aes_128_test_encrypt)]
fn aes_128_encrypt_test_cases<A: AesFactory<Key = Aes128Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}

/// Generates the test cases to validate the AES-128 implementation.
/// For example, to test `MyAes128Impl`:
///
/// ```
/// use crypto_provider::aes::testing::*;
///
/// mod tests {
///     #[apply(aes_128_decrypt_test_cases)]
///     fn aes_128_tests(f: CryptoProviderTestCase<MyAes128Impl>) {
///         f(MyAes128Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::decrypt(aes_128_test_decrypt)]
fn aes_128_decrypt_test_cases<F: AesFactory<Key = Aes128Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}

/// Generates the test cases to validate the AES-256 implementation.
/// For example, to test `MyAes256Impl`:
///
/// ```
/// use crypto_provider::aes::testing::*;
///
/// mod tests {
///     #[apply(aes_256_encrypt_test_cases)]
///     fn aes_256_tests(f: CryptoProviderTestCase<MyAes256Impl>) {
///         f(MyAes256Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::encrypt(aes_256_test_encrypt)]
fn aes_256_encrypt_test_cases<F: AesFactory<Key = Aes256Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}

/// Generates the test cases to validate the AES-256 implementation.
/// For example, to test `MyAes256Impl`:
///
/// ```
/// use crypto_provider::aes::testing::*;
///
/// mod tests {
///     #[apply(aes_256_decrypt_test_cases)]
///     fn aes_256_tests(f: CryptoProviderTestCase<MyAes256Impl>) {
///         f(MyAes256Impl);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::decrypt(aes_256_test_decrypt)]
fn aes_256_decrypt_test_cases<F: AesFactory<Key = Aes256Key>>(
    #[case] testcase: CryptoProviderTestCase<F>,
) {
}
