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

use aes::cipher::KeyIvInit;
use aes::cipher::StreamCipher;
use crypto_provider::aes::{ctr::NonceAndCounter, AesKey};

/// RustCrypto implementation of AES-CTR 128.
pub struct AesCtr128 {
    cipher: ctr::Ctr128BE<aes::Aes128>,
}

impl crypto_provider::aes::ctr::AesCtr for AesCtr128 {
    type Key = crypto_provider::aes::Aes128Key;

    fn new(key: &Self::Key, nonce_and_counter: NonceAndCounter) -> Self {
        Self {
            cipher: ctr::Ctr128BE::new(
                key.as_array().into(),
                &nonce_and_counter.as_block_array().into(),
            ),
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

    fn new(key: &Self::Key, nonce_and_counter: NonceAndCounter) -> Self {
        Self {
            cipher: ctr::Ctr128BE::new(
                key.as_array().into(),
                &nonce_and_counter.as_block_array().into(),
            ),
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
}
