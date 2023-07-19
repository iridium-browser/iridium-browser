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

/// Trait which defines hkdf operations
pub trait Hkdf {
    /// Creates a new instance of an hkdf from a salt and key material
    fn new(salt: Option<&[u8]>, ikm: &[u8]) -> Self;

    /// The RFC5869 HKDF-Expand operation. The info argument for the expand is set to
    /// the concatenation of all the elements of info_components
    fn expand_multi_info(
        &self,
        info_components: &[&[u8]],
        okm: &mut [u8],
    ) -> Result<(), InvalidLength>;

    /// The RFC5869 HKDF-Expand operation.
    fn expand(&self, info: &[u8], okm: &mut [u8]) -> Result<(), InvalidLength>;
}

/// Error type returned from the hkdf expand operations when the output key material has
/// an invalid length
#[derive(Debug)]
pub struct InvalidLength;
