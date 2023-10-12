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

//! Deserialization for V1 advertisement contents

extern crate alloc;

use alloc::vec::Vec;
use core::array::TryFromSliceError;
use core::borrow::Borrow;
use core::slice;

use nom::{branch, bytes, combinator, error, multi, number, sequence};
use strum::IntoEnumIterator as _;

use array_view::ArrayView;
use crypto_provider::CryptoProvider;
use np_hkdf::v1_salt::{self, V1Salt};

use crate::extended::NP_V1_ADV_MAX_SECTION_COUNT;
use crate::{
    credential::{v1::*, V1Credential},
    de_type::{EncryptedIdentityDataElementType, IdentityDataElementType},
    extended::{
        data_elements::{MIC_ENCRYPTION_SCHEME, SIGNATURE_ENCRYPTION_SCHEME},
        de_type::{DeType, ExtendedDataElementType as _},
        deserialize::encrypted_section::{
            MicEncryptedSection, MicEncryptedSectionDeserializationError,
            SignatureEncryptedSection, SignatureEncryptedSectionDeserializationError,
        },
        to_array_view, DeLength, ENCRYPTION_INFO_DE_TYPE, METADATA_KEY_LEN, NP_ADV_MAX_SECTION_LEN,
    },
    PlaintextIdentityMode, V1Header,
};

mod encrypted_section;

#[cfg(test)]
mod parse_tests;

#[cfg(test)]
mod section_tests;

#[cfg(test)]
mod test_stubs;

/// Parse into [IntermediateSection]s, exposing the underlying parsing errors.
/// Consumes all of `adv_body`.
pub(crate) fn parse_sections<'a, 'h: 'a>(
    adv_header: &'h V1Header,
    adv_body: &'a [u8],
) -> Result<Vec<IntermediateSection<'a>>, nom::Err<error::Error<&'a [u8]>>> {
    combinator::all_consuming(multi::many_m_n(
        1,
        NP_V1_ADV_MAX_SECTION_COUNT,
        IntermediateSection::parser_with_header(adv_header),
    ))(adv_body)
    .map(|(_rem, sections)| sections)
}

/// A partially processed section that hasn't been decrypted (if applicable) yet.
#[derive(PartialEq, Eq, Debug)]
// sections are large because they have a stack allocated buffer for the whole section
#[allow(clippy::large_enum_variant)]
pub(crate) enum IntermediateSection<'a> {
    /// A section that was not encrypted, e.g. a public identity or no-identity section.
    Plaintext(PlaintextSection),
    /// A section whose contents were encrypted, e.g. a private identity section.
    Ciphertext(CiphertextSection<'a>),
}

