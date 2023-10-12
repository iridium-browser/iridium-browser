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

//! V0 data element deserialization.
//!
//! This module only deals with the _contents_ of an advertisement, not the advertisement header.

extern crate alloc;

use crate::{
    de_type::EncryptedIdentityDataElementType,
    legacy::{
        actions,
        data_elements::{DataElement, *},
        de_type::{DataElementType, DeEncodedLength, DeTypeCode, PlainDataElementType},
        Ciphertext, PacketFlavor, Plaintext, NP_MAX_DE_CONTENT_LEN,
    },
    PlaintextIdentityMode,
};
use alloc::vec::Vec;
use crypto_provider::CryptoProvider;
use ldt_np_adv::{LegacySalt, NP_LEGACY_METADATA_KEY_LEN};
use nom::{bytes, combinator, multi, number, sequence};

#[cfg(test)]
mod tests;

/// Deserialize an advertisement into data elements (if plaintext) or an identity type and
/// ciphertext.
pub(crate) fn deserialize_adv_contents<C: CryptoProvider>(
    input: &[u8],
) -> Result<IntermediateAdvContents, AdvDeserializeError> {
    parse_raw_adv_contents::<C>(input).and_then(|raw_adv| match raw_adv {
        RawAdvertisement::Plaintext(parc) => parc
            .try_deserialize()
            .map(IntermediateAdvContents::Plaintext)
            .map_err(AdvDeserializeError::DataElementDeserializeError),
        RawAdvertisement::Ciphertext(eac) => Ok(IntermediateAdvContents::Ciphertext(eac)),
    })
}

/// Parse an advertisement's contents to the level of raw data elements (i.e no decryption,
/// no per-type deserialization, etc).
///
/// Consumes the entire input.
fn parse_raw_adv_contents<C: CryptoProvider>(
    input: &[u8],
) -> Result<RawAdvertisement, AdvDeserializeError> {
    let (_, data_elements) = parse_data_elements(input)
        .map_err(|_e| AdvDeserializeError::AdvertisementDeserializeError)?;

    if let Some(identity_de_type) =
        data_elements.first().and_then(|de| de.de_type.try_as_identity_de_type())
    {
        match identity_de_type.as_encrypted_identity_de_type() {
            Some(encrypted_de_type) => {
                if data_elements.len() == 1 {
                    match encrypted_de_type {
                        // TODO handle length=0 provisioned identity DEs
                        EncryptedIdentityDataElementType::Private
                        | EncryptedIdentityDataElementType::Trusted
                        | EncryptedIdentityDataElementType::Provisioned => {
                            combinator::map(
                                parse_encrypted_identity_de_contents,
                                |(salt, payload)| {
                                    RawAdvertisement::Ciphertext(EncryptedAdvContents {
                                        identity_type: encrypted_de_type,
                                        salt_padder: ldt_np_adv::salt_padder::<16, C>(salt),
                                        salt,
                                        ciphertext: payload,
                                    })
                                },
                            )(data_elements[0].contents)
                            .map(|(_rem, contents)| contents)
                            .map_err(|_e| AdvDeserializeError::AdvertisementDeserializeError)
                        }
                    }
                } else {
                    Err(AdvDeserializeError::TooManyTopLevelDataElements)
                }
            }
            // It's an identity de, but not encrypted, so it must be public, and the rest must be
            // plain
            None => plain_data_elements(&data_elements[1..]).map(|pdes| {
                RawAdvertisement::Plaintext(PlaintextAdvRawContents {
                    identity_type: PlaintextIdentityMode::Public,
                    data_elements: pdes,
                })
            }),
        }
    } else {
        plain_data_elements(&data_elements).map(|pde| {
            RawAdvertisement::Plaintext(PlaintextAdvRawContents {
                identity_type: PlaintextIdentityMode::None,
                data_elements: pde,
            })
        })
    }
}

