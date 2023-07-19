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
use super::{AesBlock, AesKey, BLOCK_SIZE};

/// The number of bytes used for the nonce, with the remaining bytes in a block used as the counter.
///
/// Other lengths may be used, but 12 is a good general purpose choice.
pub const AES_CTR_NONCE_LEN: usize = 12;

/// The nonce portion of the nonce+counter block used by CTR mode.
pub type AesCtrNonce = [u8; AES_CTR_NONCE_LEN];

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
    fn new(key: &Self::Key, nonce_and_counter: NonceAndCounter) -> Self;

    /// Encrypt the data in place, advancing the counter state appropriately.
    fn encrypt(&mut self, data: &mut [u8]);
    /// Decrypt the data in place, advancing the counter state appropriately.
    fn decrypt(&mut self, data: &mut [u8]);
}

/// The combined nonce and counter that CTR increments and encrypts to form the keystream.
pub struct NonceAndCounter {
    block: AesBlock,
}

impl NonceAndCounter {
    /// Appends 4 zero bytes of counter to the nonce
    pub fn from_nonce(nonce: AesCtrNonce) -> Self {
        let mut block = [0; BLOCK_SIZE];
        block[..12].copy_from_slice(nonce.as_slice());
        NonceAndCounter { block }
    }

    /// Initialize from an already concatenated nonce and counter
    // Not recommended for general use, so restricted so only test vectors can use it
    #[cfg(feature = "test_vectors")]
    pub fn from_block(block: AesBlock) -> Self {
        Self { block }
    }

    /// Nonce and counter as an AES block-sized byte array
    pub fn as_block_array(&self) -> AesBlock {
        self.block
    }
}