impl<'a> IntermediateSection<'a> {
    fn parser_with_header(
        adv_header: &'a V1Header,
    ) -> impl Fn(&'a [u8]) -> nom::IResult<&[u8], IntermediateSection> {
        move |adv_body| {
            let (remaining, section_contents_len) =
                combinator::verify(number::complete::u8, |sec_len| {
                    *sec_len as usize <= NP_ADV_MAX_SECTION_LEN && *sec_len as usize > 0
                })(adv_body)?;

            // Section structure possibilities:
            // - Public Identity DE, all other DEs
            // - No identity DE, all other DEs (including Salt, excluding identity DEs)
            // - Encryption information, non-public Identity header, ciphertext
            // - MIC, encryption information, non-public Identity header, ciphertext

            let parse_public_identity = combinator::map(
                // 1 starting offset because of public identity before it
                sequence::tuple((PublicIdentity::parse, parse_non_identity_des(1))),
                // move closure to copy section_len into scope
                move |(_identity, des)| {
                    IntermediateSection::Plaintext(PlaintextSection::new(
                        PlaintextIdentityMode::Public,
                        SectionContents::new(/* section_header */ section_contents_len, &des),
                    ))
                },
            );

            // 0 starting offset because there's no identity DE
            let parse_no_identity_de = combinator::map(parse_non_identity_des(0), move |des| {
                IntermediateSection::Plaintext(PlaintextSection::new(
                    PlaintextIdentityMode::None,
                    SectionContents::new(/* section_header */ section_contents_len, &des),
                ))
            });

            let parse_encrypted_identity = combinator::map(
                sequence::tuple((
                    EncryptionInfo::parse_signature,
                    combinator::verify(
                        combinator::consumed(sequence::tuple((
                            EncryptedIdentityMetadata::parser_at_offset(
                                v1_salt::DataElementOffset::from(1),
                            ),
                            combinator::rest,
                        ))),
                        // should be trivially true since section length was checked above,
                        // but this is an invariant for EncryptedSection, so we double check
                        |(identity_and_ciphertext, _tuple)| {
                            (METADATA_KEY_LEN..=NP_ADV_MAX_SECTION_LEN)
                                .contains(&identity_and_ciphertext.len())
                        },
                    ),
                )),
                move |(encryption_info, (identity_and_ciphertext, (identity, _des_ciphertext)))| {
                    // skip identity de header -- rest of that de is ciphertext
                    let to_skip = identity.header_bytes.len();
                    IntermediateSection::Ciphertext(CiphertextSection::SignatureEncryptedIdentity(
                        SignatureEncryptedSection {
                            section_header: section_contents_len,
                            adv_header,
                            encryption_info,
                            identity,
                            all_ciphertext: &identity_and_ciphertext[to_skip..],
                        },
                    ))
                },
            );

            let parse_mic_encrypted_identity = combinator::map(
                sequence::tuple((
                    EncryptionInfo::parse_mic,
                    combinator::verify(
                        combinator::consumed(sequence::tuple((
                            EncryptedIdentityMetadata::parser_at_offset(
                                v1_salt::DataElementOffset::from(1),
                            ),
                            combinator::rest,
                        ))),
                        // Should be trivially true since section length was checked above,
                        // but this is an invariant for MicEncryptedSection, so we double check.
                        // Also verify that there is enough space at the end to contain a valid-length MIC.
                        |(identity_ciphertext_and_mic, _tuple)| {
                            (METADATA_KEY_LEN + SectionMic::CONTENTS_LEN..=NP_ADV_MAX_SECTION_LEN)
                                .contains(&identity_ciphertext_and_mic.len())
                        },
                    ),
                )),
                move |(
                    encryption_info,
                    (identity_ciphertext_and_mic, (identity, _ciphertext_and_mic)),
                )| {
                    // should not panic since we have already ensured a valid length
                    let (identity_and_ciphertext, mic) = identity_ciphertext_and_mic
                        .split_at(identity_ciphertext_and_mic.len() - SectionMic::CONTENTS_LEN);
                    // skip identity de header -- rest of that de is ciphertext
                    let to_skip = identity.header_bytes.len();
                    IntermediateSection::Ciphertext(CiphertextSection::MicEncryptedIdentity(
                        MicEncryptedSection {
                            section_header: section_contents_len,
                            adv_header,
                            mic: mic.try_into().unwrap_or_else(|_| {
                                panic!("{} is a valid length", SectionMic::CONTENTS_LEN)
                            }),
                            encryption_info,
                            identity,
                            all_ciphertext: &identity_and_ciphertext[to_skip..],
                        },
                    ))
                },
            );

            combinator::map_parser(
                bytes::complete::take(section_contents_len),
                // guarantee we consume all of the section (not all of the adv body)
                combinator::all_consuming(branch::alt((
                    parse_mic_encrypted_identity,
                    parse_encrypted_identity,
                    parse_public_identity,
                    parse_no_identity_de,
                ))),
            )(remaining)
        }
    }
}

#[derive(PartialEq, Eq, Debug)]
struct SectionContents {
    section_header: u8,
    de_data: ArrayView<u8, NP_ADV_MAX_SECTION_LEN>,
    data_elements: Vec<OffsetDataElement>,
}

impl SectionContents {
    fn new(section_header: u8, data_elements: &[RefDataElement]) -> Self {
        let (data_elements, de_data) = convert_data_elements(data_elements);

        Self { section_header, de_data, data_elements }
    }

    fn data_elements(&'_ self) -> DataElements<'_> {
        DataElements { de_iter: self.data_elements.iter(), de_data: &self.de_data }
    }
}

/// An iterator over data elements in a V1 section
// A concrete type to make it easy to refer to in return types where opaque types aren't available.
pub struct DataElements<'a> {
    de_iter: slice::Iter<'a, OffsetDataElement>,
    de_data: &'a ArrayView<u8, NP_ADV_MAX_SECTION_LEN>,
}

