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

//! Cryptographic materials for v1 advertisement-format credentials.

use core::borrow::Borrow;
use crypto_provider::{aes::Aes128Key, ed25519, CryptoProvider};
use np_hkdf::UnsignedSectionKeys;

/// Cryptographic materials necessary for determining whether or not
/// a given V1 advertisement section matches an identity.
/// Per the construction of the V1 specification, this is also
/// the information necessary to decrypt the raw byte contents
/// of an encrypted V1 section.
#[derive(Clone)]
pub struct SectionIdentityResolutionMaterial {
    /// The AES key for decrypting section ciphertext
    pub(crate) aes_key: Aes128Key,
    /// The metadata key HMAC key for deriving and verifying the identity metadata
    /// key HMAC against the expected value.
    pub(crate) metadata_key_hmac_key: [u8; 32],
    /// The expected metadata key HMAC to check against for an identity match.
    pub(crate) expected_metadata_key_hmac: [u8; 32],
}

/// Cryptographic materials necessary for determining whether or not
/// a given V1 signed advertisement section matches an identity.
#[derive(Clone)]
pub struct SignedSectionIdentityResolutionMaterial(SectionIdentityResolutionMaterial);

impl SignedSectionIdentityResolutionMaterial {
    #[cfg(test)]
    pub(crate) fn from_raw(raw: SectionIdentityResolutionMaterial) -> Self {
        Self(raw)
    }
    pub(crate) fn as_raw_resolution_material(&self) -> &SectionIdentityResolutionMaterial {
        &self.0
    }
    pub(crate) fn from_hkdf_and_expected_metadata_key_hmac<C: CryptoProvider>(
        hkdf: &np_hkdf::NpKeySeedHkdf<C>,
        expected_metadata_key_hmac: [u8; 32],
    ) -> Self {
        Self(SectionIdentityResolutionMaterial {
            aes_key: hkdf.extended_signed_section_aes_key(),
            metadata_key_hmac_key: *hkdf.extended_signed_metadata_key_hmac_key().as_bytes(),
            expected_metadata_key_hmac,
        })
    }
}

/// Cryptographic materials necessary for determining whether or not
/// a given V1 MIC advertisement section matches an identity.
#[derive(Clone)]
pub struct UnsignedSectionIdentityResolutionMaterial(SectionIdentityResolutionMaterial);

impl UnsignedSectionIdentityResolutionMaterial {
    #[cfg(test)]
    pub(crate) fn from_raw(raw: SectionIdentityResolutionMaterial) -> Self {
        Self(raw)
    }
    pub(crate) fn as_raw_resolution_material(&self) -> &SectionIdentityResolutionMaterial {
        &self.0
    }
    pub(crate) fn from_hkdf_and_expected_metadata_key_hmac<C: CryptoProvider>(
        hkdf: &np_hkdf::NpKeySeedHkdf<C>,
        expected_metadata_key_hmac: [u8; 32],
    ) -> Self {
        Self(SectionIdentityResolutionMaterial {
            aes_key: UnsignedSectionKeys::aes_key(hkdf),
            metadata_key_hmac_key: *hkdf.extended_unsigned_metadata_key_hmac_key().as_bytes(),
            expected_metadata_key_hmac,
        })
    }
}

/// Crypto materials for V1 signed sections which are not employed in identity resolution,
/// but may be necessary to verify a signed section.
#[derive(Clone)]
pub struct SignedSectionVerificationMaterial {
    /// The np_ed25519 public key to be
    /// used for signature verification of signed sections.
    pub(crate) pub_key: ed25519::RawPublicKey,
}

impl SignedSectionVerificationMaterial {
    /// Gets the np_ed25519 public key for the given identity,
    /// used for signature verification of signed sections.
    pub(crate) fn signature_verification_public_key<C: CryptoProvider>(
        &self,
    ) -> np_ed25519::PublicKey<C> {
        np_ed25519::PublicKey::from_bytes(&self.pub_key).expect("Should only contain valid keys")
    }
}

