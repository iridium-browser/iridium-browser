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
//! Serialization and deserialization for v0 (legacy) and v1 (extended) Nearby Presence
//! advertisements.
//!
//! See `tests/examples_v0.rs` and `tests/examples_v1.rs` for some tests that show common
//! deserialization scenarios.

#![no_std]
#![forbid(unsafe_code)]
#![deny(missing_docs)]

extern crate alloc;
extern crate core;
use crate::{
    credential::{
        source::{BothCredentialSource, CredentialSource},
        v0::V0CryptoMaterial,
        MatchedCredFromCred, MatchedCredential, V0Credential, V1Credential,
    },
    extended::deserialize::{
        parse_sections, CiphertextSection, DataElements, DecryptedSection, IntermediateSection,
        PlaintextSection, Section, SectionDeserializeError,
    },
    legacy::deserialize::{
        DecryptError, DecryptedAdvContents, IntermediateAdvContents, PlaintextAdvContents,
    },
};
use alloc::vec::Vec;
#[cfg(feature = "devtools")]
use array_view::ArrayView;
use core::{fmt::Debug, marker};
use crypto_provider::CryptoProvider;
#[cfg(feature = "devtools")]
use extended::NP_ADV_MAX_SECTION_LEN;
use legacy::{data_elements::DataElementDeserializeError, deserialize::AdvDeserializeError};
use nom::{combinator, number};
pub mod credential;
pub mod de_type;
#[cfg(test)]
mod deser_v0_tests;
#[cfg(test)]
mod deser_v1_tests;
pub mod extended;
#[cfg(test)]
mod header_parse_tests;
pub mod legacy;
pub mod shared_data;
/// Canonical form of NP's service UUID.
///
/// Note that UUIDs are encoded in BT frames in little-endian order, so these bytes may need to be
/// reversed depending on the host BT API.
pub const NP_SVC_UUID: [u8; 2] = [0xFC, 0xF1];

/// Parse, deserialize, decrypt, and validate a complete NP advertisement (the entire contents of
/// the service data for the NP UUID).
pub fn deserialize_advertisement<'s, C0, C1, M, S, P>(
    adv: &[u8],
    cred_source: &'s S,
) -> Result<DeserializedAdvertisement<'s, M>, AdvDeserializationError>
where
    C0: V0Credential<Matched<'s> = M> + 's,
    C1: V1Credential<Matched<'s> = M> + 's,
    M: MatchedCredential<'s>,
    S: BothCredentialSource<C0, C1>,
    P: CryptoProvider,
{
    let (remaining, header) =
        parse_adv_header(adv).map_err(|_e| AdvDeserializationError::HeaderParseError)?;
    match header {
        AdvHeader::V1(header) => {
            deser_decrypt_v1::<C1, S::V1Source, P>(cred_source.v1(), remaining, header)
                .map(DeserializedAdvertisement::V1)
        }
        AdvHeader::V0 => deser_decrypt_v0::<C0, S::V0Source, P>(cred_source.v0(), remaining)
            .map(DeserializedAdvertisement::V0),
    }
}

/// Parse, deserialize, decrypt, and validate a complete V0 NP advertisement (the entire contents
/// of the service data for the NP UUID). If the advertisement version header does not match V0,
/// this method will return an [`AdvDeserializationError::HeaderParseError`]
pub fn deserialize_v0_advertisement<'s, C, S, P>(
    adv: &[u8],
    cred_source: &'s S,
) -> Result<V0AdvertisementContents<'s, C>, AdvDeserializationError>
where
    C: V0Credential,
    S: CredentialSource<C>,
    P: CryptoProvider,
{
    let (remaining, header) =
        parse_adv_header(adv).map_err(|_e| AdvDeserializationError::HeaderParseError)?;

    match header {
        AdvHeader::V0 => deser_decrypt_v0::<C, S, P>(cred_source, remaining),
        AdvHeader::V1(_) => Err(AdvDeserializationError::HeaderParseError),
    }
}

