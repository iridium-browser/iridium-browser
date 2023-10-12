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

//! Crate which provides impls for CryptoProvider backed by RustCrypto crates

use core::{fmt::Debug, marker::PhantomData};

use cfg_if::cfg_if;
pub use hkdf;
pub use hmac;
use rand::{Rng, RngCore, SeedableRng};
use rand_core::CryptoRng;
use subtle::ConstantTimeEq;

/// Contains the RustCrypto backed impls for AES-GCM-SIV operations
mod aead;
/// Contains the RustCrypto backed AES impl for CryptoProvider
pub mod aes;
/// Contains the RustCrypto backed impl for ed25519 key generation, signing, and verification
mod ed25519;
/// Contains the RustCrypto backed hkdf impl for CryptoProvider
mod hkdf_rc;
/// Contains the RustCrypto backed hmac impl for CryptoProvider
mod hmac_rc;
/// Contains the RustCrypto backed P256 impl for CryptoProvider
mod p256;
/// Contains the RustCrypto backed SHA2 impl for CryptoProvider
mod sha2_rc;
/// Contains the RustCrypto backed X25519 impl for CryptoProvider
mod x25519;

cfg_if! {
    if #[cfg(feature = "std")] {
        /// Providing a type alias for compatibility with existing usage of RustCrypto
        /// by default we use StdRng for the underlying csprng
        pub type RustCrypto = RustCryptoImpl<rand::rngs::StdRng>;
    } else {
        /// A no_std compatible implementation of CryptoProvider backed by RustCrypto crates
        pub type RustCrypto = RustCryptoImpl<rand_chacha::ChaCha20Rng>;
    }
}

/// The the RustCrypto backed struct which implements CryptoProvider
#[derive(Default, Clone, Debug, PartialEq, Eq)]
pub struct RustCryptoImpl<R: CryptoRng + SeedableRng + RngCore> {
    _marker: PhantomData<R>,
}

impl<R: CryptoRng + SeedableRng + RngCore> RustCryptoImpl<R> {
    /// Create a new instance of RustCrypto
    pub fn new() -> Self {
        Self { _marker: Default::default() }
    }
}

impl<R: CryptoRng + SeedableRng + RngCore + Eq + PartialEq + Debug + Clone + Send>
    crypto_provider::CryptoProvider for RustCryptoImpl<R>
{
    type HkdfSha256 = hkdf_rc::Hkdf<sha2::Sha256>;
    type HmacSha256 = hmac_rc::Hmac<sha2::Sha256>;
    type HkdfSha512 = hkdf_rc::Hkdf<sha2::Sha512>;
    type HmacSha512 = hmac_rc::Hmac<sha2::Sha512>;
    #[cfg(feature = "alloc")]
    type AesCbcPkcs7Padded = aes::cbc::AesCbcPkcs7Padded;
    type X25519 = x25519::X25519Ecdh<R>;
    type P256 = p256::P256Ecdh<R>;
    type Sha256 = sha2_rc::RustCryptoSha256;
    type Sha512 = sha2_rc::RustCryptoSha512;
    type Aes128 = aes::Aes128;
    type Aes256 = aes::Aes256;
    type AesCtr128 = aes::ctr::AesCtr128;
    type AesCtr256 = aes::ctr::AesCtr256;
    type Ed25519 = ed25519::Ed25519;
    type Aes128GcmSiv = aead::aes_gcm_siv::AesGcmSiv128;
    type Aes256GcmSiv = aead::aes_gcm_siv::AesGcmSiv256;
    type CryptoRng = RcRng<R>;

    fn constant_time_eq(a: &[u8], b: &[u8]) -> bool {
        a.ct_eq(b).into()
    }
}

/// A RustCrypto wrapper for RNG
pub struct RcRng<R>(R);

impl<R: rand_core::CryptoRng + RngCore + SeedableRng> crypto_provider::CryptoRng for RcRng<R> {
    fn new() -> Self {
        Self(R::from_entropy())
    }

    fn next_u64(&mut self) -> u64 {
        self.0.next_u64()
    }

    fn fill(&mut self, dest: &mut [u8]) {
        self.0.fill(dest)
    }
}

#[cfg(test)]
mod testing;

#[cfg(test)]
mod tests {
    use core::marker::PhantomData;

    use crypto_provider_test::sha2::*;

    use crate::RustCrypto;

    #[apply(sha2_test_cases)]
    fn sha2_tests(testcase: CryptoProviderTestCase<RustCrypto>) {
        testcase(PhantomData::<RustCrypto>);
    }
}
