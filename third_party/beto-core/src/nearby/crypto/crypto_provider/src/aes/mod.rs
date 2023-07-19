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

//! An abstraction layer around AES implementations.
//!
//! The design is an attempt to make it easy to provide implementations that are both idiomatic
//! Rust (e.g. RustCrypto) as well as FFI-backed (e.g. openssl and other C impls).
use core::{array, fmt};

pub mod ctr;

#[cfg(feature = "alloc")]
pub mod cbc;

/// Block size in bytes for AES (and XTS-AES)
pub const BLOCK_SIZE: usize = 16;

/// A single AES block.
pub type AesBlock = [u8; BLOCK_SIZE];

/// Helper trait to enforce encryption and decryption with the same size key
pub trait Aes {
    /// The AES key containing the raw bytes used to for key scheduling
    type Key: AesKey;

    /// The cipher used for encryption
    type EncryptCipher: AesEncryptCipher<Key = Self::Key>;

    /// the cipher used for decryption
    type DecryptCipher: AesDecryptCipher<Key = Self::Key>;
}

/// The base AesCipher trait which describes common operations to both encryption and decryption ciphers
pub trait AesCipher {
    /// The type of the key used which holds the raw bytes used in key scheduling
    type Key: AesKey;

    /// Creates a new cipher from the AesKey
    fn new(key: &Self::Key) -> Self;
}

/// An AES cipher used for encrypting blocks
pub trait AesEncryptCipher: AesCipher {
    /// Encrypt `block` in place.
    fn encrypt(&self, block: &mut AesBlock);
}

/// An AES cipher used for decrypting blocks
pub trait AesDecryptCipher: AesCipher {
    /// Decrypt `block` in place.
    fn decrypt(&self, block: &mut AesBlock);
}

/// An appropriately sized `[u8; N]` array that the key can be constructed from, e.g. `[u8; 16]`
/// for AES-128.
pub trait AesKey: for<'a> TryFrom<&'a [u8], Error = Self::TryFromError> {
    /// The error used by the `TryFrom` implementation used to construct `Self::Array` from a
    /// slice. For the typical case of `Self::Array` being an `[u8; N]`, this would be
    /// `core::array::TryFromSliceError`.
    ///
    /// This is broken out as a separate type to allow the `fmt::Debug` requirement needed for
    /// `expect()`.
    type TryFromError: fmt::Debug;

    /// The byte array type the key can be represented with
    type Array;

    /// Key size in bytes -- must match the length of `Self::KeyBytes`.`
    ///
    /// Unfortunately `KeyBytes` can't reference this const in the type declaration, so it must be
    /// specified separately.
    const KEY_SIZE: usize;

    /// Returns the key material as a slice
    fn as_slice(&self) -> &[u8];

    /// Returns the key material as an array
    fn as_array(&self) -> &Self::Array;
}

/// An AES-128 key.
#[derive(Clone)]
pub struct Aes128Key {
    key: [u8; 16],
}

impl AesKey for Aes128Key {
    type TryFromError = array::TryFromSliceError;
    type Array = [u8; 16];
    const KEY_SIZE: usize = 16;

    fn as_slice(&self) -> &[u8] {
        &self.key
    }

    fn as_array(&self) -> &Self::Array {
        &self.key
    }
}

impl TryFrom<&[u8]> for Aes128Key {
    type Error = array::TryFromSliceError;

    fn try_from(value: &[u8]) -> Result<Self, Self::Error> {
        value.try_into().map(|arr| Self { key: arr })
    }
}

impl From<[u8; 16]> for Aes128Key {
    fn from(arr: [u8; 16]) -> Self {
        Self { key: arr }
    }
}

/// An AES-256 key.
#[derive(Clone)]
pub struct Aes256Key {
    key: [u8; 32],
}

impl AesKey for Aes256Key {
    type TryFromError = array::TryFromSliceError;
    type Array = [u8; 32];
    const KEY_SIZE: usize = 32;

    fn as_slice(&self) -> &[u8] {
        &self.key
    }

    fn as_array(&self) -> &Self::Array {
        &self.key
    }
}

impl TryFrom<&[u8]> for Aes256Key {
    type Error = array::TryFromSliceError;

    fn try_from(value: &[u8]) -> Result<Self, Self::Error> {
        value.try_into().map(|arr| Self { key: arr })
    }
}

impl From<[u8; 32]> for Aes256Key {
    fn from(arr: [u8; 32]) -> Self {
        Self { key: arr }
    }
}
