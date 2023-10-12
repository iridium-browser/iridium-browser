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
use crypto_provider::aead::{Aead, AeadError};

use crypto_provider::aead::aes_gcm_siv::AesGcmSiv;
use crypto_provider::aes::{Aes128Key, Aes256Key, AesKey};

pub struct AesGcmSiv128(Aes128GcmSiv);

impl AesGcmSiv for AesGcmSiv128 {}

impl Aead for AesGcmSiv128 {
    const TAG_SIZE: usize = 16;
    type Nonce = [u8; 12];
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        Self(Aes128GcmSiv::new(key.as_slice().into()))
    }

    fn encrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        self.0.encrypt_in_place(Nonce::from_slice(nonce), aad, msg).map_err(|_| AeadError)
    }

    fn decrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        self.0.decrypt_in_place(Nonce::from_slice(nonce), aad, msg).map_err(|_| AeadError)
    }
}

pub struct AesGcmSiv256(Aes256GcmSiv);

impl AesGcmSiv for AesGcmSiv256 {}

impl Aead for AesGcmSiv256 {
    const TAG_SIZE: usize = 16;
    type Nonce = [u8; 12];
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        Self(Aes256GcmSiv::new(key.as_slice().into()))
    }

    fn encrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        self.0.encrypt_in_place(Nonce::from_slice(nonce), aad, msg).map_err(|_| AeadError)
    }

    fn decrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        self.0.decrypt_in_place(Nonce::from_slice(nonce), aad, msg).map_err(|_| AeadError)
    }
}

#[cfg(test)]
mod tests {
    use core::marker::PhantomData;

    use crypto_provider_test::aead::aes_gcm_siv::*;
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
