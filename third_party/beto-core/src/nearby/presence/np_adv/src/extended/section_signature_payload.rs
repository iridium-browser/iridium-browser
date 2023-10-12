// Copyright 2023 Google LLC
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

//! V1 advertisement section np_ed25519 signature payload
//! after the included context bytes, and utilities for
//! performing signatures and signature verification.

use crate::extended::deserialize::EncryptionInfo;
use crate::NP_SVC_UUID;
use crypto_provider::{aes::ctr::AesCtrNonce, CryptoProvider};
use sink::{Sink, SinkWriter};

/// A struct representing the necessary contents
/// of an v1 advertisement section np_ed25519 signature payload which
/// come after the context prefix (shared among all advs).
pub(crate) struct SectionSignaturePayload<'a> {
    /// Advertisement header byte
    adv_header_byte: u8,
    /// Header byte for the v1 section being signed
    section_header: u8,
    /// Reference to the complete contents of the [`EncryptionInfo`] DE.
    encryption_info: &'a [u8; EncryptionInfo::TOTAL_DE_LEN],
    /// Reference to the derived salt (IV) for the section
    nonce_ref: &'a AesCtrNonce,
    /// Reference to all remaining information after the derived salt, but
    /// not including the signature itself [which gets tacked onto the end].
    after_iv_info: AfterIVInfo<'a>,
}

/// Representation of the plaintext information in an advertisement
/// signature payload which comes after the derived salt
enum AfterIVInfo<'a> {
    /// Reference to a raw byte array containing all information
    /// to be included in the signature payload after the derived salt,
    /// and before the signature itself.
    Raw(&'a [u8]),
    /// References to the plaintext identity DE header and the rest
    /// of the section plaintext after that (includes the plaintext
    /// identity DE payload).
    IdentityHeaderThenRaw([u8; 2], &'a [u8]),
}

const ADV_SIGNATURE_CONTEXT: np_ed25519::SignatureContext = {
    match np_ed25519::SignatureContext::from_string_bytes("Advertisement Signed Section") {
        Ok(x) => x,
        Err(_) => panic!(),
    }
};

impl<'a> SectionSignaturePayload<'a> {
    /// Construct a section signature payload using parts typically found during
    /// serialization of advertisements.
    pub(crate) fn from_serialized_parts(
        adv_header_byte: u8,
        section_header: u8,
        encryption_info: &'a [u8; EncryptionInfo::TOTAL_DE_LEN],
        nonce_ref: &'a AesCtrNonce,
        raw_after_iv_info: &'a [u8],
    ) -> Self {
        Self {
            adv_header_byte,
            section_header,
            encryption_info,
            nonce_ref,
            after_iv_info: AfterIVInfo::Raw(raw_after_iv_info),
        }
    }

    /// Construct a section signature payload using parts typically found during
    /// deserialization of advertisements.
    pub(crate) fn from_deserialized_parts(
        adv_header_byte: u8,
        section_header: u8,
        encryption_info: &'a [u8; EncryptionInfo::TOTAL_DE_LEN],
        nonce_ref: &'a AesCtrNonce,
        identity_header: [u8; 2],
        raw_after_identity_header_info: &'a [u8],
    ) -> Self {
        Self {
            adv_header_byte,
            section_header,
            encryption_info,
            nonce_ref,
            after_iv_info: AfterIVInfo::IdentityHeaderThenRaw(
                identity_header,
                raw_after_identity_header_info,
            ),
        }
    }

    /// Generates a signature for this section signing payload using
    /// the given Ed25519 key-pair.
    pub(crate) fn sign<C: CryptoProvider>(
        self,
        key_pair: &np_ed25519::KeyPair<C>,
    ) -> np_ed25519::Signature<C> {
        key_pair
            .sign_with_context(&ADV_SIGNATURE_CONTEXT, self)
            .expect("section signature payloads should fit in signature buffer")
    }
    /// Verifies a signature for this section signing payload using
    /// the given Ed25519 public key.
    pub(crate) fn verify<C: CryptoProvider>(
        self,
        signature: &np_ed25519::Signature<C>,
        public_key: &np_ed25519::PublicKey<C>,
    ) -> Result<(), np_ed25519::SignatureVerificationError> {
        public_key.verify_signature_with_context(&ADV_SIGNATURE_CONTEXT, self, signature)
    }
}

impl<'a> SinkWriter for SectionSignaturePayload<'a> {
    type DataType = u8;

    fn write_payload<S: Sink<u8> + ?Sized>(self, sink: &mut S) -> Option<()> {
        sink.try_extend_from_slice(&NP_SVC_UUID)?;
        sink.try_push(self.adv_header_byte)?;
        sink.try_push(self.section_header)?;
        sink.try_extend_from_slice(self.encryption_info)?;
        sink.try_extend_from_slice(self.nonce_ref)?;

        // identity DE and the rest of the DEs except for the suffix
        match self.after_iv_info {
            AfterIVInfo::Raw(s) => sink.try_extend_from_slice(s),
            AfterIVInfo::IdentityHeaderThenRaw(identity_header, s) => {
                sink.try_extend_from_slice(&identity_header)?;
                sink.try_extend_from_slice(s)
            }
        }
    }
}