/// Parse, deserialize, decrypt, and validate a complete V1 NP advertisement (the entire contents
/// of the service data for the NP UUID). If the advertisement version header does not match V1,
/// this method will return an [`AdvDeserializationError::HeaderParseError`]
pub fn deserialize_v1_advertisement<'s, C, S, P>(
    adv: &[u8],
    cred_source: &'s S,
) -> Result<V1AdvertisementContents<'s, C>, AdvDeserializationError>
where
    C: V1Credential,
    S: CredentialSource<C>,
    P: CryptoProvider,
{
    let (remaining, header) =
        parse_adv_header(adv).map_err(|_e| AdvDeserializationError::HeaderParseError)?;

    match header {
        AdvHeader::V0 => Err(AdvDeserializationError::HeaderParseError),
        AdvHeader::V1(header) => deser_decrypt_v1::<C, S, P>(cred_source, remaining, header),
    }
}

type V1AdvertisementContents<'s, C> = V1AdvContents<'s, MatchedCredFromCred<'s, C>>;

/// The encryption scheme used for a V1 advertisement.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum V1EncryptionScheme {
    /// Indicates MIC-based encryption and verification.
    Mic,
    /// Indicates signature-based encryption and verification.
    Signature,
}

/// Error in decryption operations for `deser_decrypt_v1_section_bytes_for_dev_tools`.
#[cfg(feature = "devtools")]
#[derive(Debug, Clone)]
pub enum AdvDecryptionError {
    /// Cannot decrypt because the input section is not encrypted.
    InputNotEncrypted,
    /// Error parsing the given section.
    ParseError,
    /// No suitable credential found to decrypt the given section.
    NoMatchingCredentials,
}

/// Decrypt, but do not further deserialize the v1 bytes, intended for developer tooling uses only.
/// Production uses should use [deserialize_v1_advertisement] instead, which deserializes to a
/// structured format and provides extra type safety.
#[cfg(feature = "devtools")]
pub fn deser_decrypt_v1_section_bytes_for_dev_tools<S, V1, P>(
    cred_source: &S,
    header_byte: u8,
    section_bytes: &[u8],
) -> Result<(ArrayView<u8, NP_ADV_MAX_SECTION_LEN>, V1EncryptionScheme), AdvDecryptionError>
where
    S: CredentialSource<V1>,
    V1: V1Credential,
    P: CryptoProvider,
{
    let header = V1Header { header_byte };
    let int_sections =
        parse_sections(&header, section_bytes).map_err(|_| AdvDecryptionError::ParseError)?;
    let cipher_section = match &int_sections[0] {
        IntermediateSection::Plaintext(_) => Err(AdvDecryptionError::InputNotEncrypted)?,
        IntermediateSection::Ciphertext(section) => section,
    };

    use crate::credential::v1::V1CryptoMaterial;
    use core::borrow::Borrow;

    for cred in cred_source.iter() {
        let crypto_material = cred.crypto_material();

        match cipher_section {
            CiphertextSection::SignatureEncryptedIdentity(encrypted_section) => {
                let identity_resolution_material =
                    crypto_material.signed_identity_resolution_material::<P>();
                match encrypted_section.try_decrypt::<P>(identity_resolution_material.borrow()) {
                    Ok(plaintext) => return Ok((plaintext, V1EncryptionScheme::Signature)),
                    Err(_) => continue,
                }
            }
            CiphertextSection::MicEncryptedIdentity(encrypted_section) => {
                let identity_resolution_material =
                    crypto_material.unsigned_identity_resolution_material::<P>();
                let verification_material = crypto_material.unsigned_verification_material::<P>();
                match encrypted_section
                    .try_decrypt::<P>(identity_resolution_material.borrow(), &verification_material)
                {
                    Ok(plaintext) => return Ok((plaintext, V1EncryptionScheme::Mic)),
                    Err(_) => continue,
                }
            }
        }
    }
    Err(AdvDecryptionError::NoMatchingCredentials)
}

