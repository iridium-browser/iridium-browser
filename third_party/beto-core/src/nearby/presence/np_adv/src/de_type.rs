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

//! DE types that are shared across v0 and v1.

// The same ids are used for v0/legacy and v1/extended
const IDENTITY_DE_TYPES_BY_CODE: [Option<IdentityDataElementType>; 5] = [
    // no need for more elements since identity types are at indices 1-4
    None,                                       // 0b0000
    Some(IdentityDataElementType::Private),     // 0b0001
    Some(IdentityDataElementType::Trusted),     // 0b0010
    Some(IdentityDataElementType::Public),      // 0b0011
    Some(IdentityDataElementType::Provisioned), // 0b0100
];

/// DE types for special DEs that contain other DEs and consume the entire payload of the
/// advertisement.
///
/// Shared between V0 and V1.
///
/// Must not overlap with [PlainDataElementType].
#[derive(strum_macros::EnumIter, Debug, Clone, Copy, PartialEq, Eq)]
#[allow(clippy::enum_variant_names)]
pub(crate) enum IdentityDataElementType {
    Private,
    Trusted,
    Public,
    Provisioned,
}

impl IdentityDataElementType {
    pub(crate) fn as_encrypted_identity_de_type(&self) -> Option<EncryptedIdentityDataElementType> {
        match self {
            IdentityDataElementType::Private => Some(EncryptedIdentityDataElementType::Private),
            IdentityDataElementType::Trusted => Some(EncryptedIdentityDataElementType::Trusted),
            IdentityDataElementType::Public => None,
            IdentityDataElementType::Provisioned => {
                Some(EncryptedIdentityDataElementType::Provisioned)
            }
        }
    }

    /// Type codes for identity DEs are shared between versions 0 and 1.
    pub(crate) fn shared_type_code(&self) -> u8 {
        match self {
            IdentityDataElementType::Private => 0b0001,
            IdentityDataElementType::Trusted => 0b0010,
            IdentityDataElementType::Public => 0b0011,
            IdentityDataElementType::Provisioned => 0b0100,
        }
    }

    pub(crate) fn from_shared_type_code(code: u8) -> Option<Self> {
        IDENTITY_DE_TYPES_BY_CODE.get(code as usize).and_then(|o| *o)
    }
}

/// The identity DE types that support encryption.
#[derive(strum_macros::EnumIter, Debug, Clone, Copy, PartialEq, Eq)]
pub enum EncryptedIdentityDataElementType {
    /// An identity that's shared with other devices where a user has signed in to their identity provider
    Private,
    /// An identity shared via some trust designation (e.g. starred contacts)
    Trusted,
    /// An identity established via a p2p link between two specific devices
    Provisioned,
}

impl EncryptedIdentityDataElementType {
    pub(crate) fn as_identity_data_element_type(&self) -> IdentityDataElementType {
        match self {
            EncryptedIdentityDataElementType::Private => IdentityDataElementType::Private,
            EncryptedIdentityDataElementType::Trusted => IdentityDataElementType::Trusted,
            EncryptedIdentityDataElementType::Provisioned => IdentityDataElementType::Provisioned,
        }
    }
}
