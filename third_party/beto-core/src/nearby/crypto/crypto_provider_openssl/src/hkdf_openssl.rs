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
        let mut ctx = openssl::pkey_ctx::PkeyCtx::new_id(openssl::pkey::Id::HKDF)
            .expect("hkdf context should not fail");
        let md = H::get_md();
        ctx.derive_init().expect("hkdf derive init should not fail");
        ctx.set_hkdf_md(md).expect("hkdf set md should not fail");
        self.salt.as_ref().map(|salt| ctx.set_hkdf_salt(salt.as_slice()));
        ctx.set_hkdf_key(self.ikm.as_slice()).expect("should be able to set key");
        ctx.add_hkdf_info(&info_components.concat()).expect("should be able to add info");
        ctx.derive(Some(okm)).map_err(|_| InvalidLength).map(|_| ())
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
