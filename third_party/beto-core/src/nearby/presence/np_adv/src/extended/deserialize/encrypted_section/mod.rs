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

use crate::{
    credential::v1::*,
    extended::{
        deserialize::{
            parse_non_identity_des, DecryptedSection, EncryptedIdentityMetadata, EncryptionInfo,
            SectionContents, SectionMic, VerificationMode,
        },
        section_signature_payload::*,
        to_array_view, METADATA_KEY_LEN, NP_ADV_MAX_SECTION_LEN,
    },
    V1Header, NP_SVC_UUID,
};
use array_view::ArrayView;
use crypto_provider::{
    aes::ctr::{AesCtr, AesCtrNonce, NonceAndCounter, AES_CTR_NONCE_LEN},
    hmac::Hmac,
    CryptoProvider,
};
use np_hkdf::v1_salt::V1Salt;

#[cfg(test)]
mod mic_decrypt_tests;
#[cfg(test)]
mod signature_decrypt_tests;

#[derive(PartialEq, Eq, Debug)]
pub(crate) struct SignatureEncryptedSection<'a> {
    pub(crate) section_header: u8,
    pub(crate) adv_header: &'a V1Header,
    pub(crate) encryption_info: EncryptionInfo,
    pub(crate) identity: EncryptedIdentityMetadata,
    /// All ciphertext (Contents of identity DE + all DEs)
    /// Length must be in `[METADATA_KEY_LEN, NP_ADV_MAX_SECTION_LEN]`.
    pub(crate) all_ciphertext: &'a [u8],
}

impl<'a> SignatureEncryptedSection<'a> {
    /// Decrypt the ciphertext and check that the first [METADATA_KEY_LEN] bytes match the expected HMAC.
    /// The remaining data is not verified.
    ///
    /// Returns `Ok` if the section was an identity match.
    /// Otherwise, returns `Err`.
    pub(crate) fn try_decrypt<P: CryptoProvider>(
        &self,
        identity_resolution_material: &SignedSectionIdentityResolutionMaterial,
    ) -> Result<ArrayView<u8, NP_ADV_MAX_SECTION_LEN>, SignatureEncryptedSectionDeserializationError>
    {
        let salt: V1Salt<P> = self.encryption_info.salt().into();
        let (mut buf, mut cipher) = try_decrypt_metadata_key_prefix::<P>(
            self.all_ciphertext,
            salt.derive(Some(self.identity.offset)).expect("AES-CTR nonce is a valid HKDF size"),
            identity_resolution_material.as_raw_resolution_material(),
        )
        .ok_or(SignatureEncryptedSectionDeserializationError::MetadataKeyMacMismatch)?;

        // decrypt everything else
        cipher.decrypt(&mut buf[METADATA_KEY_LEN..]);

        Ok(to_array_view(buf))
    }