/// Legacy advertisement parsing errors
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum AdvDeserializeError {
    /// Parsing the overall advertisement or DE structure failed
    AdvertisementDeserializeError,
    /// Deserializing an individual DE from its DE contents failed
    DataElementDeserializeError(DataElementDeserializeError),
    /// Must not have any other top level data elements if there is an encrypted identity DE
    TooManyTopLevelDataElements,
    /// Must not have an identity DE inside an identity DE
    InvalidDataElementHierarchy,
}

/// Parse an advertisement's contents into raw DEs.
///
/// Consumes the entire input.
fn parse_data_elements(adv_contents: &[u8]) -> nom::IResult<&[u8], Vec<RawDataElement>> {
    combinator::all_consuming(multi::many0(parse_de))(adv_contents)
}

/// Parse an individual DE into its header and contents.
fn parse_de(input: &[u8]) -> nom::IResult<&[u8], RawDataElement> {
    let (remaining, (de_type, actual_len)) =
        combinator::map_opt(number::complete::u8, |de_header| {
            // header: LLLLTTTT
            let len = de_header >> 4;
            let de_type = de_header & 0x0F;
            DeTypeCode::try_from(de_type).ok().and_then(DataElementType::from_type_code).and_then(
                |de_type| {
                    len.try_into()
                        .ok()
                        .and_then(|len: DeEncodedLength| {
                            de_type.actual_len_for_encoded_len(len).ok()
                        })
                        .map(|len| (de_type, len))
                },
            )
        })(input)?;

    combinator::map(bytes::complete::take(actual_len.as_usize()), move |contents| RawDataElement {
        de_type,
        contents,
    })(remaining)
}

/// Returns `Err`` if any DEs are not of a plain DE type.
fn plain_data_elements<'d, D: AsRef<[RawDataElement<'d>]>>(
    data_elements: D,
) -> Result<Vec<RawPlainDataElement<'d>>, AdvDeserializeError> {
    data_elements
        .as_ref()
        .iter()
        .map(|de| {
            de.de_type
                .try_as_plain_de_type()
                .map(|de_type| RawPlainDataElement { de_type, contents: de.contents })
        })
        .collect::<Option<Vec<_>>>()
        .ok_or(AdvDeserializeError::InvalidDataElementHierarchy)
}

/// Parse legacy encrypted identity DEs (private, trusted, provisioned) into salt and ciphertext
/// (encrypted metadata key and at least 2 bytes of DEs).
///
/// Consumes the entire input.
fn parse_encrypted_identity_de_contents(
    de_contents: &[u8],
) -> nom::IResult<&[u8], (ldt_np_adv::LegacySalt, &[u8])> {
    combinator::all_consuming(sequence::tuple((
        // 2-byte salt
        combinator::map(
            combinator::map_res(bytes::complete::take(2_usize), |slice: &[u8]| slice.try_into()),
            |arr: [u8; 2]| arr.into(),
        ),
        // 14-byte encrypted metadata key plus encrypted DEs, which must together be at least 16
        // bytes (AES block size), and at most a full DE minus the size of the salt.
        bytes::complete::take_while_m_n(16_usize, NP_MAX_DE_CONTENT_LEN - 2, |_| true),
    )))(de_contents)
}

/// A data element with content length determined and validated per its type's length rules, but
/// no further decoding performed.
#[derive(Debug, PartialEq, Eq)]
struct RawDataElement<'d> {
    de_type: DataElementType,
    contents: &'d [u8],
}

/// An advertisement broken down into data elements, but before decryption or mapping to higher
/// level DE representations.
#[derive(Debug, PartialEq, Eq)]
enum RawAdvertisement<'d> {
    Plaintext(PlaintextAdvRawContents<'d>),
    Ciphertext(EncryptedAdvContents<'d>),
}

/// A plaintext advertisement's content in raw DEs but without further deserialization.
#[derive(Debug, PartialEq, Eq)]
struct PlaintextAdvRawContents<'d> {
    identity_type: PlaintextIdentityMode,
    data_elements: Vec<RawPlainDataElement<'d>>,
}

