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

//! Salt used in a V1 advertisement.
use crate::np_salt_hkdf;
use core::fmt;
use crypto_provider::hkdf::Hkdf;
use crypto_provider::CryptoProvider;

/// Salt optionally included in V1 advertisement header.
///
/// The salt is never used directly; rather, a derived salt should be extracted as needed for any
/// section or DE that requires it.
#[derive(Clone)]
pub struct V1Salt<C>
where
    C: CryptoProvider,
{
    // kept around for Eq and Debug impl, should not be exposed
    data: [u8; 16],
    hkdf: C::HkdfSha256,
}

impl<C: CryptoProvider> V1Salt<C> {
    /// Derive a salt for a particular section and DE, if applicable.
    ///
    /// Returns none if the requested size is larger than HKDF allows or if offset arithmetic
    /// overflows.
    pub fn derive<const N: usize>(&self, de: Option<DataElementOffset>) -> Option<[u8; N]> {
        let mut arr = [0_u8; N];
        // 0-based offsets -> 1-based indices w/ 0 indicating not present
        self.hkdf
            .expand_multi_info(
                &[
                    b"V1 derived salt",
                    &de.and_then(|d| d.offset.checked_add(1))
                        .and_then(|o| o.try_into().ok())
                        .unwrap_or(0_u32)
                        .to_be_bytes(),
                ],
                &mut arr,
            )
            .map(|_| arr)
            .ok()
    }

    /// Returns the salt bytes as a slice
    pub fn as_slice(&self) -> &[u8] {
        self.data.as_slice()
    }

    /// Returns the salt bytes as a reference to an array
    pub fn as_array_ref(&self) -> &[u8; 16] {
        &self.data
    }
}

impl<C: CryptoProvider> From<[u8; 16]> for V1Salt<C> {
    fn from(arr: [u8; 16]) -> Self {
        Self { data: arr, hkdf: np_salt_hkdf::<C>(&arr) }
    }
}

impl<C: CryptoProvider> PartialEq<Self> for V1Salt<C> {
    fn eq(&self, other: &Self) -> bool {
        // no need to compare hkdf (which it doesn't allow anyway)
        self.data == other.data
    }
}

impl<C: CryptoProvider> Eq for V1Salt<C> {}

impl<C: CryptoProvider> fmt::Debug for V1Salt<C> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.data.fmt(f)
    }
}

/// Offset of a data element in its containing section, used with [V1Salt].
#[derive(PartialEq, Eq, Debug, Clone, Copy, PartialOrd, Ord)]
pub struct DataElementOffset {
    /// 0-based offset of the DE in the advertisement
    offset: usize,
}

impl DataElementOffset {
    /// The zero offset
    pub const ZERO: DataElementOffset = Self { offset: 0 };

    /// Returns the offset as a usize
    pub fn as_usize(&self) -> usize {
        self.offset
    }

    /// Returns the next offset.
    ///
    /// Does not handle overflow as there can't be more than 2^8 DEs in a section.
    pub const fn incremented(&self) -> Self {
        Self { offset: self.offset + 1 }
    }
}

impl From<usize> for DataElementOffset {
    fn from(num: usize) -> Self {
        Self { offset: num }
    }
}
