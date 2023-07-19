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

//! Implementation of `crypto_provider::aes` types using RustCrypto's `aes`.
#![forbid(unsafe_code)]

use aes::cipher::{
    generic_array, BlockDecrypt as _, BlockEncrypt as _, KeyInit as _, KeyIvInit as _,
    StreamCipher as _,
};

use crypto_provider::aes::{
    Aes, Aes128Key, Aes256Key, AesBlock, AesCipher, AesDecryptCipher, AesEncryptCipher, AesKey,
};

/// Module implementing AES-CBC.
#[cfg(feature = "alloc")]
pub(crate) mod cbc;
#[cfg(feature = "gcm_siv")]
pub(crate) mod gcm_siv;

/// Rust crypto implementation of AES-128
pub struct Aes128;
impl Aes for Aes128 {
    type Key = Aes128Key;
    type EncryptCipher = Aes128Cipher;
    type DecryptCipher = Aes128Cipher;
}

/// Rust crypto implementation of AES-256
pub struct Aes256;
impl Aes for Aes256 {
    type Key = Aes256Key;
    type EncryptCipher = Aes256Cipher;
    type DecryptCipher = Aes256Cipher;
}

/// A Rust Crypto AES-128 cipher used for encryption and decryption
pub struct Aes128Cipher(aes::Aes128);

impl AesCipher for Aes128Cipher {
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        Self(aes::Aes128::new(key.as_array().into()))
    }
}

impl AesEncryptCipher for Aes128Cipher {
    fn encrypt(&self, block: &mut AesBlock) {
        self.0
            .encrypt_block(generic_array::GenericArray::from_mut_slice(
                block.as_mut_slice(),
            ));
    }
}

impl AesDecryptCipher for Aes128Cipher {
    fn decrypt(&self, block: &mut AesBlock) {
        self.0
            .decrypt_block(generic_array::GenericArray::from_mut_slice(
                block.as_mut_slice(),
            ))
    }
}

/// A Rust Crypto AES-256 cipher used for encryption and decryption
pub struct Aes256Cipher(aes::Aes256);

impl AesCipher for Aes256Cipher {
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        Self(aes::Aes256::new(key.as_array().into()))
    }
}

impl AesEncryptCipher for Aes256Cipher {
    fn encrypt(&self, block: &mut AesBlock) {
        self.0
            .encrypt_block(generic_array::GenericArray::from_mut_slice(
                block.as_mut_slice(),
            ));
    }
}

impl AesDecryptCipher for Aes256Cipher {
    fn decrypt(&self, block: &mut AesBlock) {
        self.0
            .decrypt_block(generic_array::GenericArray::from_mut_slice(
                block.as_mut_slice(),
            ))
    }
}

/// RustCrypto implementation of AES-CTR 128.
pub struct AesCtr128 {
    cipher: ctr::Ctr128BE<aes::Aes128>,
}

impl crypto_provider::aes::ctr::AesCtr for AesCtr128 {
    type Key = crypto_provider::aes::Aes128Key;

    fn new(key: &Self::Key, iv: [u8; 16]) -> Self {
        Self {
            cipher: ctr::Ctr128BE::new(key.as_array().into(), &iv.into()),
        }
    }

    fn encrypt(&mut self, data: &mut [u8]) {
        self.cipher.apply_keystream(data);
    }

    fn decrypt(&mut self, data: &mut [u8]) {
        self.cipher.apply_keystream(data);
    }
}

/// RustCrypto implementation of AES-CTR 256.
pub struct AesCtr256 {
    cipher: ctr::Ctr128BE<aes::Aes256>,
}

impl crypto_provider::aes::ctr::AesCtr for AesCtr256 {
    type Key = crypto_provider::aes::Aes256Key;

    fn new(key: &Self::Key, iv: [u8; 16]) -> Self {
        Self {
            cipher: ctr::Ctr128BE::new(key.as_array().into(), &iv.into()),
        }
    }

    fn encrypt(&mut self, data: &mut [u8]) {
        self.cipher.apply_keystream(data);
    }

    fn decrypt(&mut self, data: &mut [u8]) {
        self.cipher.apply_keystream(data);
    }
}

#[cfg(test)]
mod tests {
    use core::marker::PhantomData;

    use crypto_provider_test::aes::ctr::*;
    use crypto_provider_test::aes::*;

    use super::*;

    #[apply(aes_128_ctr_test_cases)]
    fn aes_128_ctr_test(testcase: CryptoProviderTestCase<AesCtr128>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_ctr_test_cases)]
    fn aes_256_ctr_test(testcase: CryptoProviderTestCase<AesCtr256>) {
        testcase(PhantomData);
    }

    #[apply(aes_128_encrypt_test_cases)]
    fn aes_128_encrypt_test(testcase: CryptoProviderTestCase<Aes128Cipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_128_decrypt_test_cases)]
    fn aes_128_decrypt_test(testcase: CryptoProviderTestCase<Aes128Cipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_encrypt_test_cases)]
    fn aes_256_encrypt_test(testcase: CryptoProviderTestCase<Aes256Cipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_decrypt_test_cases)]
    fn aes_256_decrypt_test(testcase: CryptoProviderTestCase<Aes256Cipher>) {
        testcase(PhantomData);
    }
}