/// Deserialize and decrypt the contents of a v1 adv after the version header
fn deser_decrypt_v1<'s, C, S, P>(
    cred_source: &'s S,
    remaining: &[u8],
    header: V1Header,
) -> Result<V1AdvertisementContents<'s, C>, AdvDeserializationError>
where
    C: V1Credential,
    S: CredentialSource<C>,
    P: CryptoProvider,
{
    let int_sections =
        parse_sections(&header, remaining).map_err(|_| AdvDeserializationError::ParseError {
            details_hazmat: AdvDeserializationErrorDetailsHazmat::AdvertisementDeserializeError,
        })?;
    let mut sections = Vec::new();
    let mut to_decrypt = Vec::new();
    // keep track of ordering for later sorting
    for (idx, s) in int_sections.into_iter().enumerate() {
        match s {
            IntermediateSection::Plaintext(p) => {
                sections.push((idx, V1DeserializedSection::Plaintext(p)))
            }
            IntermediateSection::Ciphertext(c) => to_decrypt.push((idx, c)),
        }
    }
    let mut invalid_sections = 0;
    // Hot loop
    // We assume that iterating credentials is more expensive than iterating sections
    for cred in cred_source.iter() {
        let mut i = 0;
        while i < to_decrypt.len() {
            let (section_idx, c): &(usize, CiphertextSection) = &to_decrypt[i];
            match c.try_deserialize::<C, P>(cred) {
                Ok(s) => {
                    sections.push((
                        *section_idx,
                        V1DeserializedSection::Decrypted(WithMatchedCredential::new(
                            cred.matched(),
                            s,
                        )),
                    ));
                    // we don't care about maintaining order, so use O(1) remove
                    to_decrypt.swap_remove(i);
                    // don't advance i -- it now points to a new element
                }
                Err(e) => match e {
                    SectionDeserializeError::IncorrectCredential => {
                        // keep it around to try with another credential
                        i += 1;
                    }
                    SectionDeserializeError::ParseError => {
                        // the credential worked, but the section itself was bogus, so drop
                        // it
                        invalid_sections += 1;
                        to_decrypt.swap_remove(i);
                    }
                },
            }
        }
        if to_decrypt.is_empty() {
            // no need to consider the remaining credentials
            break;
        }
    }
    invalid_sections += to_decrypt.len();
    // decryption may produce sections out of order
    sections.sort_by_key(|(idx, _section)| *idx);
    Ok(V1AdvContents::new(sections.into_iter().map(|(_idx, s)| s).collect(), invalid_sections))
}

type V0AdvertisementContents<'s, C> = V0AdvContents<'s, MatchedCredFromCred<'s, C>>;

