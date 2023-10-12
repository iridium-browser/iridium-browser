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

//! V0 data element types.
//!
//! In V0, there are only 16 DE types total, and parsing unknown types is not possible, so we can
//! represent all known DE types in enums without needing to handle the "unknown type" case.

use crate::{
    de_type::IdentityDataElementType,
    legacy::{
        actions::ActionsDataElement,
        data_elements::{DataElement as _, TxPowerDataElement},
        Ciphertext, PacketFlavorEnum, Plaintext, NP_MAX_DE_CONTENT_LEN,
    },
    DeLengthOutOfRange,
};
use core::ops;
use ldt_np_adv::NP_LEGACY_METADATA_KEY_LEN;
use strum_macros::EnumIter;

#[cfg(test)]
mod tests;

/// A V0 DE type in `[0, 15]`.
#[derive(Debug, PartialEq, Eq, Hash)]
pub(crate) struct DeTypeCode {
    /// The code used in a V0 adv header
    code: u8,
}

impl DeTypeCode {
    /// Returns a u8 in `[0, 15`].
    pub(crate) fn as_u8(&self) -> u8 {
        self.code
    }
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub(crate) struct DeTypeCodeOutOfRange;

impl TryFrom<u8> for DeTypeCode {
    type Error = DeTypeCodeOutOfRange;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        if value < 16 {
            Ok(Self { code: value })
        } else {
            Err(DeTypeCodeOutOfRange)
        }
    }
}

/// The actual length of a DE, not the encoded representation.
///
/// See [DeEncodedLength] for the encoded length.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub(crate) struct DeActualLength {
    /// Invariant: <= `NP_MAX_DE_CONTENT_LEN`.
    len: u8,
}

impl DeActualLength {
    pub(crate) const ZERO: DeActualLength = DeActualLength { len: 0 };

    pub(crate) fn as_usize(&self) -> usize {
        self.len as usize
    }
}

impl TryFrom<u8> for DeActualLength {
    type Error = DeLengthOutOfRange;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        if (value as usize) <= NP_MAX_DE_CONTENT_LEN {
            Ok(Self { len: value })
        } else {
            Err(DeLengthOutOfRange)
        }
    }
}

impl TryFrom<usize> for DeActualLength {
    type Error = DeLengthOutOfRange;

    fn try_from(value: usize) -> Result<Self, Self::Error> {
        if value <= NP_MAX_DE_CONTENT_LEN {
            Ok(Self { len: value as u8 })
        } else {
            Err(DeLengthOutOfRange)
        }
    }
}

/// The encoded length of a DE, not the actual length of the DE contents.
///
/// See [DeActualLength] for the length of the contents.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub(crate) struct DeEncodedLength {
    /// Invariant: `len <= 0x0F` (15, aka 4 bits)
    len: u8,
}

impl DeEncodedLength {
    /// Returns a u8 in `[0, 15]`
    pub(crate) fn as_u8(&self) -> u8 {
        self.len
    }

    /// Returns a usize in `[0, 15]`
    pub(crate) fn as_usize(&self) -> usize {
        self.len as usize
    }
}

impl TryFrom<u8> for DeEncodedLength {
    type Error = DeLengthOutOfRange;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        if value < 16 {
            Ok(Self { len: value })
        } else {
            Err(DeLengthOutOfRange)
        }
    }
}

/// DE types for normal DEs (not an identity).
///
/// May be contained in identity DEs (see [IdentityDataElementType]). Must not overlap with
/// [IdentityDataElementType].
#[derive(EnumIter, Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum PlainDataElementType {
    TxPower,
    Actions,
}

impl PlainDataElementType {
    pub(crate) fn as_generic_de_type(&self) -> DataElementType {
        match self {
            PlainDataElementType::TxPower => DataElementType::TxPower,
            PlainDataElementType::Actions => DataElementType::Actions,
        }
    }