impl<'a> Iterator for DataElements<'a> {
    type Item = DataElement<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        self.de_iter.next().map(|de| DataElement { de_data: self.de_data, de })
    }
}

/// A section deserialized from a V1 advertisement.
pub trait Section {
    /// The iterator type used to iterate over data elements
    type Iterator<'a>: Iterator<Item = DataElement<'a>>
    where
        Self: 'a;

    /// Iterator over the data elements in a section, except for any DEs related to resolving the
    /// identity or otherwise validating the payload (e.g. MIC, Signature, any identity DEs like
    /// Private Identity).
    fn data_elements(&'_ self) -> Self::Iterator<'_>;
}

/// A plaintext section deserialized from a V1 advertisement.
#[derive(PartialEq, Eq, Debug)]
pub struct PlaintextSection {
    identity: PlaintextIdentityMode,
    contents: SectionContents,
}

impl PlaintextSection {
    fn new(identity: PlaintextIdentityMode, contents: SectionContents) -> Self {
        Self { identity, contents }
    }

    /// The identity mode for the section.
    ///
    /// Since plaintext sections do not use encryption, they cannot be matched to a single identity,
    /// and only have a mode (no identity or public).
    pub fn identity(&self) -> PlaintextIdentityMode {
        self.identity
    }
}

impl Section for PlaintextSection {
    type Iterator<'a>  = DataElements<'a> where Self: 'a;

    fn data_elements(&'_ self) -> Self::Iterator<'_> {
        self.contents.data_elements()
    }
}

/// A byte buffer the size of a V1 salt.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct RawV1Salt([u8; 16]);

impl RawV1Salt {
    /// Returns the raw salt bytes as a vec.
    #[cfg(feature = "devtools")]
    pub fn to_vec(&self) -> Vec<u8> {
        self.0.to_vec()
    }
}

impl<C: CryptoProvider> From<RawV1Salt> for V1Salt<C> {
    fn from(raw_salt: RawV1Salt) -> Self {
        raw_salt.0.into()
    }
}

/// The decrypted contents of a ciphertext section.
#[derive(Debug, PartialEq, Eq)]
pub struct DecryptedSection {
    identity_type: EncryptedIdentityDataElementType,
    verification_mode: VerificationMode,
    metadata_key: [u8; METADATA_KEY_LEN],
    salt: RawV1Salt,
    contents: SectionContents,
}

impl DecryptedSection {
    fn new(
        identity_type: EncryptedIdentityDataElementType,
        verification_mode: VerificationMode,
        metadata_key: [u8; 16],
        salt: RawV1Salt,
        contents: SectionContents,
    ) -> Self {
        Self { identity_type, verification_mode, metadata_key, salt, contents }
    }

    /// The type of identity DE used in the section.
    pub fn identity_type(&self) -> EncryptedIdentityDataElementType {
        self.identity_type
    }

    /// The verification mode used in the section.
    pub fn verification_mode(&self) -> VerificationMode {
        self.verification_mode
    }

    /// The decrypted metadata key from the identity DE.
    pub fn metadata_key(&self) -> &[u8; 16] {
        &self.metadata_key
    }

    /// The salt used for decryption of this advertisement.
    pub fn salt(&self) -> RawV1Salt {
        self.salt
    }
}

impl Section for DecryptedSection {
    type Iterator<'a>  = DataElements<'a> where Self: 'a;

