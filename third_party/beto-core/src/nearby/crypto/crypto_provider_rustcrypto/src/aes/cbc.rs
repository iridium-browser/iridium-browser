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

extern crate alloc;
use aes::cipher::{block_padding::Pkcs7, BlockDecryptMut, BlockEncryptMut, KeyIvInit};
use aes::Aes256;
use alloc::vec::Vec;
use crypto_provider::aes::{
    cbc::{AesCbcIv, DecryptionError},
    Aes256Key, AesKey,
};

/// RustCrypto implementation of AES-CBC with PKCS7 padding
pub enum AesCbcPkcs7Padded {}
impl crypto_provider::aes::cbc::AesCbcPkcs7Padded for AesCbcPkcs7Padded {
    fn encrypt(key: &Aes256Key, iv: &AesCbcIv, message: &[u8]) -> Vec<u8> {
        let encryptor = cbc::Encryptor::<Aes256>::new(key.as_array().into(), iv.into());
        encryptor.encrypt_padded_vec_mut::<Pkcs7>(message)
    }

    fn decrypt(
        key: &Aes256Key,
        iv: &AesCbcIv,
        ciphertext: &[u8],
    ) -> Result<Vec<u8>, DecryptionError> {
        cbc::Decryptor::<Aes256>::new(key.as_array().into(), iv.into())
            .decrypt_padded_vec_mut::<Pkcs7>(ciphertext)
            .map_err(|_| DecryptionError::BadPadding)
    }
}

#[cfg(test)]
mod tests {
    use super::AesCbcPkcs7Padded;
    use core::marker::PhantomData;
    use crypto_provider_test::aes::cbc::*;

    #[apply(aes_256_cbc_test_cases)]
    fn aes_256_cbc_test(testcase: CryptoProviderTestCase<AesCbcPkcs7Padded>) {
        testcase(PhantomData);
    }
}