impl<'d> PlaintextAdvRawContents<'d> {
    /// Deserialize the DE contents into per-DE-type structures.
    ///
    /// Returns `Some` if each DE's contents can be successfully deserialized, otherwise `None`.
    fn try_deserialize(&self) -> Result<PlaintextAdvContents, DataElementDeserializeError> {
        self.data_elements
            .iter()
            .map(|de| de.try_deserialize::<Plaintext>())
            .collect::<Result<Vec<_>, _>>()
            .map(|des| PlaintextAdvContents {
                identity_type: self.identity_type,
                data_elements: des,
            })
    }
}

/// A "plain" data element (not a container) without parsing the content.
#[derive(Debug, PartialEq, Eq)]
pub(crate) struct RawPlainDataElement<'d> {
    de_type: PlainDataElementType,
    contents: &'d [u8],
}

impl<'d> RawPlainDataElement<'d> {
    /// Deserialize into a [PlainDataElement] to expose DE-type-specific data representations.
    ///
    /// Returns `None` if the contents of the raw DE can't be deserialized into the corresponding
    /// DE's representation.
    fn try_deserialize<F>(&self) -> Result<PlainDataElement<F>, DataElementDeserializeError>
    where
        F: PacketFlavor,
        actions::ActionsDataElement<F>: DataElement,
    {
        match self.de_type {
            PlainDataElementType::Actions => {
                actions::ActionsDataElement::deserialize::<F>(self.contents)
                    .map(PlainDataElement::Actions)
            }
            PlainDataElementType::TxPower => {
                TxPowerDataElement::deserialize::<F>(self.contents).map(PlainDataElement::TxPower)
            }
        }
    }
}

/// Contents of an encrypted advertisement before decryption.
#[derive(Debug, PartialEq, Eq)]
pub(crate) struct EncryptedAdvContents<'d> {
    identity_type: EncryptedIdentityDataElementType,
    /// Salt from the advertisement, converted into a padder.
    /// Pre-calculated so it's only derived once across multiple decrypt attempts.
    salt_padder: ldt::XorPadder<{ crypto_provider::aes::BLOCK_SIZE }>,
    /// The salt instance used for encryption of this advertisement.
    salt: LegacySalt,
    /// Ciphertext containing the metadata key and any data elements
    ciphertext: &'d [u8],
}

impl<'d> EncryptedAdvContents<'d> {
    /// Try decrypting with an identity's LDT cipher and deserializing the resulting data elements.
    ///
    /// Returns the decrypted data if decryption and verification succeeded and the resulting DEs could be parsed
    /// successfully, otherwise `Err`.
    pub(crate) fn try_decrypt<C: CryptoProvider>(
        &self,
        cipher: &ldt_np_adv::LdtNpAdvDecrypterXtsAes128<C>,
    ) -> Result<DecryptedAdvContents, DecryptError> {
        let plaintext = cipher
            .decrypt_and_verify(self.ciphertext, &self.salt_padder)
            .map_err(|_e| DecryptError::DecryptOrVerifyError)?;

        // plaintext starts with 14 bytes of metadata key, then DEs.

        let (remaining, metadata_key) = combinator::map_res(
            bytes::complete::take(NP_LEGACY_METADATA_KEY_LEN),
            |slice: &[u8]| slice.try_into(),
        )(plaintext.as_slice())
        .map_err(|_e: nom::Err<nom::error::Error<&[u8]>>| {
            DecryptError::DeserializeError(AdvDeserializeError::AdvertisementDeserializeError)
        })?;

        let (_remaining, raw_des) = combinator::all_consuming(parse_data_elements)(remaining)
            .map_err(|_e| {
                DecryptError::DeserializeError(AdvDeserializeError::AdvertisementDeserializeError)
            })?;

        plain_data_elements(&raw_des)?
            .into_iter()
            .map(|de| de.try_deserialize())
            .collect::<Result<Vec<_>, _>>()
            .map_err(|e| {
                DecryptError::DeserializeError(AdvDeserializeError::DataElementDeserializeError(e))
            })
            .map(|data_elements| {
                DecryptedAdvContents::new(
                    self.identity_type,
                    metadata_key,
                    self.salt,
                    data_elements,
                )
            })
    }
}

