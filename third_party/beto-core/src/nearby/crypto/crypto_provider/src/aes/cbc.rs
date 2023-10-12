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

//! Traits for AES-CBC 256 with PKCS7 padding.

extern crate alloc;
use alloc::vec::Vec;

use super::Aes256Key;

/// Type of the initialization vector for AES-CBC
pub type AesCbcIv = [u8; 16];

/// Trait for implementing AES-CBC with PKCS7 padding.
pub trait AesCbcPkcs7Padded {
    /// Encrypt message using `key` and `iv`, returning a ciphertext.
    fn encrypt(key: &Aes256Key, iv: &AesCbcIv, message: &[u8]) -> Vec<u8>;
    /// Decrypt ciphertext using `key` and `iv`, returning the original message if `Ok()` otherwise
    /// a `DecryptionError` indicating the type of error that occurred while decrypting.
    fn decrypt(
        key: &Aes256Key,
        iv: &AesCbcIv,
        ciphertext: &[u8],
    ) -> Result<Vec<u8>, DecryptionError>;
}

/// Error type for describing what went wrong decrypting a ciphertext.
#[derive(Debug, PartialEq, Eq)]
pub enum DecryptionError {
    /// Invalid padding, the input ciphertext does not have valid PKCS7 padding. If you get this
    /// error, check the encryption side generating this data to make sure it is adding the padding
    /// correctly. Exposing padding errors can cause a padding oracle vulnerability.
    BadPadding,
}