    fn data_elements(&'_ self) -> Self::Iterator<'_> {
        self.contents.data_elements()
    }
}

#[derive(PartialEq, Eq, Debug)]
pub(crate) enum CiphertextSection<'a> {
    SignatureEncryptedIdentity(SignatureEncryptedSection<'a>),
    MicEncryptedIdentity(MicEncryptedSection<'a>),
}

impl<'a> CiphertextSection<'a> {
    pub(crate) fn try_deserialize<'c, C, P>(
        &'a self,
        credential: &'c C,
    ) -> Result<DecryptedSection, SectionDeserializeError>
    where
        C: V1Credential,
        P: CryptoProvider,
    {
        let crypto_material = credential.crypto_material();
        match self {
            Self::SignatureEncryptedIdentity(contents) => {
                let identity_resolution_material =
                    crypto_material.signed_identity_resolution_material::<P>();
                let verification_material = crypto_material.signed_verification_material::<P>();

                contents
                    .try_deserialize::<P>(
                        identity_resolution_material.borrow(),
                        &verification_material,
                    )
                    .map_err(|e| match e {
                        SignatureEncryptedSectionDeserializationError::MetadataKeyMacMismatch
                        | SignatureEncryptedSectionDeserializationError::SignatureMismatch => {
                            SectionDeserializeError::IncorrectCredential
                        }
                        SignatureEncryptedSectionDeserializationError::SignatureMissing
                        | SignatureEncryptedSectionDeserializationError::DeParseError => {
                            SectionDeserializeError::ParseError
                        }
                    })
            }
            Self::MicEncryptedIdentity(contents) => {
                let identity_resolution_material =
                    crypto_material.unsigned_identity_resolution_material::<P>();
                let verification_material = crypto_material.unsigned_verification_material::<P>();

                contents
                    .try_deserialize::<P>(
                        identity_resolution_material.borrow(),
                        &verification_material,
                    )
                    .map_err(|e| match e {
                        MicEncryptedSectionDeserializationError::MetadataKeyMacMismatch
                        | MicEncryptedSectionDeserializationError::MicMismatch => {
                            SectionDeserializeError::IncorrectCredential
                        }
                        MicEncryptedSectionDeserializationError::DeParseError => {
                            SectionDeserializeError::ParseError
                        }
                    })
            }
        }
    }
}

/// Errors that can occur when deserializing an advertisement
#[derive(Debug, PartialEq, Eq)]
pub enum SectionDeserializeError {
    /// The credential supplied did not match the section's identity data
    IncorrectCredential,
    /// Section data is malformed
    ParseError,
}

/// Returns a parser function that parses data elements whose type is not one of the known identity
/// DE types, and whose offsets start from the provided `starting_offset`.
///
/// Consumes all input.
fn parse_non_identity_des(
    starting_offset: usize,
) -> impl Fn(&[u8]) -> nom::IResult<&[u8], Vec<RefDataElement>> {
    move |input: &[u8]| {
        combinator::map_opt(
            combinator::all_consuming(multi::many0(combinator::verify(
                ProtoDataElement::parse,
                |de| IdentityDataElementType::iter().all(|t| t.type_code() != de.header.de_type),
            ))),
            |proto_des| {
                proto_des
                    .into_iter()
                    .enumerate()
                    .map(|(offset, pde)| starting_offset.checked_add(offset).map(|sum| (sum, pde)))
                    .map(|res| {
                        res.map(|(offset, pde)| {
                            pde.into_data_element(v1_salt::DataElementOffset::from(offset))
                        })
                    })
                    .collect::<Option<Vec<_>>>()
            },
        )(input)
    }
}

/// Deserialize-specific version of a DE header that incorporates the header length.
/// This is needed for encrypted identities that need to construct a slice of everything in the
/// section following the identity DE header.
#[derive(Debug, PartialEq, Eq, Clone)]
pub(crate) struct DeHeader {
    /// The original bytes of the header, at most 6 bytes long (1 byte len, 5 bytes type)
    pub(crate) header_bytes: ArrayView<u8, 6>,
    pub(crate) de_type: DeType,
    pub(crate) contents_len: DeLength,
}

impl DeHeader {
    pub(crate) fn parse(input: &[u8]) -> nom::IResult<&[u8], DeHeader> {
        // 1-byte header: 0b0LLLTTTT
        let parse_single_byte_de_header =
            combinator::map_opt::<&[u8], _, DeHeader, error::Error<&[u8]>, _, _>(
                combinator::consumed(combinator::map_res(
                    combinator::verify(number::complete::u8, |&b| !hi_bit_set(b)),
                    |b| {
                        // L bits
                        let len = (b >> 4) & 0x07;
                        // T bits
                        let de_type = ((b & 0x0F) as u32).into();

                        len.try_into().map(|l| (l, de_type))
                    },
                )),
                |(header_bytes, (len, de_type))| {
                    ArrayView::try_from_slice(header_bytes).map(|header_bytes| DeHeader {
                        header_bytes,
                        contents_len: len,
                        de_type,
                    })
                },
            );

        // multi-byte headers: 0b1LLLLLLL (0b1TTTTTTT)* 0b0TTTTTTT
        // leading 1 in first byte = multibyte format
        // leading 1 in subsequent bytes = there is at least 1 more type bytes
        // leading 0 = this is the last header byte
        // 127-bit length, effectively infinite type bit length

        // It's conceivable to have non-canonical extended type sequences where 1 or more leading
        // bytes don't have any bits set (other than the marker hi bit), thereby contributing nothing
        // to the final value.
        // To prevent that, we require that either there be only 1 type byte, or that the first of the
        // multiple type bytes must have a value bit set. It's OK to have no value bits in subsequent
        // type bytes.

        let parse_ext_de_header = combinator::map_opt(
            combinator::consumed(sequence::pair(
                // length byte w/ leading 1
                combinator::map_res(
                    combinator::verify(number::complete::u8::<&[u8], _>, |&b| hi_bit_set(b)),
                    // snag the lower 7 bits
                    |b| (b & 0x7F).try_into(),
                ),
                branch::alt((
                    // 1 type byte case
                    combinator::recognize(
                        // 0-hi-bit type code byte
                        combinator::verify(number::complete::u8, |&b| !hi_bit_set(b)),
                    ),
                    // multiple type byte case: leading type byte must have at least 1 value bit
                    combinator::recognize(sequence::tuple((
                        // hi bit and at least 1 value bit, otherwise it would be non-canonical
                        combinator::verify(number::complete::u8, |&b| {
                            hi_bit_set(b) && (b & 0x7F != 0)
                        }),
                        // 0-3 1-hi-bit type code bytes with any bit pattern. Max is 3 since two 7
                        // bit type chunks are processed before and after this, for a total of 5,
                        // and that's as many 7-bit chunks as are needed to support a 32-bit type.
                        bytes::complete::take_while_m_n(0, 3, hi_bit_set),
                        // final 0-hi-bit type code byte
                        combinator::verify(number::complete::u8, |&b| !hi_bit_set(b)),
                    ))),
                )),
            )),
            |(header_bytes, (len, type_bytes))| {
                // snag the low 7 bits of each type byte and accumulate

                type_bytes
                    .iter()
                    .fold(Some(0_u32), |accum, b| {
                        accum.and_then(|n| n.checked_shl(7)).map(|n| n + ((b & 0x7F) as u32))
                    })
                    .and_then(|type_code| {
                        ArrayView::try_from_slice(header_bytes).map(|header_bytes| DeHeader {
                            header_bytes,
                            contents_len: len,
                            de_type: type_code.into(),
                        })
                    })
            },
        );

        branch::alt((parse_single_byte_de_header, parse_ext_de_header))(input)
    }
}

/// An intermediate stage in parsing a [DataElement] that lacks `offset`.
#[derive(Debug, PartialEq, Eq)]
struct ProtoDataElement<'d> {
    header: DeHeader,
    /// `len()` must equal `header.contents_len`
    contents: &'d [u8],
}

impl<'d> ProtoDataElement<'d> {
    fn parse(input: &[u8]) -> nom::IResult<&[u8], ProtoDataElement> {
        let (remaining, header) = DeHeader::parse(input)?;
        let len = header.contents_len;
        combinator::map(bytes::complete::take(len.as_usize()), move |slice| {
            let header_clone = header.clone();
            ProtoDataElement { header: header_clone, contents: slice }
        })(remaining)
    }

