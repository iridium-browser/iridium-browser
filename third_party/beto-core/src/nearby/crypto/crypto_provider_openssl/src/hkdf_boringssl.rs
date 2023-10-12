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

use crate::OpenSslHash;
use crypto_provider::hkdf::InvalidLength;
use std::marker::PhantomData;

/// openssl based hkdf implementation
pub struct Hkdf<H: OpenSslHash> {
    _marker: PhantomData<H>,
    salt: Option<Vec<u8>>,
    ikm: Vec<u8>,
}

impl<H: OpenSslHash> crypto_provider::hkdf::Hkdf for Hkdf<H> {
    fn new(salt: Option<&[u8]>, ikm: &[u8]) -> Self {
        Self { _marker: Default::default(), salt: salt.map(Vec::from), ikm: Vec::from(ikm) }
    }

    fn expand_multi_info(
        &self,
        info_components: &[&[u8]],
        okm: &mut [u8],
    ) -> Result<(), InvalidLength> {
        // open ssl will panic in the case of a 0 length okm, but expected behavior is no-op
        if okm.is_empty() {
            return Ok(());
        }
        openssl::hkdf::hkdf(
            okm,
            H::get_md(),
            self.ikm.as_slice(),
            self.salt.as_deref().unwrap_or_default(),
            &info_components.concat(),
        )
        .map_err(|_| InvalidLength)
        .map(|_| ())
    }

    fn expand(&self, info: &[u8], okm: &mut [u8]) -> Result<(), InvalidLength> {
        self.expand_multi_info(&[info], okm)
    }
}

#[cfg(test)]
mod tests {
    use crate::Openssl;
    use core::marker::PhantomData;
    use crypto_provider_test::hkdf::*;

    #[apply(hkdf_test_cases)]
    fn hkdf_tests(testcase: CryptoProviderTestCase<Openssl>) {
        testcase(PhantomData);
    }
}