    /// Try deserializing into a [Section].
    ///
    /// Returns an error if the credential is incorrect or if the section data is malformed.
    pub(crate) fn try_deserialize<P>(
        &self,
        identity_resolution_material: &SignedSectionIdentityResolutionMaterial,
        verification_material: &SignedSectionVerificationMaterial,
    ) -> Result<DecryptedSection, SignatureEncryptedSectionDeserializationError>
    where
        P: CryptoProvider,
    {
        let raw_salt = self.encryption_info.salt();
        let salt: V1Salt<P> = raw_salt.into();
        let plaintext = self.try_decrypt::<P>(identity_resolution_material)?;

        // won't panic: plaintext length is at least METADATA_KEY_LEN
        let (metadata_key, remaining) = plaintext.as_slice().split_at(METADATA_KEY_LEN);
        let metadata_key =
            metadata_key.try_into().expect("slice is the same length as the desired array");

        if remaining.len() < crypto_provider::ed25519::SIGNATURE_LENGTH {
            return Err(SignatureEncryptedSectionDeserializationError::SignatureMissing);
        }

        // should not panic due to above check
        let (non_identity_des, sig) =
            remaining.split_at(remaining.len() - crypto_provider::ed25519::SIGNATURE_LENGTH);

        // de offset 2 because of leading encryption info and identity DEs
        let (_, ref_des) = parse_non_identity_des(2)(non_identity_des)
            .map_err(|_| SignatureEncryptedSectionDeserializationError::DeParseError)?;

        // All implementations only check for 64 bytes, and this will always result in a 64 byte signature.
        let expected_signature =
            np_ed25519::Signature::<P>::try_from(sig).expect("Signature is always 64 bytes.");

        let nonce = salt
            .derive::<{ AES_CTR_NONCE_LEN }>(Some(self.identity.offset))
            .expect("AES-CTR nonce is a valid HKDF length");

        // all other plaintext, except for the signature.
        // Won't panic because we know we just parsed the sig de from this,
        // and we're using the as-parsed lengths
        let remainder = &plaintext.as_slice()[..plaintext.len() - sig.len()];

        let section_signature_payload = SectionSignaturePayload::from_deserialized_parts(
            self.adv_header.header_byte,
            self.section_header,
            &self.encryption_info.bytes,
            &nonce,
            self.identity.header_bytes,
            remainder,
        );

        let public_key = verification_material.signature_verification_public_key();

        section_signature_payload.verify(&expected_signature, &public_key).map_err(|e| {
            // Length of the payload should fit in the signature verification buffer.
            debug_assert!(e != np_ed25519::SignatureVerificationError::PayloadTooBig);

            SignatureEncryptedSectionDeserializationError::SignatureMismatch
        })?;

        Ok(DecryptedSection::new(
            self.identity.identity_type,
            VerificationMode::Signature,
            metadata_key,
            raw_salt,
            SectionContents::new(self.section_header, &ref_des),
        ))
    }
}

/// Should not be exposed publicly as it's too detailed.
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum SignatureEncryptedSectionDeserializationError {
    /// The decrypted metadata key did not have the right MAC
    MetadataKeyMacMismatch,
    /// The provided signature did not match the calculated signature
    SignatureMismatch,
    /// The provided signature is missing
    SignatureMissing,
    /// Decrypted DEs could not be parsed, possibly because the ciphertext was tampered with, as
    /// DE parsing has to be done before the signature is verified
    DeParseError,
}

#[derive(PartialEq, Eq, Debug)]
pub(crate) struct MicEncryptedSection<'a> {
    pub(crate) section_header: u8,
    pub(crate) adv_header: &'a V1Header,
    pub(crate) encryption_info: EncryptionInfo,
    pub(crate) mic: SectionMic,
    pub(crate) identity: EncryptedIdentityMetadata,
    /// All ciphertext (Contents of identity DE + all DEs).
    /// Length must be in `[METADATA_KEY_LEN, NP_ADV_MAX_SECTION_LEN]`.
    pub(crate) all_ciphertext: &'a [u8],
}

impl<'a> MicEncryptedSection<'a> {
    /// Attempt to decrypt and verify the encrypted portion of the section with the provided
    /// precalculated cryptographic material for an MIC-encrypted section.
    ///
    /// If successful, returns a buffer containing the decrypted plaintext.
    pub(crate) fn try_decrypt<P: CryptoProvider>(
        &self,
        identity_resolution_material: &UnsignedSectionIdentityResolutionMaterial,
        verification_material: &UnsignedSectionVerificationMaterial,
    ) -> Result<ArrayView<u8, NP_ADV_MAX_SECTION_LEN>, MicEncryptedSectionDeserializationError>
    {
        let salt: V1Salt<P> = self.encryption_info.salt().into();
        let nonce =
            salt.derive(Some(self.identity.offset)).expect("AES-CTR nonce is a valid HKDF size");

        let (mut buf, mut cipher) = try_decrypt_metadata_key_prefix::<P>(
            self.all_ciphertext,
            nonce,
            identity_resolution_material.as_raw_resolution_material(),
        )
        .ok_or(MicEncryptedSectionDeserializationError::MetadataKeyMacMismatch)?;

        // if mic is ok, the section was generated by someone holding at least the shared credential
        let mut mic_hmac = verification_material.mic_hmac_key::<P>().build_hmac();
        mic_hmac.update(&NP_SVC_UUID);
        mic_hmac.update(&[self.adv_header.header_byte]);
        mic_hmac.update(&[self.section_header]);
        mic_hmac.update(&self.encryption_info.bytes);
        mic_hmac.update(&nonce);
        mic_hmac.update(&self.identity.header_bytes);
        mic_hmac.update(self.all_ciphertext);
        mic_hmac
            // adv only contains first 16 bytes of HMAC
            .verify_truncated_left(&self.mic.mic)
            .map_err(|_e| MicEncryptedSectionDeserializationError::MicMismatch)?;

        // decrypt everything else
        cipher.decrypt(&mut buf[METADATA_KEY_LEN..]);

        Ok(to_array_view(buf))
    }

