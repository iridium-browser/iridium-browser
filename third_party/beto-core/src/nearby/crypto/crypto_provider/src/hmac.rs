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

/// Trait which defines hmac operations
pub trait Hmac<const N: usize>: Sized {
    /// Create a new hmac from a fixed size key
    fn new_from_key(key: [u8; N]) -> Self;

    /// Create new hmac value from variable size key.
    fn new_from_slice(key: &[u8]) -> Result<Self, InvalidLength>;

    /// Update state using the provided data
    fn update(&mut self, data: &[u8]);

    /// Obtain the hmac computation consuming the hmac instance
    fn finalize(self) -> [u8; N];

    /// Check that the tag value is correct for the processed input
    fn verify_slice(self, tag: &[u8]) -> Result<(), MacError>;

    /// Check that the tag value is correct for the processed input
    fn verify(self, tag: [u8; N]) -> Result<(), MacError>;

    /// Check truncated tag correctness using left side bytes of the calculated tag
    fn verify_truncated_left(self, tag: &[u8]) -> Result<(), MacError>;
}

/// Error type for when the output of the hmac operation
/// is not equal to the expected value.
#[derive(Debug, PartialEq, Eq)]
pub struct MacError;

/// Error output when the provided key material length is invalid
#[derive(Debug)]
pub struct InvalidLength;
