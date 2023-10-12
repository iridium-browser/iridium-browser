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
use hmac::digest::block_buffer::Eager;
use hmac::digest::consts::U256;
use hmac::digest::core_api::{
    BlockSizeUser, BufferKindUser, CoreProxy, FixedOutputCore, UpdateCore,
};
use hmac::digest::typenum::{IsLess, Le, NonZero};
use hmac::digest::{HashMarker, OutputSizeUser};
use hmac::Mac;

/// RustCrypto based hmac implementation
pub struct Hmac<D>
where
    D: OutputSizeUser,
    D: CoreProxy,
    D::Core: HashMarker
        + UpdateCore
        + FixedOutputCore
        + BufferKindUser<BufferKind = Eager>
        + Default
        + Clone,
    <D::Core as BlockSizeUser>::BlockSize: IsLess<U256>,
    Le<<D::Core as BlockSizeUser>::BlockSize, U256>: NonZero,
{
    hmac_impl: hmac::Hmac<D>,
}

impl crypto_provider::hmac::Hmac<32> for Hmac<sha2::Sha256> {
    #[allow(clippy::expect_used)]
    fn new_from_key(key: [u8; 32]) -> Self {
        hmac::Hmac::new_from_slice(&key)
            .map(|hmac| Self { hmac_impl: hmac })
            .expect("length will always be valid because input key is of fixed size")
    }

    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength> {
        hmac::Hmac::new_from_slice(key)
            .map(|hmac| Self { hmac_impl: hmac })
            .map_err(|_| InvalidLength)
    }

    fn update(&mut self, data: &[u8]) {
        self.hmac_impl.update(data);
    }

    fn finalize(self) -> [u8; 32] {
        self.hmac_impl.finalize().into_bytes().into()
    }

    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError> {
        self.hmac_impl.verify_slice(tag).map_err(|_| MacError)
    }

    fn verify(self, tag: [u8; 32]) -> Result<(), MacError> {
        self.hmac_impl.verify(&tag.into()).map_err(|_| MacError)
    }

    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError> {
        self.hmac_impl.verify_truncated_left(tag).map_err(|_| MacError)
    }
}

impl crypto_provider::hmac::Hmac<64> for Hmac<sha2::Sha512> {
    #[allow(clippy::expect_used)]
    fn new_from_key(key: [u8; 64]) -> Self {
        hmac::Hmac::new_from_slice(&key)
            .map(|hmac| Self { hmac_impl: hmac })
            .expect("length will always be valid because input key is of fixed size")
    }

    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength> {
        hmac::Hmac::new_from_slice(key)
            .map(|hmac| Self { hmac_impl: hmac })
            .map_err(|_| InvalidLength)
    }

    fn update(&mut self, data: &[u8]) {
        self.hmac_impl.update(data);
    }

    fn finalize(self) -> [u8; 64] {
        self.hmac_impl.finalize().into_bytes().into()
    }

    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError> {
        self.hmac_impl.verify_slice(tag).map_err(|_| MacError)
    }

    fn verify(self, tag: [u8; 64]) -> Result<(), MacError> {
        self.hmac_impl.verify(&tag.into()).map_err(|_| MacError)
    }

    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError> {
        self.hmac_impl.verify_truncated_left(tag).map_err(|_| MacError)
    }
}

#[cfg(test)]
mod tests {
    use crate::RustCrypto;
    use core::marker::PhantomData;
    use crypto_provider_test::hmac::*;

    #[apply(hmac_test_cases)]
    fn hmac_tests(testcase: CryptoProviderTestCase<RustCrypto>) {
        testcase(PhantomData);
    }
}
