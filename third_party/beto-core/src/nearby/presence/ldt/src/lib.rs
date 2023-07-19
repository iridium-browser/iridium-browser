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

//! Provides an implementation of [LDT](https://eprint.iacr.org/2017/841.pdf).

#![no_std]
#![forbid(unsafe_code)]
#![deny(clippy::indexing_slicing, clippy::unwrap_used, clippy::panic, clippy::expect_used)]

#[cfg(feature = "std")]
extern crate std;

use core::{fmt, marker::PhantomData};
use crypto_provider::CryptoProvider;
use ldt_tbc::{ConcatenatedKeyArray, TweakableBlockCipher, TweakableBlockCipherKey};
use ldt_tbc::{TweakableBlockCipherDecrypter, TweakableBlockCipherEncrypter};

/// Implementation of the [LDT](https://eprint.iacr.org/2017/841.pdf) length doubler encryption cipher.
///
/// `B` is the block size.
/// `T` is the provided implementation of a Tweakable Block Cipher
/// `M` is the implementation of a [pure mix function](https://eprint.iacr.org/2017/841.pdf)
#[repr(C)]
pub struct LdtEncryptCipher<const B: usize, T: TweakableBlockCipher<B>, M: Mix> {
    cipher_1: T::EncryptionCipher,
    cipher_2: T::EncryptionCipher,
    // marker to use `M`
    mix_phantom: PhantomData<M>,
}

impl<const B: usize, T: TweakableBlockCipher<B>, M: Mix> LdtEncryptCipher<B, T, M> {
    /// Create an [LdtEncryptCipher] with the provided Tweakable block cipher and Mix function
    pub fn new(key: &LdtKey<T::Key>) -> Self {
        LdtEncryptCipher {
            cipher_1: T::EncryptionCipher::new(&key.key_1),
            cipher_2: T::EncryptionCipher::new(&key.key_2),
            mix_phantom: PhantomData,
        }
    }

    /// Encrypt `data` in place, performing the pad operation with `padder`.
    ///
    /// Unless you have particular padding needs, use [DefaultPadder].
    ///
    /// # Errors
    /// - if `data` has a length outside of `[B, B * 2)`.
    pub fn encrypt<P: Padder<B, T>>(&self, data: &mut [u8], padder: &P) -> Result<(), LdtError> {
        do_ldt::<B, T, _, _, _, P>(
            data,
            |cipher, tweak, block| cipher.encrypt(tweak, block),
            padder,
            M::forwards,
            &self.cipher_1,
            &self.cipher_2,
        )
    }
}

/// Implementation of the [LDT](https://eprint.iacr.org/2017/841.pdf) length doubler decryption cipher.
///
/// `B` is the block size.
/// `T` is the provided implementation of a Tweakable Block Cipher
/// `M` is the implementation of a [pure mix function](https://eprint.iacr.org/2017/841.pdf)
#[repr(C)]
pub struct LdtDecryptCipher<const B: usize, T: TweakableBlockCipher<B>, M: Mix> {
    cipher_1: T::DecryptionCipher,
    cipher_2: T::DecryptionCipher,
    // marker to use `M`
    mix_phantom: PhantomData<M>,
}

impl<const B: usize, T: TweakableBlockCipher<B>, M: Mix> LdtDecryptCipher<B, T, M> {
    /// Create an [LdtDecryptCipher] with the provided Tweakable block cipher and Mix function
    pub fn new(key: &LdtKey<T::Key>) -> Self {
        LdtDecryptCipher {
            cipher_1: T::DecryptionCipher::new(&key.key_1),
            cipher_2: T::DecryptionCipher::new(&key.key_2),
            mix_phantom: PhantomData,
        }
    }

    /// Decrypt `data` in place, performing the pad operation with `padder`.
    ///
    /// Unless you have particular padding needs, use [DefaultPadder].
    ///
    /// # Errors
    /// - if `data` has a length outside of `[B, B * 2)`.
    pub fn decrypt<P: Padder<B, T>>(&self, data: &mut [u8], padder: &P) -> Result<(), LdtError> {
        do_ldt::<B, T, _, _, _, P>(
            data,
            |cipher, tweak, block| cipher.decrypt(tweak, block),
            padder,
            M::backwards,
            // cipher order swapped for decryption
            &self.cipher_2,
            &self.cipher_1,
        )
    }
}