    fn into_data_element(self, offset: v1_salt::DataElementOffset) -> RefDataElement<'d> {
        RefDataElement {
            offset,
            header_len: self.header.header_bytes.len().try_into().expect("header is <= 6 bytes"),
            de_type: self.header.de_type,
            contents: self.contents,
        }
    }
}

/// A data element that holds a slice reference for its contents
#[derive(Debug, PartialEq, Eq)]
pub(crate) struct RefDataElement<'d> {
    pub(crate) offset: v1_salt::DataElementOffset,
    pub(crate) header_len: u8,
    pub(crate) de_type: DeType,
    pub(crate) contents: &'d [u8],
}

/// A deserialized data element in a section.
///
/// The DE has been processed to the point of exposing a DE type and its contents as a `&[u8]`, but
/// no DE-type-specific processing has been performed.
#[derive(Debug)]
pub struct DataElement<'a> {
    de_data: &'a ArrayView<u8, NP_ADV_MAX_SECTION_LEN>,
    de: &'a OffsetDataElement,
}

impl<'a> DataElement<'a> {
    /// The offset of the DE in its containing Section.
    ///
    /// Used with the section salt to derive per-DE salt.
    pub fn offset(&self) -> v1_salt::DataElementOffset {
        self.de.offset
    }
    /// The type of the DE
    pub fn de_type(&self) -> DeType {
        self.de.de_type
    }
    /// The contents of the DE
    pub fn contents(&self) -> &[u8] {
        &self.de_data.as_slice()
            [self.de.start_of_contents..self.de.start_of_contents + self.de.contents_len]
    }
}

