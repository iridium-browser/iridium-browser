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

use crypto_provider::hkdf::InvalidLength;
use hmac::digest::block_buffer::Eager;
use hmac::digest::consts::U256;
use hmac::digest::core_api::{
    BlockSizeUser, BufferKindUser, CoreProxy, FixedOutputCore, UpdateCore,
};
use hmac::digest::typenum::{IsLess, Le, NonZero};
use hmac::digest::{HashMarker, OutputSizeUser};

/// RustCrypto based hkdf implementation
pub struct Hkdf<D>
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
    hkdf_impl: hkdf::Hkdf<D>,
}

impl<D> crypto_provider::hkdf::Hkdf for Hkdf<D>
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
    fn new(salt: Option<&[u8]>, ikm: &[u8]) -> Self {
        Hkdf { hkdf_impl: hkdf::Hkdf::new(salt, ikm) }
    }

    fn expand_multi_info(
        &self,
        info_components: &[&[u8]],
        okm: &mut [u8],
    ) -> Result<(), InvalidLength> {
        self.hkdf_impl.expand_multi_info(info_components, okm).map_err(|_| InvalidLength)
    }

    fn expand(&self, info: &[u8], okm: &mut [u8]) -> Result<(), InvalidLength> {
        self.hkdf_impl.expand(info, okm).map_err(|_| InvalidLength)
    }
}

#[cfg(test)]
mod tests {
    use crate::RustCrypto;
    use core::marker::PhantomData;
    use crypto_provider_test::hkdf::*;

    #[apply(hkdf_test_cases)]
    fn hkdf_tests(testcase: CryptoProviderTestCase<RustCrypto>) {
        testcase(PhantomData);
    }
}
