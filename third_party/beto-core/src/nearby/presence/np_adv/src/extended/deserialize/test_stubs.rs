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

extern crate std;

use crypto_provider::{ed25519, CryptoProvider};
use np_hkdf::{NpKeySeedHkdf, UnsignedSectionKeys};
use std::prelude::rust_2021::*;

use crate::{
    credential::v1::*,
    extended::deserialize::{CiphertextSection, PlaintextSection},
    IntermediateSection,
};

pub(crate) struct HkdfCryptoMaterial {
    pub(crate) hkdf: [u8; 32],
    pub(crate) expected_unsigned_metadata_key_hmac: [u8; 32],
    pub(crate) expected_signed_metadata_key_hmac: [u8; 32],
    pub(crate) pub_key: ed25519::RawPublicKey,
}

impl HkdfCryptoMaterial {
    pub(crate) fn new<C: CryptoProvider>(
        hkdf_key_seed: &[u8; 32],
        metadata_key: &[u8; 16],
        pub_key: np_ed25519::PublicKey<C>,
    ) -> Self {
        let hkdf = NpKeySeedHkdf::<C>::new(hkdf_key_seed);
        let unsigned =
            hkdf.extended_unsigned_metadata_key_hmac_key().calculate_hmac(metadata_key.as_slice());
        let signed =
            hkdf.extended_signed_metadata_key_hmac_key().calculate_hmac(metadata_key.as_slice());
        Self {
            hkdf: *hkdf_key_seed,
            expected_unsigned_metadata_key_hmac: unsigned,
            expected_signed_metadata_key_hmac: signed,
            pub_key: pub_key.to_bytes(),
        }
    }
}

impl HkdfCryptoMaterial {
    fn hkdf<C: CryptoProvider>(&self) -> NpKeySeedHkdf<C> {
        NpKeySeedHkdf::<C>::new(&self.hkdf)
    }
}

impl V1CryptoMaterial for HkdfCryptoMaterial {
    type SignedIdentityResolverReference<'a> = SignedSectionIdentityResolutionMaterial
        where Self: 'a;
    type UnsignedIdentityResolverReference<'a> = UnsignedSectionIdentityResolutionMaterial
        where Self: 'a;

    fn signed_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::SignedIdentityResolverReference<'_> {
        SignedSectionIdentityResolutionMaterial::from_hkdf_and_expected_metadata_key_hmac::<C>(
            &self.hkdf::<C>(),
            self.expected_signed_metadata_key_hmac,
        )
    }
    fn unsigned_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::UnsignedIdentityResolverReference<'_> {
        UnsignedSectionIdentityResolutionMaterial::from_hkdf_and_expected_metadata_key_hmac::<C>(
            &self.hkdf::<C>(),
            self.expected_unsigned_metadata_key_hmac,
        )
    }
    fn signed_verification_material<C: CryptoProvider>(&self) -> SignedSectionVerificationMaterial {
        SignedSectionVerificationMaterial { pub_key: self.pub_key }
    }

    fn unsigned_verification_material<C: CryptoProvider>(
        &self,
    ) -> UnsignedSectionVerificationMaterial {
        let mic_hmac_key = *UnsignedSectionKeys::hmac_key(&self.hkdf::<C>()).as_bytes();
        UnsignedSectionVerificationMaterial { mic_hmac_key }
    }
}

pub(crate) trait IntermediateSectionExt {
    /// Returns `Some` if `self` is `Plaintext`
    fn as_plaintext(&self) -> Option<&PlaintextSection>;
    /// Returns `Some` if `self` is `Ciphertext`
    fn as_ciphertext(&self) -> Option<&CiphertextSection>;
}

impl<'a> IntermediateSectionExt for IntermediateSection<'a> {
    fn as_plaintext(&self) -> Option<&PlaintextSection> {
        match self {
            IntermediateSection::Plaintext(s) => Some(s),
            IntermediateSection::Ciphertext(_) => None,
        }
    }

    fn as_ciphertext(&self) -> Option<&CiphertextSection> {
        match self {
            IntermediateSection::Plaintext(_) => None,
            IntermediateSection::Ciphertext(s) => Some(s),
        }
    }
}
