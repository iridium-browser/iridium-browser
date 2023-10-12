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

//! V1 advertisement support.
use crate::extended::de_type::DeType;
use crate::DeLengthOutOfRange;
use array_view::ArrayView;

pub mod data_elements;
pub mod de_type;
pub mod deserialize;
pub mod section_signature_payload;
pub mod serialize;

/// Maximum size of an NP advertisement, including the adv header
pub const BLE_ADV_SVC_CONTENT_LEN: usize = 254
    // length and type bytes for svc data TLV
    - 1 - 1
    // NP UUID
    - 2;

/// Maximum number of sections in an advertisement
pub const NP_V1_ADV_MAX_SECTION_COUNT: usize = 8;

/// Maximum size of a NP section, including its header byte
pub const NP_ADV_MAX_SECTION_LEN: usize = BLE_ADV_SVC_CONTENT_LEN
    // adv header byte
    - 1;

/// Max V1 DE length (7 bit length field).
pub(crate) const MAX_DE_LEN: usize = 127;

const METADATA_KEY_LEN: usize = 16;

/// Length of a DE's content -- must be in `[0, 127]`
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct DeLength {
    len: u8,
}

impl DeLength {
    /// A convenient constant for zero length.
    pub const ZERO: DeLength = DeLength { len: 0 };

    fn as_usize(&self) -> usize {
        self.len as usize
    }
}

impl TryFrom<u8> for DeLength {
    type Error = DeLengthOutOfRange;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        if value as usize <= MAX_DE_LEN {
            Ok(Self { len: value })
        } else {
            Err(DeLengthOutOfRange {})
        }
    }
}

impl TryFrom<usize> for DeLength {
    type Error = DeLengthOutOfRange;

    fn try_from(value: usize) -> Result<Self, Self::Error> {
        value.try_into().map_err(|_e| DeLengthOutOfRange).and_then(|num: u8| num.try_into())
    }
}

/// Convert a tinyvec into an equivalent ArrayView
fn to_array_view<T, const N: usize>(vec: tinyvec::ArrayVec<[T; N]>) -> ArrayView<T, N>
where
    [T; N]: tinyvec::Array,
{
    let len = vec.len();
    ArrayView::try_from_array(vec.into_inner(), len).expect("len is from original vec")
}

pub(crate) const ENCRYPTION_INFO_DE_TYPE: DeType = DeType::const_from(0x10);