/// Deserialize and decrypt the contents of a v0 adv after the version header
fn deser_decrypt_v0<'s, C, S, P>(
    cred_source: &'s S,
    remaining: &[u8],
) -> Result<V0AdvertisementContents<'s, C>, AdvDeserializationError>
where
    C: V0Credential,
    S: CredentialSource<C>,
    P: CryptoProvider,
{
    let contents = legacy::deserialize::deserialize_adv_contents::<P>(remaining)?;
    return match contents {
        IntermediateAdvContents::Plaintext(p) => Ok(V0AdvContents::Plaintext(p)),
        IntermediateAdvContents::Ciphertext(c) => {
            for cred in cred_source.iter() {
                let cm = cred.crypto_material();
                let ldt = cm.ldt_adv_cipher::<P>();
                match c.try_decrypt(&ldt) {
                    Ok(c) => {
                        return Ok(V0AdvContents::Decrypted(WithMatchedCredential::new(
                            cred.matched(),
                            c,
                        )))
                    }
                    Err(e) => match e {
                        DecryptError::DecryptOrVerifyError => continue,
                        DecryptError::DeserializeError(e) => {
                            return Err(e.into());
                        }
                    },
                }
            }
            Ok(V0AdvContents::NoMatchingCredentials)
        }
    };
}
/// Parse a NP advertisement header.
///
/// This can be used on all versions of advertisements since it's the header that determines the
/// version.
///
/// Returns a `nom::IResult` with the parsed header and the remaining bytes of the advertisement.
fn parse_adv_header(adv: &[u8]) -> nom::IResult<&[u8], AdvHeader> {
    // header bits: VVVxxxxx
    let (remaining, (header_byte, version, _low_bits)) = combinator::verify(
        // splitting a byte at a bit boundary to take lower 5 bits
        combinator::map(number::complete::u8, |byte| (byte, byte >> 5, byte & 0x1F)),
        |&(_header_byte, version, low_bits)| match version {
            // reserved bits, for any version, must be zero
            PROTOCOL_VERSION_LEGACY | PROTOCOL_VERSION_EXTENDED => low_bits == 0,
            _ => false,
        },
    )(adv)?;
    match version {
        PROTOCOL_VERSION_LEGACY => Ok((remaining, AdvHeader::V0)),
        PROTOCOL_VERSION_EXTENDED => Ok((remaining, AdvHeader::V1(V1Header { header_byte }))),
        _ => unreachable!(),
    }
}
#[derive(Debug, PartialEq, Eq, Clone)]
pub(crate) enum AdvHeader {
    V0,
    V1(V1Header),
}
/// An NP advertisement with its header parsed.
#[derive(Debug, PartialEq, Eq)]
pub enum DeserializedAdvertisement<'m, M: MatchedCredential<'m>> {
    /// V0 header has all reserved bits, so there is no data to represent other than the version
    /// itself.
    V0(V0AdvContents<'m, M>),
    /// V1 advertisement
    V1(V1AdvContents<'m, M>),
}
/// The contents of a deserialized and decrypted V1 advertisement.
#[derive(Debug, PartialEq, Eq)]
pub struct V1AdvContents<'m, M: MatchedCredential<'m>> {
    sections: Vec<V1DeserializedSection<'m, M>>,
    invalid_sections: usize,
}
impl<'m, M: MatchedCredential<'m>> V1AdvContents<'m, M> {
    fn new(sections: Vec<V1DeserializedSection<'m, M>>, invalid_sections: usize) -> Self {
        Self { sections, invalid_sections }
    }
    /// Destructures this V1 advertisement into just the sections
    /// which could be successfully deserialized and decrypted
    pub fn into_valid_sections(self) -> Vec<V1DeserializedSection<'m, M>> {
        self.sections
    }
    /// The sections that could be successfully deserialized and decrypted
    pub fn sections(&self) -> impl Iterator<Item = &V1DeserializedSection<M>> {
        self.sections.iter()
    }
    /// The number of sections that could not be parsed or decrypted.
    pub fn invalid_sections_count(&self) -> usize {
        self.invalid_sections
    }
}
/// Advertisement content that was either already plaintext or has been decrypted.
#[derive(Debug, PartialEq, Eq)]
pub enum V0AdvContents<'m, M: MatchedCredential<'m>> {
    /// Contents of an originally plaintext advertisement
    Plaintext(PlaintextAdvContents),
    /// Contents that was ciphertext in the original advertisement, and has been decrypted
    /// with the credential in the [MatchedCredential]
    Decrypted(WithMatchedCredential<'m, M, DecryptedAdvContents>),
    /// The advertisement was encrypted, but no credentials matched
    NoMatchingCredentials,
}
/// Advertisement content that was either already plaintext or has been decrypted.
#[derive(Debug, PartialEq, Eq)]
pub enum V1DeserializedSection<'m, M: MatchedCredential<'m>> {
    /// Section that was plaintext in the original advertisement
    Plaintext(PlaintextSection),
    /// Section that was ciphertext in the original advertisement, and has been decrypted
    /// with the credential in the [MatchedCredential]
    Decrypted(WithMatchedCredential<'m, M, DecryptedSection>),
}
impl<'m, M> Section for V1DeserializedSection<'m, M>
where
    M: MatchedCredential<'m>,
{
    type Iterator<'d>  = DataElements<'d> where Self: 'd;
    fn data_elements(&'_ self) -> Self::Iterator<'_> {
        match self {
            V1DeserializedSection::Plaintext(p) => p.data_elements(),
            V1DeserializedSection::Decrypted(d) => d.contents.data_elements(),
        }
    }
}
/// Decrypted advertisement content with the [MatchedCredential] from the credential that decrypted
/// it.
#[derive(Debug, PartialEq, Eq)]
pub struct WithMatchedCredential<'m, M: MatchedCredential<'m>, T> {
    matched: M,
    contents: T,
    // the compiler sees 'm as unused
    marker: marker::PhantomData<&'m ()>,
}
impl<'m, M: MatchedCredential<'m>, T> WithMatchedCredential<'m, M, T> {
    fn new(matched: M, contents: T) -> Self {
        Self { matched, contents, marker: marker::PhantomData }
    }
    /// Credential data for the credential that decrypted the content.
    pub fn matched_credential(&self) -> &M {
        &self.matched
    }
    /// The decrypted advertisement content.
    pub fn contents(&self) -> &T {
        &self.contents
    }
}
/// Data in a V1 advertisement header.
#[derive(Debug, PartialEq, Eq, Clone)]
pub(crate) struct V1Header {
    header_byte: u8,
}
const PROTOCOL_VERSION_LEGACY: u8 = 0;
const PROTOCOL_VERSION_EXTENDED: u8 = 1;

/// Errors that can occur during advertisement deserialization.
#[derive(PartialEq)]
pub enum AdvDeserializationError {
    /// The advertisement header could not be parsed
    HeaderParseError,
    /// The advertisement content could not be parsed
    ParseError {
        /// Potentially hazardous details about deserialization errors. Read the documentation for
        /// [AdvDeserializationErrorDetailsHazmat] before using this field.
        details_hazmat: AdvDeserializationErrorDetailsHazmat,
    },
}

impl Debug for AdvDeserializationError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            AdvDeserializationError::HeaderParseError => write!(f, "HeaderParseError"),
            AdvDeserializationError::ParseError { .. } => write!(f, "ParseError"),
        }
    }
}

