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

// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Implementation of `crypto_provider::aes` types using openssl's `symm` module.

use openssl::symm::{Cipher, Crypter, Mode};

use crypto_provider::aes::{
    cbc::{AesCbcIv, DecryptionError},
    ctr::NonceAndCounter,
    Aes, Aes128Key, Aes256Key, AesBlock, AesCipher, AesDecryptCipher, AesEncryptCipher, AesKey,
};

/// Uber struct which contains impls for AES-128 fns
pub struct Aes128;

impl Aes for Aes128 {
    type Key = Aes128Key;
    type EncryptCipher = Aes128Cipher;
    type DecryptCipher = Aes128Cipher;
}

/// Uber struct which contains impls for AES-256 fns
pub struct Aes256;

impl Aes for Aes256 {
    type Key = Aes256Key;
    type EncryptCipher = Aes256Cipher;
    type DecryptCipher = Aes256Cipher;
}

/// Wrapper around openssl's AES-128 impl
pub struct Aes128Cipher(Aes128Key);

impl AesCipher for Aes128Cipher {
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        Self(key.clone())
    }
}

impl AesEncryptCipher for Aes128Cipher {
    fn encrypt(&self, block: &mut AesBlock) {
        // openssl requires the output to be at least 32 bytes long
        let mut output = [0_u8; 32];
        let mut crypter =
            Crypter::new(Cipher::aes_128_ecb(), Mode::Encrypt, self.0.as_slice(), None).unwrap();
        crypter.pad(false);
        crypter.update(block, &mut output).unwrap();
        block.copy_from_slice(&output[..crypto_provider::aes::BLOCK_SIZE]);
    }
}

impl AesDecryptCipher for Aes128Cipher {
    fn decrypt(&self, block: &mut AesBlock) {
        // openssl requires the output to be at least 32 bytes long
        let mut output = [0_u8; 32];
        let mut crypter =
            Crypter::new(Cipher::aes_128_ecb(), Mode::Decrypt, self.0.as_slice(), None).unwrap();
        crypter.pad(false);
        crypter.update(block, &mut output).unwrap();
        block.copy_from_slice(&output[..crypto_provider::aes::BLOCK_SIZE]);
    }
}

/// Wrapper around openssl's AES-128 impl
pub struct Aes256Cipher(Aes256Key);

impl AesCipher for Aes256Cipher {
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        Self(key.clone())
    }
}

impl AesEncryptCipher for Aes256Cipher {
    fn encrypt(&self, block: &mut AesBlock) {
        // openssl requires the output to be at least 32 bytes long
        let mut output = [0_u8; 32];
        let mut crypter =
            Crypter::new(Cipher::aes_256_ecb(), Mode::Encrypt, self.0.as_slice(), None).unwrap();
        crypter.pad(false);
        crypter.update(block, &mut output).unwrap();
        block.copy_from_slice(&output[..crypto_provider::aes::BLOCK_SIZE]);
    }
}

impl AesDecryptCipher for Aes256Cipher {
    fn decrypt(&self, block: &mut AesBlock) {
        // openssl requires the output to be at least 32 bytes long
        let mut output = [0_u8; 32];
        let mut crypter =
            Crypter::new(Cipher::aes_256_ecb(), Mode::Decrypt, self.0.as_slice(), None).unwrap();
        crypter.pad(false);
        crypter.update(block, &mut output).unwrap();
        block.copy_from_slice(&output[..crypto_provider::aes::BLOCK_SIZE]);
    }
}

/// OpenSSL implementation of AES-CBC-PKCS7.
pub struct OpenSslAesCbcPkcs7;

impl crypto_provider::aes::cbc::AesCbcPkcs7Padded for OpenSslAesCbcPkcs7 {
    fn encrypt(key: &crypto_provider::aes::Aes256Key, iv: &AesCbcIv, message: &[u8]) -> Vec<u8> {
        openssl::symm::encrypt(Cipher::aes_256_cbc(), key.as_slice(), Some(iv.as_slice()), message)
            .unwrap()
    }

    fn decrypt(
        key: &crypto_provider::aes::Aes256Key,
        iv: &AesCbcIv,
        ciphertext: &[u8],
    ) -> Result<Vec<u8>, DecryptionError> {
        openssl::symm::decrypt(
            Cipher::aes_256_cbc(),
            key.as_slice(),
            Some(iv.as_slice()),
            ciphertext,
        )
        .map_err(|_| DecryptionError::BadPadding)
    }
}

