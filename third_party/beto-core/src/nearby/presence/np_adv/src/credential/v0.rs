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

//! Cryptographic materials for v0 advertisement-format credentials.

use super::*;

/// Cryptographic material for an individual NP credential used to decrypt v0 advertisements.
// Space-time tradeoffs:
// - LDT keys (64b) take about 1.4us.
pub trait V0CryptoMaterial {
    /// Returns an LDT NP advertisement cipher built with the provided `Aes`
    fn ldt_adv_cipher<C: CryptoProvider>(&self) -> ldt_np_adv::LdtNpAdvDecrypterXtsAes128<C>;
}

/// A [`V0CryptoMaterial`] that minimizes memory footprint at the expense of CPU time when
/// providing derived key material
pub struct MinimumFootprintV0CryptoMaterial {
    key_seed: [u8; 32],
    legacy_metadata_key_hmac: [u8; 32],
}

impl MinimumFootprintV0CryptoMaterial {
    /// Construct an [MinimumFootprintV0CryptoMaterial] from the provided identity data.
    pub fn new(key_seed: [u8; 32], legacy_metadata_key_hmac: [u8; 32]) -> Self {
        Self { key_seed, legacy_metadata_key_hmac }
    }
}

impl V0CryptoMaterial for MinimumFootprintV0CryptoMaterial {
    fn ldt_adv_cipher<C: CryptoProvider>(&self) -> ldt_np_adv::LdtNpAdvDecrypterXtsAes128<C> {
        let hkdf = np_hkdf::NpKeySeedHkdf::new(&self.key_seed);
        ldt_np_adv::build_np_adv_decrypter(
            &hkdf.legacy_ldt_key(),
            self.legacy_metadata_key_hmac,
            hkdf.legacy_metadata_key_hmac_key(),
        )
    }
}

/// [`V0CryptoMaterial`] that minimizes CPU time when providing key material at
/// the expense of occupied memory.
pub struct PrecalculatedV0CryptoMaterial {
    pub(crate) legacy_ldt_key: ldt::LdtKey<xts_aes::XtsAes128Key>,
    pub(crate) legacy_metadata_key_hmac: [u8; 32],
    pub(crate) legacy_metadata_key_hmac_key: [u8; 32],
}

impl PrecalculatedV0CryptoMaterial {
    /// Construct a new instance from the provided credential material.
    pub fn new<C: CryptoProvider>(key_seed: &[u8; 32], legacy_metadata_key_hmac: [u8; 32]) -> Self {
        let hkdf = np_hkdf::NpKeySeedHkdf::<C>::new(key_seed);
        Self {
            legacy_ldt_key: hkdf.legacy_ldt_key(),
            legacy_metadata_key_hmac,
            legacy_metadata_key_hmac_key: *hkdf.legacy_metadata_key_hmac_key().as_bytes(),
        }
    }
}

impl V0CryptoMaterial for PrecalculatedV0CryptoMaterial {
    fn ldt_adv_cipher<C: CryptoProvider>(&self) -> ldt_np_adv::LdtNpAdvDecrypterXtsAes128<C> {
        ldt_np_adv::build_np_adv_decrypter(
            &self.legacy_ldt_key,
            self.legacy_metadata_key_hmac,
            self.legacy_metadata_key_hmac_key.into(),
        )
    }
}
