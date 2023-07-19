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

use crypto_provider::hmac::{InvalidLength, MacError};

/// BoringSSL implemented Hmac Sha256 struct
pub struct HmacSha256(bssl_crypto::hmac::HmacSha256);

impl crypto_provider::hmac::Hmac<32> for HmacSha256 {
    fn new_from_key(key: [u8; 32]) -> Self {
        Self(bssl_crypto::hmac::HmacSha256::new(key))
    }

    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength> {
        Ok(Self(bssl_crypto::hmac::HmacSha256::new_from_slice(key)))
    }

    fn update(&mut self, data: &[u8]) {
        self.0.update(data)
    }

    fn finalize(self) -> [u8; 32] {
        self.0.finalize()
    }

    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError> {
        self.0.verify_slice(tag).map_err(|_| MacError)
    }

    fn verify(self, tag: [u8; 32]) -> Result<(), MacError> {
        self.0.verify(tag).map_err(|_| MacError)
    }

    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError> {
        self.0.verify_truncated_left(tag).map_err(|_| MacError)
    }
}

/// BoringSSL implemented Hmac Sha512 struct
pub struct HmacSha512(bssl_crypto::hmac::HmacSha512);

impl crypto_provider::hmac::Hmac<64> for HmacSha512 {
    fn new_from_key(key: [u8; 64]) -> Self {
        Self(bssl_crypto::hmac::HmacSha512::new(key))
    }

    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength> {
        Ok(Self(bssl_crypto::hmac::HmacSha512::new_from_slice(key)))
    }

    fn update(&mut self, data: &[u8]) {
        self.0.update(data)
    }

    fn finalize(self) -> [u8; 64] {
        self.0.finalize()
    }

    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError> {
        self.0.verify_slice(tag).map_err(|_| MacError)
    }

    fn verify(self, tag: [u8; 64]) -> Result<(), MacError> {
        self.0.verify(tag).map_err(|_| MacError)
    }

    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError> {
        self.0.verify_truncated_left(tag).map_err(|_| MacError)
    }
}

#[cfg(test)]
mod tests {
    use crate::Boringssl;
    use core::marker::PhantomData;
    use crypto_provider_test::hmac::*;

    #[apply(hmac_test_cases)]
    fn hmac_tests(testcase: CryptoProviderTestCase<Boringssl>) {
        testcase(PhantomData);
    }
}
