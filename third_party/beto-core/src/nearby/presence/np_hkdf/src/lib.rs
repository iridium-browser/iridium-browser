// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Wrappers around NP's usage of HKDF.
//!
//! All HKDF calls should happen in this module and expose the correct result type for
//! each derived key use case.
#![no_std]
#![forbid(unsafe_code)]
#![deny(
    missing_docs,
    clippy::indexing_slicing,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used
)]

extern crate core;
#[cfg(feature = "std")]
extern crate std;

use core::marker;
use crypto_provider::{aes::Aes128Key, hkdf::Hkdf, hmac::Hmac, CryptoProvider};

pub mod v1_salt;

/// A wrapper around the common NP usage of HMAC-SHA256.
///
/// These are generally derived via HKDF, but could be used for any HMAC-SHA256 key.
#[derive(Debug)]
pub struct NpHmacSha256Key<C: CryptoProvider> {
    /// Nearby Presence uses 32-byte HMAC keys.
    ///
    /// Inside the HMAC algorithm they will be padded to 64 bytes.
    key: [u8; 32],
    c_phantom: marker::PhantomData<C>,
}

impl<C: CryptoProvider> NpHmacSha256Key<C> {
    /// Build a fresh HMAC instance.
    ///
    /// Since each HMAC is modified as data is fed to it, HMACs should not be reused.
    ///
    /// See also [Self::calculate_hmac] for simple use cases.
    pub fn build_hmac(&self) -> C::HmacSha256 {
        C::HmacSha256::new_from_key(self.key)
    }

    /// Returns a reference to the underlying key bytes.
    pub fn as_bytes(&self) -> &[u8; 32] {
        &self.key
    }

    /// Build an HMAC, update it with the provided `data`, and finalize it, returning the resulting
    /// MAC. This is convenient for one-and-done HMAC usage rather than incrementally accumulating
    /// the final MAC.
    pub fn calculate_hmac(&self, data: &[u8]) -> [u8; 32] {
        let mut hmac = self.build_hmac();
        hmac.update(data);
        hmac.finalize()
    }

    /// Build an HMAC, update it with the provided `data`, and verify it.
    ///
    /// This is convenient for one-and-done HMAC usage rather than incrementally accumulating
    /// the final MAC.
    pub fn verify_hmac(
        &self,
        data: &[u8],
        expected_mac: [u8; 32],
    ) -> Result<(), crypto_provider::hmac::MacError> {
        let mut hmac = self.build_hmac();
        hmac.update(data);
        hmac.verify(expected_mac)
    }
}

impl<C: CryptoProvider> From<[u8; 32]> for NpHmacSha256Key<C> {
    fn from(key: [u8; 32]) -> Self {
        Self { key, c_phantom: Default::default() }
    }
}

impl<C: CryptoProvider> Clone for NpHmacSha256Key<C> {
    fn clone(&self) -> Self {
        Self { key: self.key, c_phantom: Default::default() }
    }
}

/// Salt use for all NP HKDFs
const NP_HKDF_SALT: &[u8] = b"Google Nearby";

/// A wrapper around an NP key seed for deriving HKDF-SHA256 sub keys.
pub struct NpKeySeedHkdf<C: CryptoProvider> {
    hkdf: NpHkdf<C>,
}

impl<C: CryptoProvider> NpKeySeedHkdf<C> {
    /// Build an HKDF from a NP credential key seed
    pub fn new(key_seed: &[u8; 32]) -> Self {
        Self { hkdf: NpHkdf::new(key_seed) }
    }

    /// LDT key used to decrypt a legacy advertisement
    #[allow(clippy::expect_used)]
    pub fn legacy_ldt_key(&self) -> ldt::LdtKey<xts_aes::XtsAes128Key> {
        ldt::LdtKey::from_concatenated(
            &self.hkdf.derive_array(b"Legacy LDT key").expect("LDT key is a valid length"),
        )
    }

    /// HMAC key used when verifying the raw metadata key extracted from an advertisement
    pub fn legacy_metadata_key_hmac_key(&self) -> NpHmacSha256Key<C> {
        self.hkdf.derive_hmac_sha256_key(b"Legacy metadata key verification HMAC key")
    }

    /// AES-GCM nonce used when decrypting metadata
    #[allow(clippy::expect_used)]
    pub fn legacy_metadata_nonce(&self) -> [u8; 12] {
        self.hkdf.derive_array(b"Legacy Metadata Nonce").expect("Nonce is a valid length")
    }

    /// AES-GCM nonce used when decrypting metadata.
    ///
    /// Shared between signed and unsigned since they use the same credential.
    #[allow(clippy::expect_used)]
    pub fn extended_metadata_nonce(&self) -> [u8; 12] {
        self.hkdf.derive_array(b"Metadata Nonce").expect("Nonce is a valid length")
    }