// internal implementation of ldt cipher operations, re-used by encryption and decryption, by providing
// the corresponding cipher_op and mix operation
fn do_ldt<const B: usize, T, O, C, X, P>(
    data: &mut [u8],
    cipher_op: O,
    padder: &P,
    mix: X,
    first_cipher: &C,
    second_cipher: &C,
) -> Result<(), LdtError>
where
    T: TweakableBlockCipher<B>,
    // Encrypt or decrypt in place with a tweak
    O: Fn(&C, T::Tweak, &mut [u8; B]),
    // Mix a/b into block-sized chunks
    X: Fn(&[u8], &[u8]) -> ([u8; B], [u8; B]),
    P: Padder<B, T>,
{
    if data.len() < B || data.len() >= B * 2 {
        return Err(LdtError::InvalidLength(data.len()));
    }
    let s = data.len() - B;
    debug_assert!(s < B);

    // m1 length B, m2 length s (s < B)
    let (m1, m2) = data.split_at(B);
    debug_assert_eq!(s, m2.len());
    let m1_ciphertext = {
        let mut m1_plaintext = [0_u8; B];
        // m1 is of length B, so no panic
        m1_plaintext[..].copy_from_slice(m1);
        let tweak = padder.pad_tweak(m2);
        cipher_op(first_cipher, tweak, &mut m1_plaintext);
        m1_plaintext
    };
    // |z| = B - s, |m3| = s
    let (z, m3) = m1_ciphertext.split_at(B - s);
    debug_assert_eq!(s, m3.len());
    // c3 and c2 are the last s bytes of their size-B arrays, respectively
    let (mut c3, c2) = mix(m3, m2);
    let c1 = {
        // constructing z || c3 is easy since c3 is already the last s bytes
        c3[0..(B - s)].copy_from_slice(z);
        let mut z_c3 = c3;
        let tweak = padder.pad_tweak(&c2[B - s..]);
        cipher_op(second_cipher, tweak, &mut z_c3);
        z_c3
    };
    let len = data.len();
    data.get_mut(0..B).ok_or(LdtError::InvalidLength(len))?.copy_from_slice(&c1);
    data.get_mut(B..).ok_or(LdtError::InvalidLength(len))?.copy_from_slice(&c2[B - s..]);

    Ok(())
}

/// Errors produced by LDT encryption/decryption.
#[derive(Debug, PartialEq, Eq)]
pub enum LdtError {
    /// Data to encrypt/decrypt is the wrong length -- must be in `[B, 2 * B)` for block size `B`
    /// of the underlying [TweakableBlockCipher].
    InvalidLength(usize),
}

impl fmt::Display for LdtError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            LdtError::InvalidLength(len) => {
                write!(f, "Invalid length ({len}), must be in [block size, 2 * block size)")
            }
        }
    }
}

/// A key for an LDT cipher.
///
/// `T` is the key type used for the underlying tweakable block cipher.
pub struct LdtKey<T: TweakableBlockCipherKey> {
    key_1: T,
    key_2: T,
}

impl<T: TweakableBlockCipherKey> LdtKey<T> {
    /// Build a key from two separate tweakable block cipher keys.
    pub fn from_separate(key_1: T, key_2: T) -> Self {
        Self { key_1, key_2 }
    }

    /// Build a key from an array representing two concatenated tweakable block cipher keys.
    pub fn from_concatenated(key: &T::ConcatenatedKeyArray) -> Self {
        let (key_1, key_2) = T::split_from_concatenated(key);
        Self::from_separate(key_1, key_2)
    }

    /// Build a random key from a secure RNG.
    pub fn from_random<C: CryptoProvider>(rng: &mut C::CryptoRng) -> Self {
        Self::from_concatenated(&ConcatenatedKeyArray::from_random::<C>(rng))
    }

    /// Returns the key material as a concatenated array with the contents of the two tweakable
    /// block cipher keys.
    pub fn as_concatenated(&self) -> T::ConcatenatedKeyArray {
        self.key_1.concatenate_with(&self.key_2)
    }
}