/// Crypto materials for V1 unsigned sections which are not employed in identity resolution,
/// but may be necessary to fully decrypt an unsigned section.
#[derive(Clone)]
pub struct UnsignedSectionVerificationMaterial {
    /// The MIC HMAC key for verifying the integrity of unsigned sections.
    pub(crate) mic_hmac_key: [u8; 32],
}

impl UnsignedSectionVerificationMaterial {
    /// Returns the MIC HMAC key for unsigned sections
    pub(crate) fn mic_hmac_key<C: CryptoProvider>(&self) -> np_hkdf::NpHmacSha256Key<C> {
        self.mic_hmac_key.into()
    }
}

// Space-time tradeoffs:
// - Calculating an HKDF from the key seed costs about 2us on a gLinux laptop, and occupies 80b.
// - Calculating an AES (16b) or HMAC (32b) key from the HKDF costs about 700ns.
// The right tradeoff may also vary by use case. For frequently used identities we should
// probably pre-calculate everything. For occasionally used ones, or ones that are loaded from
// disk, used once, and discarded, we might want to precalculate on a separate thread or the
// like.
// The AES key and metadata key HMAC key are the most frequently used ones, as the MIC HMAC key
// is only used on the matching identity, not all identities.

/// Cryptographic material for an individual NP credential used to decrypt and verify v1 sections.
pub trait V1CryptoMaterial {
    /// The return type of `Self::signed_identity_resolution_material`, which is some
    /// data-type which allows borrowing `SignedSectionIdentityResolutionMaterial`
    type SignedIdentityResolverReference<'a>: Borrow<SignedSectionIdentityResolutionMaterial>
    where
        Self: 'a;
    /// The return type of `Self::unsigned_identity_resolution_material`, which is some
    /// data-type which allows borrowing `UnsignedSectionIdentityResolutionMaterial`
    type UnsignedIdentityResolverReference<'a>: Borrow<UnsignedSectionIdentityResolutionMaterial>
    where
        Self: 'a;

    /// Constructs or references the identity resolution material for signed sections
    fn signed_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::SignedIdentityResolverReference<'_>;

    /// Constructs or references the identity resolution material for unsigned sections
    fn unsigned_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::UnsignedIdentityResolverReference<'_>;

    /// Constructs or copies non-identity-resolution deserialization material for signed
    /// sections.
    ///
    /// Note: We mandate "copies" here due to the relatively small size of verification-only crypto
    /// materials (32 bytes).
    fn signed_verification_material<C: CryptoProvider>(&self) -> SignedSectionVerificationMaterial;

    /// Constructs or copies non-identity-resolution deserialization material for unsigned
    /// sections.
    ///
    /// Note: We mandate "copies" here due to the relatively small size of verification-only crypto
    /// materials (32 bytes).
    fn unsigned_verification_material<C: CryptoProvider>(
        &self,
    ) -> UnsignedSectionVerificationMaterial;

    /// Constructs pre-calculated crypto material out of this crypto-material.
    fn to_precalculated<C: CryptoProvider>(self) -> PrecalculatedV1CryptoMaterial
    where
        Self: Sized,
    {
        let signed_identity_resolution_material =
            self.signed_identity_resolution_material::<C>().borrow().clone();
        let unsigned_identity_resolution_material =
            self.unsigned_identity_resolution_material::<C>().borrow().clone();
        let signed_verification_material = self.signed_verification_material::<C>();
        let unsigned_verification_material = self.unsigned_verification_material::<C>();
        PrecalculatedV1CryptoMaterial {
            signed_identity_resolution_material,
            unsigned_identity_resolution_material,
            signed_verification_material,
            unsigned_verification_material,
        }
    }
}

/// [`V1CryptoMaterial`] that minimizes CPU time when providing key material at
/// the expense of occupied memory
pub struct PrecalculatedV1CryptoMaterial {
    pub(crate) signed_identity_resolution_material: SignedSectionIdentityResolutionMaterial,
    pub(crate) unsigned_identity_resolution_material: UnsignedSectionIdentityResolutionMaterial,
    pub(crate) signed_verification_material: SignedSectionVerificationMaterial,
    pub(crate) unsigned_verification_material: UnsignedSectionVerificationMaterial,
}