    pub(crate) fn supports_flavor(&self, flavor: PacketFlavorEnum) -> bool {
        match self {
            PlainDataElementType::Actions => {
                // Actions is effectively two different DEs based on which actions it can
                // contain, so we have to check them separately. The Plaintext one obviously
                // supports plaintext, and vice versa, so we could just say `true` here, but we
                // spell it out to be consistent with the other DE types.
                match flavor {
                    PacketFlavorEnum::Plaintext => {
                        ActionsDataElement::<Plaintext>::supports_flavor(flavor)
                    }
                    PacketFlavorEnum::Ciphertext => {
                        ActionsDataElement::<Ciphertext>::supports_flavor(flavor)
                    }
                }
            }
            PlainDataElementType::TxPower => TxPowerDataElement::supports_flavor(flavor),
        }
    }
}

/// Corresponds to the different implementations of [DataElement].
///
/// It's intended for use cases that don't care if a DE is a plain or identity DE. Every variant
/// corresponds to either a [PlainDataElementType] or a [IdentityDataElementType].
#[derive(EnumIter, Debug, Clone, Copy, PartialEq, Eq)]
#[allow(missing_docs)]
#[doc(hidden)]
pub enum DataElementType {
    PrivateIdentity,
    TrustedIdentity,
    PublicIdentity,
    ProvisionedIdentity,
    TxPower,
    Actions,
}

const DE_TYPES_BY_CODE: [Option<DataElementType>; 16] = [
    None,                                       // 0b0000
    Some(DataElementType::PrivateIdentity),     // 0b0001
    Some(DataElementType::TrustedIdentity),     // 0b0010
    Some(DataElementType::PublicIdentity),      // 0b0011
    Some(DataElementType::ProvisionedIdentity), // 0b0100
    Some(DataElementType::TxPower),             // 0b0101
    Some(DataElementType::Actions),             // 0b0110
    None,                                       // 0b0111
    None,                                       // 0b1000
    None,                                       // 0b1001
    None,                                       // 0b1010
    None,                                       // 0b1011
    None,                                       // 0b1100
    None,                                       // 0b1101
    None,                                       // 0b1110
    None,                                       // 0b1111
];

impl DataElementType {
    /// Salt + key + at least 2 bytes of DE to make LDT ciphertext min length
    const VALID_ENCRYPTED_IDENTITY_DE_ACTUAL_LEN: ops::RangeInclusive<usize> =
        (2 + NP_LEGACY_METADATA_KEY_LEN + 2..=NP_MAX_DE_CONTENT_LEN);
    const VALID_ENCRYPTED_IDENTITY_DE_HEADER_LEN: ops::RangeInclusive<usize> = (2..=6);

    /// If there is a corresponding [ContainerDataElementType], returns it, otherwise None.
    pub(crate) fn try_as_identity_de_type(&self) -> Option<IdentityDataElementType> {
        match self {
            DataElementType::PrivateIdentity => Some(IdentityDataElementType::Private),
            DataElementType::TrustedIdentity => Some(IdentityDataElementType::Trusted),
            DataElementType::PublicIdentity => Some(IdentityDataElementType::Public),
            DataElementType::ProvisionedIdentity => Some(IdentityDataElementType::Provisioned),
            DataElementType::TxPower | DataElementType::Actions => None,
        }
    }

    /// If there is a corresponding [PlainDataElementType], returns it, otherwise None.
    pub(crate) fn try_as_plain_de_type(&self) -> Option<PlainDataElementType> {
        match self {
            DataElementType::TxPower => Some(PlainDataElementType::TxPower),
            DataElementType::Actions => Some(PlainDataElementType::Actions),
            DataElementType::PrivateIdentity
            | DataElementType::TrustedIdentity
            | DataElementType::PublicIdentity
            | DataElementType::ProvisionedIdentity => None,
        }
    }

    /// Returns the matching type for the code, else `None`
    pub(crate) fn from_type_code(de_type: DeTypeCode) -> Option<Self> {
        DE_TYPES_BY_CODE.get(de_type.as_u8() as usize).and_then(|o| *o)
    }

