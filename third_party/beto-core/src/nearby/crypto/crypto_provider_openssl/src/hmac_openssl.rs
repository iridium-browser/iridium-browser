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

use crate::{sha2::OpenSslSha256, sha2::OpenSslSha512, OpenSslHash};
use core::convert::TryInto;
use crypto_provider::hmac::{InvalidLength, MacError};
use openssl::memcmp;
use openssl::pkey::{PKey, Private};
use openssl::sign::Signer;
use ouroboros::self_referencing;
use std::marker::PhantomData;

/// openssl based hmac implementation
#[self_referencing]
pub struct Hmac<H: OpenSslHash> {
    ctx: PKey<Private>,
    #[borrows(ctx)]
    #[covariant]
    signer: Signer<'this>,
    marker: PhantomData<H>,
}

impl crypto_provider::hmac::Hmac<32> for Hmac<OpenSslSha256> {
    fn new_from_key(key: [u8; 32]) -> Self {
        new_from_key(key)
    }

    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength> {
        new_from_slice(key)
    }

    fn update(&mut self, data: &[u8]) {
        update(self, data)
    }

    fn finalize(self) -> [u8; 32] {
        finalize(self)
    }

    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError> {
        verify_slice(self, tag)
    }

    fn verify(self, tag: [u8; 32]) -> Result<(), MacError> {
        verify(self, tag)
    }

    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError> {
        verify_truncated_left(self, tag)
    }
}

impl crypto_provider::hmac::Hmac<64> for Hmac<OpenSslSha512> {
    fn new_from_key(key: [u8; 64]) -> Self {
        new_from_key(key)
    }

    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength> {
        new_from_slice(key)
    }

    fn update(&mut self, data: &[u8]) {
        update(self, data)
    }

    fn finalize(self) -> [u8; 64] {
        finalize(self)
    }

    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError> {
        verify_slice(self, tag)
    }

    fn verify(self, tag: [u8; 64]) -> Result<(), MacError> {
        verify(self, tag)
    }

    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError> {
        verify_truncated_left(self, tag)
    }
}

fn hmac_from_builder<H: OpenSslHash>(ctx: PKey<Private>) -> Hmac<H> {
    let digest = H::get_digest();
    HmacBuilder {
        ctx,
        marker: PhantomData,
        signer_builder: |ctx: &openssl::pkey::PKey<openssl::pkey::Private>| {
            Signer::new(digest, ctx).expect("should be able to create signer")
        },
    }
    .build()
}

fn new_from_key<const N: usize, H: OpenSslHash>(key: [u8; N]) -> Hmac<H> {
    let ctx = openssl::pkey::PKey::hmac(&key)
        .expect("hmac creation shouldn't fail since key is of valid size");
    hmac_from_builder(ctx)
}

fn new_from_slice<H: OpenSslHash>(key: &[u8]) -> Result<Hmac<H>, InvalidLength> {
    openssl::pkey::PKey::hmac(key).map(hmac_from_builder).map_err(|_| InvalidLength)
}

fn update<H: OpenSslHash>(hmac: &mut Hmac<H>, data: &[u8]) {
    hmac.with_signer_mut(|signer| {
        signer.update(data).expect("should be able to update signer");
    })
}

fn finalize<const N: usize, H: OpenSslHash>(hmac: Hmac<H>) -> [u8; N] {
    hmac.borrow_signer()
        .sign_to_vec()
        .expect("sign to vec should succeed")
        .as_slice()
        .try_into()
        .expect("wrong length")
}

fn verify_slice<H: OpenSslHash>(hmac: Hmac<H>, tag: &[u8]) -> Result<(), MacError> {
    let binding = hmac.borrow_signer().sign_to_vec().expect("sign to vec should succeed");
    let slice = binding.as_slice();
    if memcmp::eq(slice, tag) {
        Ok(())
    } else {
        Err(MacError)
    }
}

fn verify<const N: usize, H: OpenSslHash>(hmac: Hmac<H>, tag: [u8; N]) -> Result<(), MacError> {
    let binding = hmac.borrow_signer().sign_to_vec().expect("sign to vec should succeed");
    let slice = binding.as_slice();
    if memcmp::eq(slice, &tag) {
        Ok(())
    } else {
        Err(MacError)
    }
}

fn verify_truncated_left<H: OpenSslHash>(hmac: Hmac<H>, tag: &[u8]) -> Result<(), MacError> {
    let binding = hmac.borrow_signer().sign_to_vec().expect("sign to vec should succeed");
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
