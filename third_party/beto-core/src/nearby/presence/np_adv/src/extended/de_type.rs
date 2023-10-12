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

//! V1 DE type types
use crate::de_type::{EncryptedIdentityDataElementType, IdentityDataElementType};

/// Data element types for extended advertisements
#[derive(Debug, PartialEq, Eq, Clone, Copy, Hash)]
pub struct DeType {
    // 4 billion type codes should be enough for anybody
    code: u32,
}

impl DeType {
    /// A `const` equivalent to `From<u32>` since trait methods can't yet be const.
    pub(crate) const fn const_from(value: u32) -> Self {
        Self { code: value }
    }

    /// Returns the type as a u32
    pub fn as_u32(&self) -> u32 {
        self.code
    }
}

impl From<u8> for DeType {
    fn from(value: u8) -> Self {
        DeType { code: value as u32 }
    }
}

impl From<u32> for DeType {
    fn from(value: u32) -> Self {
        DeType { code: value }
    }
}

pub(crate) trait ExtendedDataElementType: Sized {
    /// A type code for use in the DE header.
    fn type_code(&self) -> DeType;
    /// Returns the matching type for the code, else `None`
    fn from_type_code(de_type: DeType) -> Option<Self>;
}

impl ExtendedDataElementType for IdentityDataElementType {
    fn type_code(&self) -> DeType {
        DeType::from(self.shared_type_code())
    }

    fn from_type_code(de_type: DeType) -> Option<Self> {
        de_type.code.try_into().ok().and_then(Self::from_shared_type_code)
    }
}

impl ExtendedDataElementType for EncryptedIdentityDataElementType {
    fn type_code(&self) -> DeType {
        DeType::from(self.as_identity_data_element_type().shared_type_code())
    }

    fn from_type_code(code: DeType) -> Option<Self> {
        IdentityDataElementType::from_type_code(code)
            .and_then(|idet| idet.as_encrypted_identity_de_type())
    }
}

#[cfg(test)]
mod tests {
    extern crate std;

    use super::*;
    use std::{collections, fmt};

    #[test]
    fn identity_type_codes_are_consistent() {
        de_type_codes_are_consistent::<IdentityDataElementType>()
    }

    #[test]
    fn encrypted_identity_type_codes_are_consistent() {
        de_type_codes_are_consistent::<EncryptedIdentityDataElementType>()
    }

    #[test]
    fn identity_type_codes_are_distinct() {
        de_distinct_type_codes::<IdentityDataElementType>()
    }

    #[test]
    fn encrypted_identity_type_codes_are_distinct() {
        de_distinct_type_codes::<EncryptedIdentityDataElementType>()
    }

    #[test]
    fn identity_no_accidentally_mapped_type_codes() {
        de_no_accidentally_mapped_type_codes::<IdentityDataElementType>()
    }

    #[test]
    fn encrypted_identity_no_accidentally_mapped_type_codes() {
        de_no_accidentally_mapped_type_codes::<EncryptedIdentityDataElementType>()
    }

    fn de_type_codes_are_consistent<
        D: PartialEq + fmt::Debug + ExtendedDataElementType + strum::IntoEnumIterator,
    >() {
        for det in D::iter() {
            let actual = D::from_type_code(det.type_code());
            assert_eq!(Some(det), actual)
        }
    }

    fn de_distinct_type_codes<
        D: PartialEq + fmt::Debug + ExtendedDataElementType + strum::IntoEnumIterator,
    >() {
        let codes = D::iter().map(|det| det.type_code()).collect::<collections::HashSet<_>>();
        assert_eq!(codes.len(), D::iter().count());
    }

    fn de_no_accidentally_mapped_type_codes<
        D: PartialEq + fmt::Debug + ExtendedDataElementType + strum::IntoEnumIterator,
    >() {
        let codes = D::iter().map(|det| det.type_code()).collect::<collections::HashSet<_>>();
        // not going to try all 4 billion possibilities, but we can make an effort
        for possible_code in 0_u32..100_000 {
            if codes.contains(&possible_code.into()) {
                continue;
            }

            assert_eq!(None, D::from_type_code(possible_code.into()));
        }
    }
}