/// OpenSSL implementation of AES-CTR-128
pub struct OpenSslAesCtr128 {
    enc_cipher: Crypter,
    dec_cipher: Crypter,
}

impl crypto_provider::aes::ctr::AesCtr for OpenSslAesCtr128 {
    type Key = crypto_provider::aes::Aes128Key;
    fn new(key: &Self::Key, nonce_and_counter: NonceAndCounter) -> Self {
        Self {
            enc_cipher: Crypter::new(
                Cipher::aes_128_ctr(),
                Mode::Encrypt,
                key.as_slice(),
                Some(&nonce_and_counter.as_block_array()),
            )
            .unwrap(),
            dec_cipher: Crypter::new(
                Cipher::aes_128_ctr(),
                Mode::Decrypt,
                key.as_slice(),
                Some(&nonce_and_counter.as_block_array()),
            )
            .unwrap(),
        }
    }

    fn encrypt(&mut self, data: &mut [u8]) {
        let mut in_slice = vec![0u8; data.len()];
        in_slice.copy_from_slice(data);
        let _ = self.enc_cipher.update(&in_slice, data);
    }

    fn decrypt(&mut self, data: &mut [u8]) {
        let mut in_slice = vec![0u8; data.len()];
        in_slice.copy_from_slice(data);
        let _ = self.dec_cipher.update(&in_slice, data);
    }
}

/// OpenSSL implementation of AES-CTR-256
pub struct OpenSslAesCtr256 {
    enc_cipher: Crypter,
    dec_cipher: Crypter,
}

impl crypto_provider::aes::ctr::AesCtr for OpenSslAesCtr256 {
    type Key = crypto_provider::aes::Aes256Key;
    fn new(key: &Self::Key, nonce_and_counter: NonceAndCounter) -> Self {
        Self {
            enc_cipher: Crypter::new(
                Cipher::aes_256_ctr(),
                Mode::Encrypt,
                key.as_slice(),
                Some(&nonce_and_counter.as_block_array()),
            )
            .unwrap(),
            dec_cipher: Crypter::new(
                Cipher::aes_256_ctr(),
                Mode::Decrypt,
                key.as_slice(),
                Some(&nonce_and_counter.as_block_array()),
            )
            .unwrap(),
        }
    }

    fn encrypt(&mut self, data: &mut [u8]) {
        let mut in_slice = vec![0u8; data.len()];
        in_slice.copy_from_slice(data);
        let _ = self.enc_cipher.update(&in_slice, data);
    }

    fn decrypt(&mut self, data: &mut [u8]) {
        let mut in_slice = vec![0u8; data.len()];
        in_slice.copy_from_slice(data);
        let _ = self.dec_cipher.update(&in_slice, data);
    }
}

#[cfg(test)]
mod tests {
    use core::marker::PhantomData;

    use crypto_provider_test::aes::cbc::*;
    use crypto_provider_test::aes::ctr::*;
    use crypto_provider_test::aes::*;

    use super::*;

    #[apply(aes_128_ctr_test_cases)]
    fn aes_128_ctr_test(testcase: CryptoProviderTestCase<OpenSslAesCtr128>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_ctr_test_cases)]
    fn aes_256_ctr_test(testcase: CryptoProviderTestCase<OpenSslAesCtr256>) {
        testcase(PhantomData);
    }

    #[apply(aes_128_encrypt_test_cases)]
    fn aes_128_enc_test(testcase: CryptoProviderTestCase<Aes128Cipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_128_decrypt_test_cases)]
    fn aes_128_dec_test(testcase: CryptoProviderTestCase<Aes128Cipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_encrypt_test_cases)]
    fn aes_256_enc_test(testcase: CryptoProviderTestCase<Aes256Cipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_decrypt_test_cases)]
    fn aes_256_dec_test(testcase: CryptoProviderTestCase<Aes256Cipher>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_cbc_test_cases)]
    fn aes_256_cbc_test(testcase: CryptoProviderTestCase<OpenSslAesCbcPkcs7>) {
        testcase(PhantomData);
    }
}