/// A [pure mix function](https://eprint.iacr.org/2017/841.pdf).
pub trait Mix {
    /// Mix `a` and `b`, writing into the last `s` bytes of the output arrays.
    /// `a` and `b` must be the same length `s`, and no longer than the block size `B`.
    /// Must be the inverse of [Mix::backwards].
    fn forwards<const B: usize>(a: &[u8], b: &[u8]) -> ([u8; B], [u8; B]);

    /// Mix `a` and `b`, writing into the last `s` bytes of the output arrays.
    /// `a` and `b` must be the same length, and no longer than the block size `B`.
    /// Must be the inverse of [Mix::forwards].
    fn backwards<const B: usize>(a: &[u8], b: &[u8]) -> ([u8; B], [u8; B]);
}

/// Per section 2.4, swapping `a` and `b` is a valid mix function
pub struct Swap {}
impl Mix for Swap {
    fn forwards<const B: usize>(a: &[u8], b: &[u8]) -> ([u8; B], [u8; B]) {
        debug_assert_eq!(a.len(), b.len());
        // implies b length as well
        debug_assert!(a.len() <= B);
        let mut out1 = [0; B];
        let mut out2 = [0; B];

        let start = B - a.len();
        out1[start..].copy_from_slice(b);
        out2[start..].copy_from_slice(a);
        (out1, out2)
    }

    fn backwards<const B: usize>(a: &[u8], b: &[u8]) -> ([u8; B], [u8; B]) {
        // backwards is the same as forwards.
        Self::forwards(a, b)
    }
}

/// Pads partial-block input into tweaks.
///
/// This is exposed as a separate trait to allow for padding methods beyond the default.
pub trait Padder<const B: usize, T: TweakableBlockCipher<B>> {
    /// Build a tweak for `T` out of `data`.
    /// `data` must be shorter than the tweak size so that some padding can take place.
    ///
    /// # Panics
    ///
    /// Panics if the length of `data` >= the tweak size.
    fn pad_tweak(&self, data: &[u8]) -> T::Tweak;
}

/// The default padding algorithm per section 2 of LDT paper.
#[derive(Default)]
pub struct DefaultPadder;

impl<const B: usize, T: TweakableBlockCipher<B>> Padder<B, T> for DefaultPadder {
    fn pad_tweak(&self, data: &[u8]) -> T::Tweak {
        Self::default_padding(data).into()
    }
}

impl DefaultPadder {
    /// Expand `data` to an array of the appropriate size, and append a 1 bit after the original data.
    ///
    /// `T` is the tweak size to pad to in bytes.
    ///
    /// # Panics
    ///
    /// Panics if the length of the data to pad is >= the tweak size.
    // allow index_slicing, since we are ensuring panic is impossible in assert
    #[allow(clippy::indexing_slicing)]
    fn default_padding<const N: usize>(data: &[u8]) -> [u8; N] {
        // If this assert fails, our LDT impl is broken - always pads data < block size
        assert!(data.len() < N);
        let mut out = [0; N];
        out[0..data.len()].copy_from_slice(data);
        // 0b1 followed by zeros for all remaining bits.
        // Since the array was initialized with 0, nothing left to do.
        out[data.len()] = 128;
        out
    }
}

/// Pads with the default algorithm to the tweak size, then XORs that with the provided bytes.
///
/// This is useful as a means to perturb the cipher output without having to alter the input
/// directly.
///
/// `T` is the tweak size to pad to in bytes
#[derive(Debug, PartialEq, Eq)]
pub struct XorPadder<const T: usize> {
    xor_bytes: [u8; T],
}

impl<const T: usize> From<[u8; T]> for XorPadder<T> {
    fn from(bytes: [u8; T]) -> Self {
        XorPadder { xor_bytes: bytes }
    }
}

impl<const B: usize, T: TweakableBlockCipher<B>> Padder<B, T> for XorPadder<B> {
    fn pad_tweak(&self, data: &[u8]) -> T::Tweak {
        let mut out = DefaultPadder::default_padding::<B>(data);
        debug_assert_eq!(self.xor_bytes.len(), out.len());

        // xor into the padded data
        out.iter_mut()
            .zip(self.xor_bytes.iter())
            .for_each(|(out_byte, xor_byte): (&mut u8, &u8)| *out_byte ^= xor_byte);

        out.into()
    }
}
