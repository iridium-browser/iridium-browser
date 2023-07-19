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
// limitations under the License.'
#![no_std]
#![forbid(unsafe_code)]
#![deny(missing_docs)]

//! Crypto abstraction trait only crate, which provides traits for cryptographic primitives

use core::fmt::Debug;

use crate::aes::{Aes128Key, Aes256Key};

/// mod containing hmac trait
pub mod hkdf;

/// mod containing hkdf trait
pub mod hmac;

/// mod containing X25519 trait
pub mod x25519;

/// mod containing traits for NIST-P256 elliptic curve implementation.
pub mod p256;

/// mod containing traits for elliptic curve cryptography.
pub mod elliptic_curve;

/// mod containing SHA256 trait.
pub mod sha2;

/// mod containing aes trait.
pub mod aes;

/// mod containing aead trait.
pub mod aead;

/// mod containing traits for ed25519 key generation, signing, and verification
pub mod ed25519;

/// Uber crypto trait which defines the traits for all crypto primitives as associated types
pub trait CryptoProvider: Clone + Debug + PartialEq + Eq + Send {
    /// The Hkdf type which implements the hkdf trait
    type HkdfSha256: hkdf::Hkdf;
    /// The Hmac type which implements the hmac trait
    type HmacSha256: hmac::Hmac<32>;
    /// The Hkdf type which implements the hkdf trait
    type HkdfSha512: hkdf::Hkdf;
    /// The Hmac type which implements the hmac trait
    type HmacSha512: hmac::Hmac<64>;
    /// The AES-CBC-PKCS7 implementation to use
    type AesCbcPkcs7Padded: aes::cbc::AesCbcPkcs7Padded;
    /// The X25519 implementation to use for ECDH.
    type X25519: elliptic_curve::EcdhProvider<x25519::X25519>;
    /// The P256 implementation to use for ECDH.
    type P256: p256::P256EcdhProvider;
    /// The SHA256 hash implementation.
    type Sha256: sha2::Sha256;
    /// The SHA512 hash implementation.
    type Sha512: sha2::Sha512;
    /// Plain AES-128 implementation (without block cipher mode).
    type Aes128: aes::Aes<Key = Aes128Key>;
    /// Plain AES-256 implementation (without block cipher mode).
    type Aes256: aes::Aes<Key = Aes256Key>;
    /// AES-128 with CTR block mode
    type AesCtr128: aes::ctr::AesCtr<Key = aes::Aes128Key>;
    /// AES-256 with CTR block mode
    type AesCtr256: aes::ctr::AesCtr<Key = aes::Aes256Key>;
    /// The trait defining ed25519, a Edwards-curve Digital Signature Algorithm signature scheme
    /// using SHA-512 (SHA-2) and Curve25519
    type Ed25519: ed25519::Ed25519Provider;
    /// The trait defining AES-128-GCM-SIV, a nonce-misuse resistant AEAD with a key size of 16 bytes.
    type Aes128GcmSiv: aead::aes_gcm_siv::AesGcmSiv<Key = Aes128Key>;
    /// The trait defining AES-256-GCM-SIV, a nonce-misuse resistant AEAD with a key size of 32 bytes.
    type Aes256GcmSiv: aead::aes_gcm_siv::AesGcmSiv<Key = Aes256Key>;

    /// The cryptographically secure random number generator
    type CryptoRng: CryptoRng;

    /// Compares the two given slices, in constant time, and returns true if they are equal.
    fn constant_time_eq(a: &[u8], b: &[u8]) -> bool;
}

/// Wrapper to a cryptographically secure pseudo random number generator
pub trait CryptoRng {
    /// Returns an instance of the rng
    fn new() -> Self;

    /// Return the next random u64
    fn next_u64(&mut self) -> u64;

    /// Fill dest with random data
    fn fill(&mut self, dest: &mut [u8]);

    /// Generate a random entity
    fn gen<F: FromCryptoRng>(&mut self) -> F
    where
        Self: Sized,
    {
        F::new_random::<Self>(self)
    }
}

/// If impls want to opt out of passing a Rng they can simply use `()` for the Rng associated type
impl CryptoRng for () {
    fn new() -> Self {}

    fn next_u64(&mut self) -> u64 {
        unimplemented!()
    }

    fn fill(&mut self, _dest: &mut [u8]) {
        unimplemented!()
    }
}

/// For types that can be created from a `CryptoRng`
pub trait FromCryptoRng {
    /// Construct a new `Self` with random data from `rng`
    fn new_random<R: CryptoRng>(rng: &mut R) -> Self;
}

impl<const N: usize> FromCryptoRng for [u8; N] {
    fn new_random<R: CryptoRng>(rng: &mut R) -> Self {
        let mut arr = [0; N];
        rng.fill(&mut arr);
        arr
    }
}

impl FromCryptoRng for u8 {
    fn new_random<R: CryptoRng>(rng: &mut R) -> Self {
        let arr: [u8; 1] = rng.gen();
        arr[0]
    }
}
