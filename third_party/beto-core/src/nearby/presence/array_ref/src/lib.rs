#![no_std]
// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#![forbid(unsafe_code)]
#![deny(missing_docs)]

//! Crate exposing macros to take array references of slices

#[cfg(feature = "std")]
extern crate std;

/// Generate an array reference to a subset of a slice-able bit of data
/// panics if the provided offset and len are out of range of the array
#[macro_export]
macro_rules! array_ref {
    ($arr:expr, $offset:expr, $len:expr) => {{
        let offset = $offset;
        let slice = &$arr[offset..offset + $len];
        let result: &[u8; $len] =
            slice.try_into().expect("array ref len and offset should be valid for provided array");
        result
    }};
}

/// Generates a mutable array reference to a subset of a slice-able bit of data
/// panics if the provided offset and len are out of range of the array
#[macro_export]
macro_rules! array_mut_ref {
    ($arr:expr, $offset:expr, $len:expr) => {{
        let offset = $offset;
        let slice = &mut $arr[offset..offset + $len];
        let result: &mut [u8; $len] =
            slice.try_into().expect("array ref len and offset should be valid for provided array");
        result
    }};
}
