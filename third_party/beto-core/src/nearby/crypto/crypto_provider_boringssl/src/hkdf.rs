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

//! BoringSSL based HKDF implementation. Unfortunately, because the OpenSSL and BoringSSL APIs
//! diverged for HKDF, we have to have separate implementations.
//!
//! See the _Using BoringSSL_ section in `crypto/README.md` for instructions on
//! how to test against BoringSSL, or see the subcommands in the top level crate.

use bssl_crypto::digest::Md;
use crypto_provider::hkdf::InvalidLength;

/// Struct providing BoringSSL implemented Hkdf operations.
pub struct Hkdf<M: Md>(bssl_crypto::hkdf::Hkdf<M>);

impl<M: Md> crypto_provider::hkdf::Hkdf for Hkdf<M> {
    fn new(salt: Option<&[u8]>, ikm: &[u8]) -> Self {
        Self(bssl_crypto::hkdf::Hkdf::<M>::new(salt, ikm))
    }

    fn expand_multi_info(
        &self,
        info_components: &[&[u8]],
        okm: &mut [u8],
    ) -> Result<(), InvalidLength> {
        if okm.is_empty() {
            return Ok(());
        }
        self.0.expand_multi_info(info_components, okm).map_err(|_| InvalidLength)
    }

    fn expand(&self, info: &[u8], okm: &mut [u8]) -> Result<(), InvalidLength> {
        if okm.is_empty() {
            return Ok(());
        }
        self.0.expand(info, okm).map_err(|_| InvalidLength)
    }
}

#[cfg(test)]
mod tests {
    use crate::Boringssl;
    use core::marker::PhantomData;
    use crypto_provider_test::hkdf::*;

    #[apply(hkdf_test_cases)]
    fn hkdf_tests(testcase: CryptoProviderTestCase<Boringssl>) {
        testcase(PhantomData);
    }
}
