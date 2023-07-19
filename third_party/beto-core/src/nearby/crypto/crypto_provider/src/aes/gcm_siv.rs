// Copyright 2023 Google LLC
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

//! Traits for AES-GCM-SIV.

use super::AesKey;
extern crate alloc;
use alloc::vec::Vec;

/// An enum for indicating issues with the GCM-SIV encryption/decryption operations.
pub enum GcmSivError {
    /// Returned if the output buffer is too small to store the resulting ciphertext + tag.
    EncryptOutBufferTooSmall,
    /// Returned if the ciphertext + tag combination does not match when decrypting a blob.
    DecryptTagDoesNotMatch,
}

/// An implementation of AES-GCM-SIV.
///
/// An AesGcmSiv impl may be used for encryption and decryption.
pub trait AesGcmSiv {
    /// The [AesKey] this cipher uses. See [super::Aes128Key] and [super::Aes256Key] for the common AES-128 and
    /// AES-256 cases.
    type Key: AesKey;

    /// Build a `Self` from key material.
    fn new(key: &Self::Key) -> Self;

    /// Encrypt the data in place with a nonce to make sure each ciphertext is unique.
    /// This will need 16 bytes reserved in the data array for the tag.
    /// Optionally, additional associated data can be passed in for computation of the cryptographic tag.
    fn encrypt(&self, data: &mut Vec<u8>, aad: &[u8], nonce: &[u8]) -> Result<(), GcmSivError>;
    /// Decrypt the ciphertext concatenated with its tag in place with the nonce used for encryption.
    /// If associated data was passed in when creating the ciphertext, it should be passed in here as well
    /// in order to properly decrypt the message.
    fn decrypt(&self, data: &mut Vec<u8>, aad: &[u8], nonce: &[u8]) -> Result<(), GcmSivError>;
}