/// The level of integrity protection in an encrypted section
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum VerificationMode {
    /// A symmetric MIC (message integrity code aka message authentication code) was verified.
    ///
    /// Since this is a symmetric operation, holders of the key material needed to verify a MIC
    /// can also forge MICs.
    Mic,
    /// An asymmetric signature was verified.
    ///
    /// Since this is an asymmetric operation, only the holder of the private key can generate
    /// signatures, so it offers a stronger level of authenticity protection than [Self::Mic].
    Signature,
}

/// The identity used to successfully decrypt and validate an encrypted section
#[derive(Clone, PartialEq, Eq, Debug)]
pub struct EncryptedSectionIdentity {
    identity_type: EncryptedIdentityDataElementType,
    validation_mode: VerificationMode,
    metadata_key: [u8; METADATA_KEY_LEN],
}

impl EncryptedSectionIdentity {
    /// The type of identity DE used in the section
    pub fn identity_type(&self) -> EncryptedIdentityDataElementType {
        self.identity_type
    }
    /// The validation mode used when decrypting and verifying the section
    pub fn verification_mode(&self) -> VerificationMode {
        self.validation_mode
    }
    /// The decrypted metadata key from the section's identity DE
    pub fn metadata_key(&self) -> &[u8; METADATA_KEY_LEN] {
        &self.metadata_key
    }
}

/// A DE that designates its contents via offset and length to avoid self-referential slices.
#[derive(Debug, PartialEq, Eq)]
struct OffsetDataElement {
    offset: v1_salt::DataElementOffset,
    de_type: DeType,
    start_of_contents: usize,
    contents_len: usize,
}

/// Convert data elements from holding slices to holding offsets and lengths to avoid
/// lifetime woes.
/// This entails some data copying, so if it causes noticeable perf issues we can revisit
/// it, but it is likely to be much cheaper than decryption.
///
/// # Panics
///
/// Will panic if handed more data elements than fit in one section. This is only possible if
/// generating data elements from a source other than parsing a section.
fn convert_data_elements(
    elements: &[RefDataElement],
) -> (Vec<OffsetDataElement>, ArrayView<u8, NP_ADV_MAX_SECTION_LEN>) {
    let mut buf = tinyvec::ArrayVec::new();

    (
        elements
            .iter()
            .map(|de| {
                let current_len = buf.len();
                // won't overflow because these DEs originally came from a section, and now
                // we're packing only their contents without DE headers
                buf.extend_from_slice(de.contents);
                OffsetDataElement {
                    offset: de.offset,
                    de_type: de.de_type,
                    start_of_contents: current_len,
                    contents_len: de.contents.len(),
                }
            })
            .collect(),
        to_array_view(buf),
    )
}

#[derive(PartialEq, Eq, Debug, Clone)]
pub(crate) struct EncryptionInfo {
    pub bytes: [u8; 19],
}

impl EncryptionInfo {
    // 2-byte header, 1-byte encryption scheme, 16-byte salt
    pub(crate) const TOTAL_DE_LEN: usize = 19;
    const CONTENTS_LEN: usize = 17;
    const ENCRYPTION_INFO_SCHEME_MASK: u8 = 0b01111000;

    fn parse_signature(input: &[u8]) -> nom::IResult<&[u8], EncryptionInfo> {
        Self::parser_for_scheme(SIGNATURE_ENCRYPTION_SCHEME)(input)
    }

    fn parse_mic(input: &[u8]) -> nom::IResult<&[u8], EncryptionInfo> {
        Self::parser_for_scheme(MIC_ENCRYPTION_SCHEME)(input)
    }

