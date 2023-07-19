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

extern crate alloc;
use alloc::vec::Vec;

/// Contains traits for the AES-GCM-SIV AEAD algorithm.
pub mod aes_gcm_siv;

/// Error returned on unsuccessful AEAD operation.
pub struct AeadError;

/// Authenticated Encryption with Associated Data (AEAD) algorithm, where `N` is the size of the
/// Nonce. Encrypts and decrypts buffers in-place.
pub trait Aead {
    /// The size of the authentication tag, this is appended to the message on the encrypt operation
    /// and truncated from the plaintext after decrypting.
    const TAG_SIZE: usize;

    /// The cryptographic nonce used by the AEAD. The nonce must be unique for all messages with
    /// the same key. This is critically important - nonce reuse may completely undermine the
    /// security of the AEAD. Nonces may be predictable and public, so long as they are unique.
    type Nonce;

    /// The key material used to initialize the AEAD.
    type Key;

    /// Instantiates a new instance of the AEAD from key material.
    fn new(key: &Self::Key) -> Self;

    /// Encrypt the given buffer containing a plaintext message in-place. On success increases the
    /// buffer by `Self::TAG_SIZE` bytes and appends the auth tag to the end of `msg`.
    fn encrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &Self::Nonce) -> Result<(), AeadError>;

    /// Decrypt the message in-place, returning an error in the event the provided authentication
    /// tag does not match the given ciphertext. The buffer will be truncated to the length of the
    /// original plaintext message upon success.
    fn decrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &Self::Nonce) -> Result<(), AeadError>;
}