    /// HMAC key used when verifying the raw metadata key extracted from an advertisement
    pub fn extended_unsigned_metadata_key_hmac_key(&self) -> NpHmacSha256Key<C> {
        self.hkdf.derive_hmac_sha256_key(b"Unsigned Section metadata key HMAC key")
    }

    /// HMAC key used when verifying the raw metadata key extracted from an extended signed advertisement
    #[allow(clippy::expect_used)]
    pub fn extended_signed_metadata_key_hmac_key(&self) -> NpHmacSha256Key<C> {
        self.hkdf.derive_hmac_sha256_key(b"Signed Section metadata key HMAC key")
    }

    /// AES128 key used when decrypting an extended signed section
    #[allow(clippy::expect_used)]
    pub fn extended_signed_section_aes_key(&self) -> Aes128Key {
        self.hkdf.derive_aes128_key(b"Signed Section AES key")
    }
}

impl<C: CryptoProvider> UnsignedSectionKeys<C> for NpKeySeedHkdf<C> {
    fn aes_key(&self) -> Aes128Key {
        self.hkdf.derive_aes128_key(b"Unsigned Section AES key")
    }

    fn hmac_key(&self) -> NpHmacSha256Key<C> {
        self.hkdf.derive_hmac_sha256_key(b"Unsigned Section HMAC key")
    }
}

/// Derived keys for V1 MIC (unsigned) sections
pub trait UnsignedSectionKeys<C: CryptoProvider> {
    /// AES128 key used when decrypting an extended unsigned section
    fn aes_key(&self) -> Aes128Key;

    /// HMAC-SHA256 key used when verifying an extended unsigned section
    fn hmac_key(&self) -> NpHmacSha256Key<C>;
}

/// Expand a legacy salt into the expanded salt used with XOR padding in LDT.
#[allow(clippy::expect_used)]
pub fn legacy_ldt_expanded_salt<const B: usize, C: CryptoProvider>(salt: &[u8; 2]) -> [u8; B] {
    simple_np_hkdf_expand::<B, C>(salt, b"Legacy LDT salt pad")
        // the padded salt is the tweak size of a tweakable block cipher, which shouldn't be
        // anywhere close to the max HKDF size (255 * 32)
        .expect("Tweak size is a valid HKDF size")
}

/// Expand a legacy (short) raw metadata key into an AES128 key.
#[allow(clippy::expect_used)]
pub fn legacy_metadata_expanded_key<C: CryptoProvider>(raw_metadata_key: &[u8; 14]) -> [u8; 16] {
    simple_np_hkdf_expand::<16, C>(raw_metadata_key, b"Legacy metadata key expansion")
        .expect("AES128 key is a valid HKDF size")
}

/// Build an HKDF using the NP HKDF salt, calculate output, and discard the HKDF.
/// If using the NP key seed as IKM, see [NpKeySeedHkdf] instead.
///
/// Returns None if the requested size is > 255 * 32 bytes.
fn simple_np_hkdf_expand<const N: usize, C: CryptoProvider>(
    ikm: &[u8],
    info: &[u8],
) -> Option<[u8; N]> {
    let mut buf = [0; N];
    let hkdf = np_salt_hkdf::<C>(ikm);
    hkdf.expand(info, &mut buf[..]).map(|_| buf).ok()
}

/// Construct an HKDF with the Nearby Presence salt and provided `ikm`
pub fn np_salt_hkdf<C: CryptoProvider>(ikm: &[u8]) -> C::HkdfSha256 {
    C::HkdfSha256::new(Some(NP_HKDF_SALT), ikm)
}

/// NP-flavored HKDF operations for common derived output types
pub struct NpHkdf<C: CryptoProvider> {
    hkdf: C::HkdfSha256,
}

impl<C: CryptoProvider> NpHkdf<C> {
    /// Build an HKDF using the NP HKDF salt and supplied `ikm`
    pub fn new(ikm: &[u8]) -> Self {
        Self { hkdf: np_salt_hkdf::<C>(ikm) }
    }

    /// Derive a length `N` array using the provided `info`
    /// Returns `None` if N > 255 * 32.
    pub fn derive_array<const N: usize>(&self, info: &[u8]) -> Option<[u8; N]> {
        let mut arr = [0_u8; N];
        self.hkdf.expand(info, &mut arr).map(|_| arr).ok()
    }

    /// Derive an HMAC-SHA256 key using the provided `info`
    #[allow(clippy::expect_used)]
    pub fn derive_hmac_sha256_key(&self, info: &[u8]) -> NpHmacSha256Key<C> {
        self.derive_array(info).expect("HMAC-SHA256 keys are a valid length").into()
    }
    /// Derive an AES-128 key using the provided `info`
    #[allow(clippy::expect_used)]
    pub fn derive_aes128_key(&self, info: &[u8]) -> Aes128Key {
        self.derive_array(info).expect("AES128 keys are a valid length").into()
    }
}