impl V1CryptoMaterial for PrecalculatedV1CryptoMaterial {
    type SignedIdentityResolverReference<'a> = &'a SignedSectionIdentityResolutionMaterial
        where Self: 'a;
    type UnsignedIdentityResolverReference<'a> = &'a UnsignedSectionIdentityResolutionMaterial
        where Self: 'a;

    fn signed_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::SignedIdentityResolverReference<'_> {
        &self.signed_identity_resolution_material
    }
    fn unsigned_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::UnsignedIdentityResolverReference<'_> {
        &self.unsigned_identity_resolution_material
    }
    fn signed_verification_material<C: CryptoProvider>(&self) -> SignedSectionVerificationMaterial {
        self.signed_verification_material.clone()
    }
    fn unsigned_verification_material<C: CryptoProvider>(
        &self,
    ) -> UnsignedSectionVerificationMaterial {
        self.unsigned_verification_material.clone()
    }
}

/// [`V1CryptoMaterial`] that minimizes memory footprint at the expense of CPU
/// time when providing derived key material.
pub struct MinimumFootprintV1CryptoMaterial {
    key_seed: [u8; 32],
    expected_unsigned_metadata_key_hmac: [u8; 32],
    expected_signed_metadata_key_hmac: [u8; 32],
    pub_key: ed25519::RawPublicKey,
}

impl MinimumFootprintV1CryptoMaterial {
    /// Construct an [MinimumFootprintV1CryptoMaterial] from the provided identity data.
    pub fn new<C: CryptoProvider>(
        key_seed: [u8; 32],
        expected_unsigned_metadata_key_hmac: [u8; 32],
        expected_signed_metadata_key_hmac: [u8; 32],
        pub_key: np_ed25519::PublicKey<C>,
    ) -> Self {
        Self {
            key_seed,
            expected_unsigned_metadata_key_hmac,
            expected_signed_metadata_key_hmac,
            pub_key: pub_key.to_bytes(),
        }
    }
}

impl V1CryptoMaterial for MinimumFootprintV1CryptoMaterial {
    type SignedIdentityResolverReference<'a> = SignedSectionIdentityResolutionMaterial
        where Self: 'a;
    type UnsignedIdentityResolverReference<'a> = UnsignedSectionIdentityResolutionMaterial
        where Self: 'a;

    fn signed_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::SignedIdentityResolverReference<'_> {
        let hkdf = np_hkdf::NpKeySeedHkdf::<C>::new(&self.key_seed);
        SignedSectionIdentityResolutionMaterial::from_hkdf_and_expected_metadata_key_hmac(
            &hkdf,
            self.expected_signed_metadata_key_hmac,
        )
    }

    fn unsigned_identity_resolution_material<C: CryptoProvider>(
        &self,
    ) -> Self::UnsignedIdentityResolverReference<'_> {
        let hkdf = np_hkdf::NpKeySeedHkdf::<C>::new(&self.key_seed);
        UnsignedSectionIdentityResolutionMaterial::from_hkdf_and_expected_metadata_key_hmac(
            &hkdf,
            self.expected_unsigned_metadata_key_hmac,
        )
    }

    fn signed_verification_material<C: CryptoProvider>(&self) -> SignedSectionVerificationMaterial {
        SignedSectionVerificationMaterial { pub_key: self.pub_key }
    }

    fn unsigned_verification_material<C: CryptoProvider>(
        &self,
    ) -> UnsignedSectionVerificationMaterial {
        let hkdf = np_hkdf::NpKeySeedHkdf::<C>::new(&self.key_seed);
        let mic_hmac_key = *UnsignedSectionKeys::hmac_key(&hkdf).as_bytes();
        UnsignedSectionVerificationMaterial { mic_hmac_key }
    }
}