    /// A type code in `[0,15]` for use in the high bits of the DE header byte.
    pub(crate) fn type_code(&self) -> DeTypeCode {
        match self {
            DataElementType::PrivateIdentity => IdentityDataElementType::Private.shared_type_code(),
            DataElementType::TrustedIdentity => IdentityDataElementType::Trusted.shared_type_code(),
            DataElementType::PublicIdentity => IdentityDataElementType::Public.shared_type_code(),
            DataElementType::ProvisionedIdentity => {
                IdentityDataElementType::Provisioned.shared_type_code()
            }
            DataElementType::TxPower => 0b0101,
            DataElementType::Actions => 0b0110,
        }
        .try_into()
        .expect("hardcoded type codes are valid")
    }

    /// Convert the actual DE length to the encoded length included in the header.
    ///
    /// Returns `Err` if the actual length is invalid for the type, or the corresponding encoded length is out of range.
    pub(crate) fn encoded_len_for_actual_len(
        &self,
        actual_len: DeActualLength,
    ) -> Result<DeEncodedLength, DeLengthOutOfRange> {
        match self {
            // TODO 0-length provisioned
            DataElementType::PrivateIdentity
            | DataElementType::TrustedIdentity
            | DataElementType::ProvisionedIdentity => {
                if !Self::VALID_ENCRYPTED_IDENTITY_DE_ACTUAL_LEN.contains(&actual_len.as_usize()) {
                    Err(DeLengthOutOfRange)
                } else {
                    actual_len
                        .len
                        .checked_sub(16)
                        .ok_or(DeLengthOutOfRange)
                        .and_then(|n| n.try_into())
                }
            }
            DataElementType::PublicIdentity => {
                if actual_len.len != 0 {
                    Err(DeLengthOutOfRange)
                } else {
                    actual_len.len.try_into()
                }
            }
            DataElementType::TxPower => {
                if actual_len.len != 1 {
                    Err(DeLengthOutOfRange)
                } else {
                    actual_len.len.try_into()
                }
            }
            DataElementType::Actions => {
                // doesn't matter which variant is used
                if !ActionsDataElement::<Plaintext>::ACTIONS_LEN.contains(&actual_len.as_usize()) {
                    Err(DeLengthOutOfRange)
                } else {
                    actual_len.len.try_into()
                }
            }
        }
    }

    /// Convert the length in the header to the actual DE length.
    ///
    /// Returns `Err` if the encoded length is invalid for the type, or the corresponding actual length is out of range.
    pub(crate) fn actual_len_for_encoded_len(
        &self,
        header_len: DeEncodedLength,
    ) -> Result<DeActualLength, DeLengthOutOfRange> {
        match self {
            DataElementType::PrivateIdentity
            | DataElementType::TrustedIdentity
            // TODO provisioned 0 length
            | DataElementType::ProvisionedIdentity => {
                if !Self::VALID_ENCRYPTED_IDENTITY_DE_HEADER_LEN.contains(&header_len.as_usize()) {
                    Err(DeLengthOutOfRange)
                } else {
                    header_len
                        .len
                        .checked_add(16)
                        .ok_or(DeLengthOutOfRange)
                        .and_then(|n| n.try_into())
                }
            }
            DataElementType::PublicIdentity => {
                if header_len.len != 0 {
                    Err(DeLengthOutOfRange)
                } else {
                    header_len.len.try_into()
                }
            }
            DataElementType::TxPower => {
                if header_len.len != 1 {
                    Err(DeLengthOutOfRange)
                } else {
                    header_len.len.try_into()
                }
            }
            DataElementType::Actions => {
                if !ActionsDataElement::<Plaintext>::ACTIONS_LEN.contains(&header_len.as_usize()) {
                    Err(DeLengthOutOfRange)
                } else {
                    header_len.len.try_into()
                }
            }
        }
    }
}
