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

//! A no_std friendly array wrapper to expose a variable length prefix of the array.
#![no_std]
#![forbid(unsafe_code)]
#![deny(
    missing_docs,
    clippy::indexing_slicing,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used
)]

#[cfg(feature = "std")]
extern crate std;

use core::{borrow, fmt};

/// A view into the first `len` elements of an array.
///
/// Useful when you have a fixed size array but variable length data that fits in it.
#[derive(PartialEq, Eq, Clone)]
pub struct ArrayView<T, const N: usize> {
    array: [T; N],
    len: usize,
}

// manual impl to avoid showing parts of the buffer beyond len
impl<T: fmt::Debug, const N: usize> fmt::Debug for ArrayView<T, N> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.as_slice().fmt(f)
    }
}

impl<T, const N: usize> ArrayView<T, N> {
    /// Destructures this ArrayView into a (length, payload) pair.
    pub fn into_raw_parts(self) -> (usize, [T; N]) {
        (self.len, self.array)
    }
    /// A version of [`ArrayView#try_from_array`] which panics if `len > buffer.len()`,
    /// suitable for usage in `const` contexts.
    pub const fn const_from_array(array: [T; N], len: usize) -> ArrayView<T, N> {
        if N < len {
            panic!("Invalid const ArrayView");
        } else {
            ArrayView { array, len }
        }
    }

    /// Create an [ArrayView] of the first `len` elements of `buffer`.
    ///
    /// Returns `None` if `len > buffer.len()`.
    pub fn try_from_array(array: [T; N], len: usize) -> Option<ArrayView<T, N>> {
        if N < len {
            None
        } else {
            Some(ArrayView { array, len })
        }
    }

    /// Returns the prefix of the array as a slice.
    pub fn as_slice(&self) -> &[T] {
        &self.array[..self.len]
    }

    /// The length of the data in the view
    pub fn len(&self) -> usize {
        self.len
    }

    /// Returns true if the length is 0
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }
}

impl<T: Default + Copy, const N: usize> ArrayView<T, N> {
    /// Create an `ArrayView` containing the data from the provided slice, assuming the slice can
    /// fit in the array size.
    pub fn try_from_slice(slice: &[T]) -> Option<ArrayView<T, N>> {
        if N < slice.len() {
            None
        } else {
            let mut array = [T::default(); N];
            array[..slice.len()].copy_from_slice(slice);
            Some(ArrayView { array, len: slice.len() })
        }
    }
}

impl<T, const N: usize> AsRef<[T]> for ArrayView<T, N> {
    fn as_ref(&self) -> &[T] {
        self.as_slice()
    }
}

impl<T, const N: usize> borrow::Borrow<[T]> for ArrayView<T, N> {
    fn borrow(&self) -> &[T] {
        self.as_slice()
    }
}

#[cfg(test)]
mod tests {
    #![allow(clippy::unwrap_used)]

    extern crate std;
    use crate::ArrayView;
    use std::format;

    #[test]
    fn debug_only_shows_len_elements() {
        assert_eq!(
            "[1, 2]",
            &format!("{:?}", ArrayView::try_from_array([1, 2, 3, 4, 5], 2).unwrap())
        );
    }

    #[test]
    fn try_from_slice_too_long() {
        assert_eq!(None, ArrayView::<u8, 3>::try_from_slice(&[1, 2, 3, 4, 5]));
    }

    #[test]
    fn try_from_slice_ok() {
        let view = ArrayView::<u8, 10>::try_from_slice(&[1, 2, 3, 4, 5]).unwrap();
        assert_eq!(&[1, 2, 3, 4, 5], view.as_slice())
    }
}
