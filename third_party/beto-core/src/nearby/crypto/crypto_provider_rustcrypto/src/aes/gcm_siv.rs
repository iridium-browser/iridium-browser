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

use aes_gcm_siv::{AeadInPlace, Aes128GcmSiv, Aes256GcmSiv, KeyInit, Nonce};
extern crate alloc;
use alloc::vec::Vec;

use crypto_provider::aes::gcm_siv::GcmSivError;
use crypto_provider::aes::{Aes128Key, Aes256Key, AesKey};

pub struct AesGcmSiv128(Aes128GcmSiv);

impl crypto_provider::aes::gcm_siv::AesGcmSiv for AesGcmSiv128 {
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        Self(Aes128GcmSiv::new(key.as_slice().into()))
    }

    fn encrypt(&self, data: &mut Vec<u8>, aad: &[u8], nonce: &[u8]) -> Result<(), GcmSivError> {
        self.0
            .encrypt_in_place(Nonce::from_slice(nonce), aad, data)
            .map_err(|_| GcmSivError::EncryptOutBufferTooSmall)
    }

    fn decrypt(&self, data: &mut Vec<u8>, aad: &[u8], nonce: &[u8]) -> Result<(), GcmSivError> {
        self.0
            .decrypt_in_place(Nonce::from_slice(nonce), aad, data)
            .map_err(|_| GcmSivError::DecryptTagDoesNotMatch)
    }
}

pub struct AesGcmSiv256(Aes256GcmSiv);

impl crypto_provider::aes::gcm_siv::AesGcmSiv for AesGcmSiv256 {
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        Self(Aes256GcmSiv::new(key.as_slice().into()))
    }

    fn encrypt(&self, data: &mut Vec<u8>, aad: &[u8], nonce: &[u8]) -> Result<(), GcmSivError> {
        self.0
            .encrypt_in_place(Nonce::from_slice(nonce), aad, data)
            .map_err(|_| GcmSivError::EncryptOutBufferTooSmall)
    }

    fn decrypt(&self, data: &mut Vec<u8>, aad: &[u8], nonce: &[u8]) -> Result<(), GcmSivError> {
        self.0
            .decrypt_in_place(Nonce::from_slice(nonce), aad, data)
            .map_err(|_| GcmSivError::DecryptTagDoesNotMatch)
    }
}

#[cfg(test)]
mod tests {
    use core::marker::PhantomData;

    use crypto_provider_test::aes::gcm_siv::*;
    use crypto_provider_test::aes::*;

    use super::*;

    #[apply(aes_128_gcm_siv_test_cases)]
    fn aes_gcm_siv_128_test(testcase: CryptoProviderTestCase<AesGcmSiv128>) {
        testcase(PhantomData);
    }

    #[apply(aes_256_gcm_siv_test_cases)]
    fn aes_gcm_siv_256_test(testcase: CryptoProviderTestCase<AesGcmSiv256>) {
        testcase(PhantomData);
    }
}