    fn parser_for_scheme(
        expected_scheme: u8,
    ) -> impl Fn(&[u8]) -> nom::IResult<&[u8], EncryptionInfo> {
        move |input| {
            combinator::map_res(
                combinator::consumed(combinator::map_parser(
                    combinator::map(
                        combinator::verify(ProtoDataElement::parse, |de| {
                            de.header.de_type == ENCRYPTION_INFO_DE_TYPE
                                && de.contents.len() == Self::CONTENTS_LEN
                        }),
                        |de| de.contents,
                    ),
                    sequence::tuple((
                        combinator::verify(number::complete::be_u8, |scheme: &u8| {
                            expected_scheme == (Self::ENCRYPTION_INFO_SCHEME_MASK & scheme)
                        }),
                        bytes::complete::take(16_usize),
                    )),
                )),
                |(bytes, _contents)| bytes.try_into(),
            )(input)
        }
    }

    fn salt(&self) -> RawV1Salt {
        // should never panic
        RawV1Salt(self.bytes[Self::TOTAL_DE_LEN - 16..].try_into().ok().unwrap())
    }
}

impl TryFrom<&[u8]> for EncryptionInfo {
    type Error = TryFromSliceError;

    fn try_from(value: &[u8]) -> Result<Self, Self::Error> {
        value.try_into().map(|fixed_bytes: [u8; 19]| Ok(Self { bytes: fixed_bytes }))?
    }
}

#[derive(PartialEq, Eq, Debug)]
pub(crate) struct SectionMic {
    mic: [u8; 16],
}

impl SectionMic {
    // 16-byte metadata key
    pub(crate) const CONTENTS_LEN: usize = 16;
}

impl From<[u8; 16]> for SectionMic {
    fn from(value: [u8; 16]) -> Self {
        SectionMic { mic: value }
    }
}

impl TryFrom<&[u8]> for SectionMic {
    type Error = TryFromSliceError;

    fn try_from(value: &[u8]) -> Result<Self, Self::Error> {
        let fixed_bytes: [u8; SectionMic::CONTENTS_LEN] = value.try_into()?;
        Ok(Self { mic: fixed_bytes })
    }
}

#[derive(PartialEq, Eq, Debug)]
struct PublicIdentity;

impl PublicIdentity {
    fn parse(input: &[u8]) -> nom::IResult<&[u8], PublicIdentity> {
        combinator::map(
            combinator::verify(DeHeader::parse, |deh| {
                deh.de_type == IdentityDataElementType::Public.type_code()
                    && deh.contents_len.as_usize() == 0
            }),
            |_| PublicIdentity,
        )(input)
    }
}

/// Parsed form of an encrypted identity DE before its contents are decrypted.
/// Metadata key is stored in the enclosing section.
#[derive(PartialEq, Eq, Debug)]
pub(crate) struct EncryptedIdentityMetadata {
    pub(crate) offset: v1_salt::DataElementOffset,
    /// The original DE header from the advertisement.
    /// Encrypted identity should always be a len=2 header.
    pub(crate) header_bytes: [u8; 2],
    pub(crate) identity_type: EncryptedIdentityDataElementType,
}

impl EncryptedIdentityMetadata {
    // 2-byte header, 16-byte metadata key
    pub(crate) const TOTAL_DE_LEN: usize = 18;

    /// Returns a parser function that parses an [EncryptedIdentity] using the provided DE `offset`.
    fn parser_at_offset(
        offset: v1_salt::DataElementOffset,
    ) -> impl Fn(&[u8]) -> nom::IResult<&[u8], EncryptedIdentityMetadata> {
        move |input| {
            combinator::map_opt(ProtoDataElement::parse, |de| {
                EncryptedIdentityDataElementType::from_type_code(de.header.de_type).and_then(
                    |identity_type| {
                        de.header.header_bytes.as_slice().try_into().ok().and_then(|header_bytes| {
                            de.contents.try_into().ok().map(|_metadata_key_ciphertext: [u8; 16]| {
                                // ensure the ciphertext is the right size, then discard
                                EncryptedIdentityMetadata { header_bytes, offset, identity_type }
                            })
                        })
                    },
                )
            })(input)
        }
    }
}

fn hi_bit_set(b: u8) -> bool {
    b & 0x80 > 0
}