    /// Try deserializing into a [Section].
    ///
    /// Returns an error if the credential is incorrect or if the section data is malformed.
    pub(crate) fn try_deserialize<P>(
        &self,
        identity_resolution_material: &UnsignedSectionIdentityResolutionMaterial,
        verification_material: &UnsignedSectionVerificationMaterial,
    ) -> Result<DecryptedSection, MicEncryptedSectionDeserializationError>
    where
        P: CryptoProvider,
    {
        let plaintext =
            self.try_decrypt::<P>(identity_resolution_material, verification_material)?;
        // won't panic: plaintext is at least METADATA_KEY_LEN long
        let (metadata_key, remaining_des) = plaintext.as_slice().split_at(METADATA_KEY_LEN);
        let metadata_key =
            metadata_key.try_into().expect("slice is the same length as the desired array");
        // offset 2 for encryption info and identity DEs
        let (_, ref_des) = parse_non_identity_des(2)(remaining_des)
            .map_err(|_| MicEncryptedSectionDeserializationError::DeParseError)?;

        Ok(DecryptedSection::new(
            self.identity.identity_type,
            VerificationMode::Mic,
            metadata_key,
            self.encryption_info.salt(),
            SectionContents::new(self.section_header, &ref_des),
        ))
    }
}

/// Should not be exposed publicly as it's too detailed.
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum MicEncryptedSectionDeserializationError {
    /// Plaintext metadata key did not have the expected MAC
    MetadataKeyMacMismatch,
    /// Calculated MIC did not match MIC from section
    MicMismatch,
    /// Parsing of decrypted DEs failed
    DeParseError,
}

/// Decrypt a ciphertext buffer whose first [METADATA_KEY_LEN] bytes of plaintext are, maybe, the
/// metadata key for an NP identity.
///
/// Returns `Some` if decrypting the first [METADATA_KEY_LEN] bytes produces plaintext whose HMAC
/// matches the expected MAC. Only the first [METADATA_KEY_LEN] bytes are decrypted. The cipher is
/// returned along with the buffer as it is in the appropriate state to continue decrypting the
/// remaining ciphertext.
///
/// Returns `None` if the plaintext does not match the expected HMAC.
///
/// # Panics
///
/// Panics if `ciphertext`'s length is not in `[METADATA_KEY_LEN, NP_ADV_MAX_SECTION_LEN]`.
#[allow(clippy::type_complexity)]
fn try_decrypt_metadata_key_prefix<C: CryptoProvider>(
    ciphertext: &[u8],
    nonce: AesCtrNonce,
    identity_resolution_material: &SectionIdentityResolutionMaterial,
) -> Option<(tinyvec::ArrayVec<[u8; NP_ADV_MAX_SECTION_LEN]>, C::AesCtr128)> {
    assert!(ciphertext.len() <= NP_ADV_MAX_SECTION_LEN);
    let mut decrypt_buf = tinyvec::ArrayVec::<[u8; NP_ADV_MAX_SECTION_LEN]>::new();
    let aes_key = &identity_resolution_material.aes_key;
    let mut cipher = C::AesCtr128::new(aes_key, NonceAndCounter::from_nonce(nonce));

    decrypt_buf.extend_from_slice(ciphertext);
    cipher.decrypt(&mut decrypt_buf[..METADATA_KEY_LEN]);

    let metadata_key_hmac_key: np_hkdf::NpHmacSha256Key<C> =
        identity_resolution_material.metadata_key_hmac_key.into();
    let expected_metadata_key_hmac = identity_resolution_material.expected_metadata_key_hmac;
    metadata_key_hmac_key
        .verify_hmac(&decrypt_buf[..METADATA_KEY_LEN], expected_metadata_key_hmac)
        .ok()
        .map(|_| (decrypt_buf, cipher))
}