/// Potentially hazardous details about deserialization errors. These error information can
/// potentially expose side-channel information about the plaintext of the advertisements and/or
/// the keys used to decrypt them. For any place that you avoid exposing the keys directly
/// (e.g. across FFIs, print to log, etc), avoid exposing these error details as well.
#[derive(PartialEq)]
pub enum AdvDeserializationErrorDetailsHazmat {
    /// Parsing the overall advertisement or DE structure failed
    AdvertisementDeserializeError,
    /// Deserializing an individual DE from its DE contents failed
    V0DataElementDeserializeError(DataElementDeserializeError),
    /// Must not have any other top level data elements if there is an encrypted identity DE
    TooManyTopLevelDataElements,
    /// Must not have an identity DE inside an identity DE
    InvalidDataElementHierarchy,
}

impl From<AdvDeserializeError> for AdvDeserializationError {
    fn from(err: AdvDeserializeError) -> Self {
        match err {
            AdvDeserializeError::AdvertisementDeserializeError => {
                AdvDeserializationError::ParseError {
                    details_hazmat:
                        AdvDeserializationErrorDetailsHazmat::AdvertisementDeserializeError,
                }
            }
            AdvDeserializeError::DataElementDeserializeError(e) => {
                AdvDeserializationError::ParseError {
                    details_hazmat:
                        AdvDeserializationErrorDetailsHazmat::V0DataElementDeserializeError(e),
                }
            }
            AdvDeserializeError::TooManyTopLevelDataElements => {
                AdvDeserializationError::ParseError {
                    details_hazmat:
                        AdvDeserializationErrorDetailsHazmat::TooManyTopLevelDataElements,
                }
            }
            AdvDeserializeError::InvalidDataElementHierarchy => {
                AdvDeserializationError::ParseError {
                    details_hazmat:
                        AdvDeserializationErrorDetailsHazmat::InvalidDataElementHierarchy,
                }
            }
        }
    }
}

/// DE length is out of range (e.g. > 4 bits for encoded V0, > max DE size for actual V0, >127 for
/// V1) or invalid for the relevant DE type.
#[derive(Debug, PartialEq, Eq)]
pub struct DeLengthOutOfRange;

/// The identity mode for a deserialized plaintext section or advertisement.
#[derive(PartialEq, Eq, Debug, Clone, Copy)]
pub enum PlaintextIdentityMode {
    /// No identity DE was present in the section
    None,
    /// A "Public Identity" DE was present in the section
    Public,
}

/// A "public identity" -- a nonspecific "empty identity".
///
/// Used when serializing V0 advertisements or V1 sections.
#[derive(Default, Debug)]
pub struct PublicIdentity {}

/// The lack of any identity information whatsoever, which is distinct from [PublicIdentity].
///
/// Used when serializing V0 advertisements or V1 sections.
#[derive(Default, Debug)]
pub struct NoIdentity {}
