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
#![no_std]
#![forbid(unsafe_code)]
#![deny(
    missing_docs,
    clippy::indexing_slicing,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used
)]

//! Crate which provides impls for CryptoProvider backed by BoringSSL.

use bssl_crypto::digest::{Sha256, Sha512};
use bssl_crypto::rand::rand_bytes;
use crypto_provider::{CryptoProvider, CryptoRng};
use crypto_provider_stubs::*;

/// Implementation of `crypto_provider::aes` types using BoringSSL.
pub mod aes;

/// Implementations of crypto_provider::hkdf traits backed by BoringSSL
pub mod hkdf;

/// Implementations of crypto_provider::hmac traits backed by BoringSSL
pub mod hmac;

/// Implementations of crypto_provider::ed25519 traits backed by BoringSSL
mod ed25519;

/// The BoringSSL backed struct which implements CryptoProvider
#[derive(Default, Clone, Debug, PartialEq, Eq)]
pub struct Boringssl;

impl CryptoProvider for Boringssl {
    type HkdfSha256 = hkdf::Hkdf<Sha256>;
    type HmacSha256 = hmac::HmacSha256;
    type HkdfSha512 = hkdf::Hkdf<Sha512>;
    type HmacSha512 = hmac::HmacSha512;
    type AesCbcPkcs7Padded = AesCbcPkcs7PaddedStubs;
    type X25519 = X25519Stubs;
    type P256 = P256Stubs;
    type Sha256 = Sha2Stubs;
    type Sha512 = Sha2Stubs;
    type Aes128 = aes::Aes128;
    type Aes256 = aes::Aes256;
    type AesCtr128 = Aes128Stubs;
    type AesCtr256 = Aes256Stubs;
    type Ed25519 = ed25519::Ed25519;
    type Aes128GcmSiv = Aes128Stubs;
    type Aes256GcmSiv = Aes256Stubs;
    type CryptoRng = BoringSslRng;

    fn constant_time_eq(_a: &[u8], _b: &[u8]) -> bool {
        unimplemented!()
    }
}

/// OpenSSL implemented random number generator
pub struct BoringSslRng;

impl CryptoRng for BoringSslRng {
    fn new() -> Self {
        BoringSslRng {}
    }

    fn next_u64(&mut self) -> u64 {
        let mut buf = [0; 8];
        rand_bytes(&mut buf);
        u64::from_be_bytes(buf)
    }

    fn fill(&mut self, dest: &mut [u8]) {
        rand_bytes(dest)
    }
}
