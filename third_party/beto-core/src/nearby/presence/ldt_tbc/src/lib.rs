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
#![deny(
    missing_docs,
    clippy::indexing_slicing,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used
)]

//! Defining traits for an LDT specific Tweakable Block Cipher

#[cfg(feature = "std")]
extern crate std;

use crypto_provider::{CryptoProvider, CryptoRng};

/// The higher level trait defining the single block at a time Tweakable Block Cipher types.
/// Holds associates types for both the [TweakableBlockCipherEncrypter] and corresponding
/// [TweakableBlockCipherDecrypter]
pub trait TweakableBlockCipher<const B: usize> {
    /// The tweakable block cipher encryption cipher
    type EncryptionCipher: TweakableBlockCipherEncrypter<B, Key = Self::Key, Tweak = Self::Tweak>;

    /// The tweakable block cipher decryption cipher
    type DecryptionCipher: TweakableBlockCipherDecrypter<B, Key = Self::Key, Tweak = Self::Tweak>;

    /// The tweak type used with encryption/decryption.
    type Tweak: From<[u8; B]>;

    /// the tweakable block cipher key type for the tbc
    type Key: TweakableBlockCipherKey;
}

/// Trait defining a Tweakable Block Cipher, single block at a time, decrypt operation
/// `B` is the block size in bytes.
pub trait TweakableBlockCipherEncrypter<const B: usize> {
    /// The tweakable block cipher key type for the tbc
    type Key: TweakableBlockCipherKey;
    /// The tweak type used when encrypting
    type Tweak: From<[u8; B]>;
    /// Build a [TweakableBlockCipherEncrypter] with the provided and the provided key.
    fn new(key: &Self::Key) -> Self;
    /// Encrypt `block` in place using the specified `tweak`.
    fn encrypt(&self, tweak: Self::Tweak, block: &mut [u8; B]);
}

/// Trait defining a Tweakable Block Cipher, single block at a time, encrypt operation
/// `B` is the block size in bytes.
pub trait TweakableBlockCipherDecrypter<const B: usize> {
    /// The tweakable block cipher key type for the tbc
    type Key: TweakableBlockCipherKey;
    /// The tweak type used when decrypting
    type Tweak: From<[u8; B]>;
    /// Build a [TweakableBlockCipherDecrypter] with the provided and the provided key.
    fn new(key: &Self::Key) -> Self;
    /// Decrypt `block` in place using the specified `tweak`.
    fn decrypt(&self, tweak: Self::Tweak, block: &mut [u8; B]);
}

/// A tweakable block cipher key as used by LDT
pub trait TweakableBlockCipherKey: Sized {
    /// Two tweakable block cipher keys concatenated, as used by LDT
    type ConcatenatedKeyArray: ConcatenatedKeyArray;

    /// Split a concatenated array of two keys' bytes into individual keys.
    fn split_from_concatenated(key: &Self::ConcatenatedKeyArray) -> (Self, Self);

    /// Concatenate with another key to form an array of both key's bytes.
    fn concatenate_with(&self, other: &Self) -> Self::ConcatenatedKeyArray;
}

/// The array form of two concatenated tweakable block cipher keys.
pub trait ConcatenatedKeyArray: Sized {
    /// Build a concatenated key from a secure RNG.
    fn from_random<C: CryptoProvider>(rng: &mut C::CryptoRng) -> Self;
}

impl ConcatenatedKeyArray for [u8; 64] {
    fn from_random<C: CryptoProvider>(rng: &mut C::CryptoRng) -> Self {
        let mut arr = [0; 64];
        rng.fill(&mut arr);
        arr
    }
}

impl ConcatenatedKeyArray for [u8; 128] {
    fn from_random<C: CryptoProvider>(rng: &mut C::CryptoRng) -> Self {
        let mut arr = [0; 128];
        rng.fill(&mut arr);
        arr
    }
}
