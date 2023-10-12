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

//! BoringSSL based HMAC implementation. Unfortunately, because the OpenSSL and BoringSSL APIs
//! diverged (https://boringssl.googlesource.com/boringssl/+/HEAD/PORTING.md#hmac-s), we have to
//! have separate implementations.
//!
//! See the _Using BoringSSL_ section in `crypto/README.md` for instructions on
//! how to test against BoringSSL, or see the subcommands in the top level crate.

use crate::{sha2::OpenSslSha256, sha2::OpenSslSha512, OpenSslHash};
use crypto_provider::hmac::{InvalidLength, MacError};
use openssl::hmac::HmacCtx;
use openssl::memcmp;
use std::marker::PhantomData;

/// BoringSSL based HMAC implementation
pub struct Hmac<H: OpenSslHash> {
    marker: PhantomData<H>,
    ctx: HmacCtx,
}

trait Hash<const N: usize>: OpenSslHash {}
impl Hash<32> for OpenSslSha256 {}
impl Hash<64> for OpenSslSha512 {}

impl<const N: usize, H: Hash<N>> crypto_provider::hmac::Hmac<N> for Hmac<H> {
    fn new_from_key(key: [u8; N]) -> Self {
        Self { marker: PhantomData, ctx: HmacCtx::new(&key, H::get_md()).unwrap() }
    }

    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength> {
        Ok(Self { marker: PhantomData, ctx: HmacCtx::new(key, H::get_md()).unwrap() })
    }

    fn update(&mut self, data: &[u8]) {
        self.ctx.update(data).expect("should be able to update signer");
    }

    fn finalize(mut self) -> [u8; N] {
        let mut buf = [0_u8; N];
        self.ctx.finalize(&mut buf).expect("wrong length");
        buf
    }

    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError> {
        let binding: [u8; N] = self.finalize();
        let slice = binding.as_slice();
        if memcmp::eq(slice, tag) {
            Ok(())
        } else {
            Err(MacError)
        }
    }

    fn verify(self, tag: [u8; N]) -> Result<(), MacError> {
        let binding: [u8; N] = self.finalize();
        let slice = binding.as_slice();
        if memcmp::eq(slice, &tag) {
            Ok(())
        } else {
            Err(MacError)
        }
    }

    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError> {
        let binding: [u8; N] = self.finalize();
        let slice = binding.as_slice();
        let len = tag.len();
        if len == 0 || len > H::get_digest().block_size() {
            return Err(MacError);
        }
        let left = slice.get(..len).ok_or(MacError)?;
        if memcmp::eq(left, tag) {
            Ok(())
        } else {
            Err(MacError)
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::Openssl;
    use core::marker::PhantomData;
    use crypto_provider_test::hmac::*;

    #[apply(hmac_test_cases)]
    fn hmac_tests(testcase: CryptoProviderTestCase<Openssl>) {
        testcase(PhantomData);
    }
}
