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

//! Traits for AES-CTR.
use super::AesKey;

/// An implementation of AES-CTR.
///
/// An AesCtr impl must only be used for encryption _or_ decryption, not both. Since CTR mode
/// is stateful, mixing encrypts and decrypts may advance the internal state in unexpected ways.
/// Create separate encrypt/decrypt instances if both operations are needed.
pub trait AesCtr {
    /// The [AesKey] this cipher uses. See [super::Aes128Key] and [super::Aes256Key] for the common AES-128 and
    /// AES-256 cases.
    type Key: AesKey;

    /// Build a `Self` from key material.
    fn new(key: &Self::Key, iv: [u8; 16]) -> Self;

    /// Encrypt the data in place, advancing the counter state appropriately.
    fn encrypt(&mut self, data: &mut [u8]);
    /// Decrypt the data in place, advancing the counter state appropriately.
    fn decrypt(&mut self, data: &mut [u8]);
}