/// Errors that can occur decrypting encrypted advertisements.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub(crate) enum DecryptError {
    /// Decrypting or verifying the advertisement ciphertext failed
    DecryptOrVerifyError,
    /// Decrypting succeeded, but deserializing from the plaintext failed
    DeserializeError(AdvDeserializeError),
}

impl From<AdvDeserializeError> for DecryptError {
    fn from(e: AdvDeserializeError) -> Self {
        DecryptError::DeserializeError(e)
    }
}

/// All v0 normal DE types with deserialized contents.
#[derive(Debug, PartialEq, Eq)]
#[allow(missing_docs)]
pub enum PlainDataElement<F: PacketFlavor> {
    Actions(actions::ActionsDataElement<F>),
    TxPower(TxPowerDataElement),
}

/// The contents of a plaintext advertisement after deserializing DE contents
#[derive(Debug, PartialEq, Eq)]
pub struct PlaintextAdvContents {
    identity_type: PlaintextIdentityMode,
    data_elements: Vec<PlainDataElement<Plaintext>>,
}

impl PlaintextAdvContents {
    /// Returns the identity type used for the advertisement
    pub fn identity(&self) -> PlaintextIdentityMode {
        self.identity_type
    }
    /// Returns the deserialized data elements
    pub fn data_elements(&self) -> impl Iterator<Item = &PlainDataElement<Plaintext>> {
        self.data_elements.iter()
    }
    /// Destructures this V0 plaintext advertisement
    /// into just the contained data elements
    pub fn to_data_elements(self) -> Vec<PlainDataElement<Plaintext>> {
        self.data_elements
    }
}

/// Contents of an encrypted advertisement after decryption and deserializing DEs.
#[derive(Debug, PartialEq, Eq)]
pub struct DecryptedAdvContents {
    identity_type: EncryptedIdentityDataElementType,
    metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN],
    salt: LegacySalt,
    data_elements: Vec<PlainDataElement<Ciphertext>>,
}

impl DecryptedAdvContents {
    /// Returns a new DecryptedAdvContents with the provided contents.
    fn new(
        identity_type: EncryptedIdentityDataElementType,
        metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN],
        salt: LegacySalt,
        data_elements: Vec<PlainDataElement<Ciphertext>>,
    ) -> Self {
        Self { identity_type, metadata_key, salt, data_elements }
    }

    /// The type of identity DE used in the advertisement.
    pub fn identity_type(&self) -> EncryptedIdentityDataElementType {
        self.identity_type
    }

    /// The decrypted metadata key from the identity DE.
    pub fn metadata_key(&self) -> &[u8; 14] {
        &self.metadata_key
    }

    /// Iterator over the data elements in an advertisement, except for any DEs related to resolving
    /// the identity or otherwise validating the payload (e.g. any identity DEs like Private
    /// Identity).
    pub fn data_elements(&self) -> impl Iterator<Item = &PlainDataElement<Ciphertext>> {
        self.data_elements.iter()
    }

    /// The salt used for decryption of this advertisement.
    pub fn salt(&self) -> LegacySalt {
        self.salt
    }
}

/// The contents of an advertisement after plaintext DEs, if any, have been deserialized, but
/// before any decryption is done.
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum IntermediateAdvContents<'d> {
    /// Plaintext advertisements
    Plaintext(PlaintextAdvContents),
    /// Ciphertext advertisements
    Ciphertext(EncryptedAdvContents<'d>),
}
