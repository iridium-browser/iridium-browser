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

#![no_std]
#![forbid(unsafe_code)]
#![deny(
    missing_docs,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used,
    clippy::indexing_slicing
)]

//! Implementation of the XTS-AES tweakable block cipher.
//!
//! See NIST docs [here](https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/1619-2007-NIST-Submission.pdf)
//! and [here](https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38e.pdf).

#[cfg(feature = "std")]
extern crate std;

use array_ref::{array_mut_ref, array_ref};
use core::fmt;
use core::marker::PhantomData;

use crypto_provider::aes::{Aes, AesCipher, AesDecryptCipher, AesEncryptCipher};
use crypto_provider::{
    aes::{AesKey, BLOCK_SIZE},
    CryptoProvider,
};

use ldt_tbc::{
    TweakableBlockCipher, TweakableBlockCipherDecrypter, TweakableBlockCipherEncrypter,
    TweakableBlockCipherKey,
};

#[cfg(test)]
mod tweak_tests;

/// XTS-AES as per NIST docs
/// [here](https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/1619-2007-NIST-Submission.pdf)
/// and
/// [here](https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38e.pdf).
///
/// Data encrypted with XTS is divided into data units, each of which corresponds to one tweak
/// (assigned consecutively). If using a numeric tweak (`u128`), the tweak must be converted to
/// little endian bytes (e.g. with `u128::to_le_bytes()`).
///
/// Inside each data unit, there are one or more 16-byte blocks. The length of a data unit SHOULD
/// not to exceed 2^20 blocks (16GiB) in order to maintain security properties; see
/// [appendix D](https://luca-giuzzi.unibs.it/corsi/Support/papers-cryptography/1619-2007-NIST-Submission.pdf).
/// The last block (if there is more than one block) may have fewer than 16 bytes, in which case
/// ciphertext stealing will be applied.
///
/// Each block in a data unit has its own block number, generated consecutively, incorporated into
/// the tweak used to encrypt that block.
///
/// There is no support for partial bytes (bit lengths that aren't a multiple of 8).
pub struct XtsAes<A: Aes<Key = K::BlockCipherKey>, K: XtsKey + TweakableBlockCipherKey> {
    _marker: PhantomData<A>,
    _marker2: PhantomData<K>,
}

impl<A: Aes<Key = K::BlockCipherKey>, K: XtsKey + TweakableBlockCipherKey>
    TweakableBlockCipher<BLOCK_SIZE> for XtsAes<A, K>
{
    type EncryptionCipher = XtsEncrypter<A, K>;
    type DecryptionCipher = XtsDecrypter<A, K>;
    type Tweak = Tweak;
    type Key = K;
}

/// The XtsAes128 implementation
pub type XtsAes128<C> = XtsAes<<C as CryptoProvider>::Aes128, XtsAes128Key>;

/// The XtsAes256 implementation
pub type XtsAes256<C> = XtsAes<<C as CryptoProvider>::Aes256, XtsAes256Key>;

/// Struct which provides Xts Aes Encrypt operations
#[repr(C)]
pub struct XtsEncrypter<A: Aes<Key = K::BlockCipherKey>, K: XtsKey> {
    main_encryption_cipher: A::EncryptCipher,
    tweak_encryption_cipher: A::EncryptCipher,
    _marker: PhantomData<K>,
}

/// Struct which provides Xts Aes Decrypt operations
#[repr(C)]
pub struct XtsDecrypter<A: Aes<Key = K::BlockCipherKey>, K: XtsKey> {
    main_decryption_cipher: A::DecryptCipher,
    tweak_encryption_cipher: A::EncryptCipher,
    _marker: PhantomData<K>,
}

#[allow(clippy::expect_used)]
#[allow(clippy::indexing_slicing)]
impl<A: Aes<Key = K::BlockCipherKey>, K: XtsKey> XtsEncrypter<A, K> {
    /// Encrypt a data unit in place, using sequential block numbers for each block.
    /// `data_unit` must be at least [BLOCK_SIZE] bytes, and fewer than
    /// `BLOCK_SIZE * 2^20` bytes.
    pub fn encrypt_data_unit(&self, tweak: Tweak, data_unit: &mut [u8]) -> Result<(), XtsError> {
        let (standalone_blocks, last_complete_block, partial_last_block) =
            data_unit_parts(data_unit)?;

        let mut tweaked_xts = self.tweaked(tweak);

        standalone_blocks.chunks_mut(BLOCK_SIZE).for_each(|block| {
            // Until array_chunks is stabilized, using a macro to get array refs out of slice chunks
            // Won't panic because we are only processing complete blocks
            tweaked_xts.encrypt_block(array_mut_ref!(block, 0, BLOCK_SIZE));
            tweaked_xts.advance_to_next_block_num();
        });

        if partial_last_block.is_empty() {
            tweaked_xts.encrypt_block(array_mut_ref!(last_complete_block, 0, BLOCK_SIZE));
            // nothing to do for the last block since it's empty
        } else {
            // b is in bytes not bits; we do not consider partial bytes
            let b = partial_last_block.len();

            // Per spec: CC = encrypt P_{m-1} (the last complete block)
            let cc = {
                let mut last_complete_block_plaintext: crypto_provider::aes::AesBlock =
                    last_complete_block.try_into().expect("complete block");
                tweaked_xts.encrypt_block(&mut last_complete_block_plaintext);
                tweaked_xts.advance_to_next_block_num();
                last_complete_block_plaintext
            };

            // Copy b bytes of P_m before we overwrite them with C_m
            let mut partial_last_block_plaintext: crypto_provider::aes::AesBlock = [0; BLOCK_SIZE];
            partial_last_block_plaintext
                .get_mut(0..b)
                .expect("this should never be hit, since a partial block is always smaller than a complete block")
                .copy_from_slice(partial_last_block);

            // C_m = first b bytes of CC
            partial_last_block.copy_from_slice(
                cc.get(0..b)
                    .expect("partial block len should always be less than a complete block"),
            );

            // PP = P_m || last (16 - b) bytes of CC
            let mut pp = {
                // the first b bytes have already been written as C_m, so it's safe to overwrite
                let mut cc = cc;
                cc.get_mut(0..b)
                    .expect("partial block len should always be less than a complete block")
                    .copy_from_slice(
                        partial_last_block_plaintext
                            .get(0..b)
                            .expect("b is in range of block length"),
                    );
                cc
            };

            // C_{m-1} = encrypt PP
            tweaked_xts.encrypt_block(&mut pp);
            last_complete_block.copy_from_slice(&pp[..]);
        }

        Ok(())
    }

    /// Returns an [XtsTweaked] configured with the specified tweak and a block number of 0.
    fn tweaked(&self, tweak: Tweak) -> XtsEncrypterTweaked<A> {
        let mut bytes = tweak.bytes;
        self.tweak_encryption_cipher.encrypt(&mut bytes);

        XtsEncrypterTweaked {
            tweak_state: TweakState::new(bytes),
            enc_cipher: &self.main_encryption_cipher,
        }
    }
}

#[allow(clippy::expect_used)]
#[allow(clippy::indexing_slicing)]
impl<A: Aes<Key = K::BlockCipherKey>, K: XtsKey> XtsDecrypter<A, K> {
    /// Decrypt a data unit in place, using sequential block numbers for each block.
    /// `data_unit` must be at least [BLOCK_SIZE] bytes, and fewer than
    /// `BLOCK_SIZE * 2^20` bytes.
    pub fn decrypt_data_unit(&self, tweak: Tweak, data_unit: &mut [u8]) -> Result<(), XtsError> {
        let (standalone_blocks, last_complete_block, partial_last_block) =
            data_unit_parts(data_unit)?;

        let mut tweaked_xts = self.tweaked(tweak);

        standalone_blocks.chunks_mut(BLOCK_SIZE).for_each(|block| {
            tweaked_xts.decrypt_block(array_mut_ref!(block, 0, BLOCK_SIZE));
            tweaked_xts.advance_to_next_block_num();
        });

        if partial_last_block.is_empty() {
            tweaked_xts.decrypt_block(array_mut_ref!(last_complete_block, 0, BLOCK_SIZE));
        } else {
            let b = partial_last_block.len();

            // tweak state is currently at m-1 block, so capture m-1 state for later use
            let tweak_state_m_1 = tweaked_xts.current_tweak();

            // Per spec: PP = encrypt C_{m-1} (the last complete block) at block num m
            let pp = {
                tweaked_xts.advance_to_next_block_num();
                // Block num is now at m
                // We need C_m later, so make a copy to avoid overwriting
                let mut last_complete_block_ciphertext: crypto_provider::aes::AesBlock =
                    last_complete_block.try_into().expect("complete block");
                tweaked_xts.decrypt_block(&mut last_complete_block_ciphertext);
                tweaked_xts.set_tweak(tweak_state_m_1);
                last_complete_block_ciphertext
            };

            // Copy b bytes of C_m before we overwrite them with P_m
            let mut partial_last_block_ciphertext: crypto_provider::aes::AesBlock = [0; BLOCK_SIZE];
            partial_last_block_ciphertext[0..b].copy_from_slice(partial_last_block);

            // P_m = first b bytes of PP
            partial_last_block.copy_from_slice(&pp[0..b]);

            // CC = C_m | CP (last 16-b bytes of PP)
            let cc = {
                let cp = &pp[b..];
                last_complete_block[0..b].copy_from_slice(&partial_last_block_ciphertext[0..b]);
                last_complete_block[b..].copy_from_slice(cp);
                last_complete_block
            };

            // decrypting at block num = m -1
            tweaked_xts.decrypt_block(array_mut_ref!(cc, 0, BLOCK_SIZE));
        }

        Ok(())
    }

    /// Returns an [XtsTweaked] configured with the specified tweak and a block number of 0.
    fn tweaked(&self, tweak: Tweak) -> XtsDecrypterTweaked<A> {
        let mut bytes = tweak.bytes;
        self.tweak_encryption_cipher.encrypt(&mut bytes);

        XtsDecrypterTweaked {
            tweak_state: TweakState::new(bytes),
            dec_cipher: &self.main_decryption_cipher,
        }
    }
}

type DataUnitPartsResult<'a> = Result<(&'a mut [u8], &'a mut [u8], &'a mut [u8]), XtsError>;

/// Returns `(standalone blocks, last complete block, partial last block)`.
fn data_unit_parts(data_unit: &mut [u8]) -> DataUnitPartsResult {
    if data_unit.len() < BLOCK_SIZE {
        return Err(XtsError::DataTooShort);
    } else if data_unit.len() > MAX_XTS_SIZE {
        return Err(XtsError::DataTooLong);
    }
    // complete_blocks >= 1
    let complete_blocks = data_unit.len() / BLOCK_SIZE;
    // standalone_units >= 0 blocks, suffix = last complete block + possible partial block.
    let (standalone_blocks, suffix) = data_unit.split_at_mut((complete_blocks - 1) * BLOCK_SIZE);
    let (last_complete_block, partial_last_block) = suffix.split_at_mut(BLOCK_SIZE);
    Ok((standalone_blocks, last_complete_block, partial_last_block))
}

impl<A: Aes<Key = K::BlockCipherKey>, K: XtsKey + TweakableBlockCipherKey>
    TweakableBlockCipherEncrypter<BLOCK_SIZE> for XtsEncrypter<A, K>
{
    type Key = K;
    type Tweak = Tweak;

    /// Build an [XtsEncrypter] with the provided [Aes] and the provided key.
    fn new(key: &Self::Key) -> Self {
        XtsEncrypter {
            main_encryption_cipher: A::EncryptCipher::new(key.key_1()),
            tweak_encryption_cipher: A::EncryptCipher::new(key.key_2()),
            _marker: Default::default(),
        }
    }

    #[allow(clippy::expect_used)]
    fn encrypt(&self, tweak: Self::Tweak, block: &mut [u8; 16]) {
        // we're encrypting precisely one block, so the block number won't advance, and ciphertext
        // stealing will not be applied.
        self.encrypt_data_unit(tweak, block).expect("One block is a valid size");
    }
}

impl<A: Aes<Key = K::BlockCipherKey>, K: XtsKey + TweakableBlockCipherKey>
    TweakableBlockCipherDecrypter<BLOCK_SIZE> for XtsDecrypter<A, K>
{
    type Key = K;
    type Tweak = Tweak;

    fn new(key: &K) -> Self {
        XtsDecrypter {
            main_decryption_cipher: A::DecryptCipher::new(key.key_1()),
            tweak_encryption_cipher: A::EncryptCipher::new(key.key_2()),
            _marker: Default::default(),
        }
    }

    #[allow(clippy::expect_used)]
    fn decrypt(&self, tweak: Self::Tweak, block: &mut [u8; 16]) {
        self.decrypt_data_unit(tweak, block).expect("One block is a valid size");
    }
}

/// Errors that can occur during XTS encryption/decryption.
#[derive(Debug, PartialEq, Eq)]
pub enum XtsError {
    /// The data is less than one AES block
    DataTooShort,
    /// The data is longer than 2^20 blocks, at which point XTS security degrades
    DataTooLong,
}

/// XTS spec recommends to not go beyond 2^20 blocks.
const MAX_XTS_SIZE: usize = (1 << 20) * BLOCK_SIZE;

/// An XTS key comprised of two keys for the underlying block cipher.
pub trait XtsKey: for<'a> TryFrom<&'a [u8], Error = Self::TryFromError> {
    /// The key used by the block cipher underlying XTS
    type BlockCipherKey;
    /// The error returned when `TryFrom<&[u8]>` fails.
    type TryFromError: fmt::Debug;

    /// Returns the first of the two block cipher keys.
    fn key_1(&self) -> &Self::BlockCipherKey;
    /// Returns the second of the two block cipher keys.
    fn key_2(&self) -> &Self::BlockCipherKey;
}

/// An XTS-AES-128 key.
pub struct XtsAes128Key {
    key_1: <Self as XtsKey>::BlockCipherKey,
    key_2: <Self as XtsKey>::BlockCipherKey,
}

impl XtsKey for XtsAes128Key {
    type BlockCipherKey = crypto_provider::aes::Aes128Key;
    type TryFromError = XtsKeyTryFromSliceError;

    fn key_1(&self) -> &Self::BlockCipherKey {
        &self.key_1
    }

    fn key_2(&self) -> &Self::BlockCipherKey {
        &self.key_2
    }
}

impl TryFrom<&[u8]> for XtsAes128Key {
    type Error = XtsKeyTryFromSliceError;

    fn try_from(slice: &[u8]) -> Result<Self, Self::Error> {
        try_split_concat_key::<16>(slice)
            .map(|(key_1, key_2)| Self { key_1: key_1.into(), key_2: key_2.into() })
            .ok_or_else(XtsKeyTryFromSliceError::new)
    }
}

#[allow(clippy::expect_used)]
impl From<&[u8; 32]> for XtsAes128Key {
    fn from(array: &[u8; 32]) -> Self {
        let arr1: [u8; 16] = array[..16].try_into().expect("array is correctly sized");
        let arr2: [u8; 16] = array[16..].try_into().expect("array is correctly sized");
        Self {
            key_1: crypto_provider::aes::Aes128Key::from(arr1),
            key_2: crypto_provider::aes::Aes128Key::from(arr2),
        }
    }
}

impl TweakableBlockCipherKey for XtsAes128Key {
    type ConcatenatedKeyArray = [u8; 64];

    // Allow index slicing, since a panic will be impossible to hit
    #[allow(clippy::indexing_slicing)]
    fn split_from_concatenated(key: &Self::ConcatenatedKeyArray) -> (Self, Self) {
        ((array_ref!(key, 0, 32)).into(), (array_ref!(key, 32, 32)).into())
    }

    fn concatenate_with(&self, other: &Self) -> Self::ConcatenatedKeyArray {
        let mut out = [0; 64];
        out[..16].copy_from_slice(self.key_1().as_slice());
        out[16..32].copy_from_slice(self.key_2().as_slice());
        out[32..48].copy_from_slice(other.key_1().as_slice());
        out[48..].copy_from_slice(other.key_2().as_slice());

        out
    }
}

/// An XTS-AES-256 key.
pub struct XtsAes256Key {
    key_1: <Self as XtsKey>::BlockCipherKey,
    key_2: <Self as XtsKey>::BlockCipherKey,
}

impl XtsKey for XtsAes256Key {
    type BlockCipherKey = crypto_provider::aes::Aes256Key;
    type TryFromError = XtsKeyTryFromSliceError;

    fn key_1(&self) -> &Self::BlockCipherKey {
        &self.key_1
    }

    fn key_2(&self) -> &Self::BlockCipherKey {
        &self.key_2
    }
}

impl TryFrom<&[u8]> for XtsAes256Key {
    type Error = XtsKeyTryFromSliceError;

    fn try_from(slice: &[u8]) -> Result<Self, Self::Error> {
        try_split_concat_key::<32>(slice)
            .map(|(key_1, key_2)| Self { key_1: key_1.into(), key_2: key_2.into() })
            .ok_or_else(XtsKeyTryFromSliceError::new)
    }
}

#[allow(clippy::expect_used)]
impl From<&[u8; 64]> for XtsAes256Key {
    fn from(array: &[u8; 64]) -> Self {
        let arr1: [u8; 32] = array[..32].try_into().expect("array is correctly sized");
        let arr2: [u8; 32] = array[32..].try_into().expect("array is correctly sized");
        Self {
            key_1: crypto_provider::aes::Aes256Key::from(arr1),
            key_2: crypto_provider::aes::Aes256Key::from(arr2),
        }
    }
}

impl TweakableBlockCipherKey for XtsAes256Key {
    type ConcatenatedKeyArray = [u8; 128];

    // Allow index slicing, since a panic will be impossible to hit
    #[allow(clippy::indexing_slicing)]
    fn split_from_concatenated(key: &Self::ConcatenatedKeyArray) -> (Self, Self) {
        ((array_ref!(key, 0, 64)).into(), (array_ref!(key, 64, 64)).into())
    }

    fn concatenate_with(&self, other: &Self) -> Self::ConcatenatedKeyArray {
        let mut out = [0; 128];
        out[..32].copy_from_slice(self.key_1().as_slice());
        out[32..64].copy_from_slice(self.key_2().as_slice());
        out[64..96].copy_from_slice(other.key_1().as_slice());
        out[96..].copy_from_slice(other.key_2().as_slice());

        out
    }
}

/// The error returned when converting from a slice fails.
#[derive(Debug)]
pub struct XtsKeyTryFromSliceError {
    _private: (),
}

impl XtsKeyTryFromSliceError {
    fn new() -> Self {
        Self { _private: () }
    }
}

/// The tweak for an XTS-AES cipher.
#[derive(Clone)]
pub struct Tweak {
    bytes: crypto_provider::aes::AesBlock,
}

impl Tweak {
    /// Little-endian content of the tweak.
    pub fn le_bytes(&self) -> crypto_provider::aes::AesBlock {
        self.bytes
    }
}

impl From<crypto_provider::aes::AesBlock> for Tweak {
    fn from(bytes: crypto_provider::aes::AesBlock) -> Self {
        Self { bytes }
    }
}

impl From<u128> for Tweak {
    fn from(n: u128) -> Self {
        Self { bytes: n.to_le_bytes() }
    }
}

/// An XTS tweak advanced to a particular block num.
#[derive(Clone)]
pub(crate) struct TweakState {
    /// The block number inside the data unit. Should not exceed 2^20.
    block_num: u32,
    /// Original tweak multiplied by the primitive polynomial `block_num` times as per section 5.2
    tweak: crypto_provider::aes::AesBlock,
}

impl TweakState {
    /// Create a TweakState from the provided state with block_num = 0.
    fn new(tweak: [u8; 16]) -> TweakState {
        TweakState { block_num: 0, tweak }
    }

    /// Advance the tweak state in the data unit to the `block_num`'th block without encrypting
    /// or decrypting the intermediate blocks.
    ///
    /// `block_num` should not exceed 2^20.
    ///
    /// # Panics
    /// - If `block_num` is less than the current block num
    fn advance_to_block(&mut self, block_num: u32) {
        // It's a programmer error; nothing to recover from
        assert!(self.block_num <= block_num);

        let mut target = [0_u8; BLOCK_SIZE];

        // Multiply by the primitive polynomial as many times as needed, as per section 5.2
        // of IEEE spec
        #[allow(clippy::expect_used)]
        for _ in 0..(block_num - self.block_num) {
            // Conceptual left shift across the bytes.
            // Most significant byte: if shift would carry, XOR in the coefficients of primitive
            // polynomial in F_2^128 (x^128 = x^7 + x^2 + x + 1 = 0) = 135 decimal.
            // % 128 is compiled as & !128 (i.e. fast).
            target[0] = (2
                * (self.tweak.first().expect("aes block must have non zero length") % 128))
                ^ (135
                    * select_hi_bit(
                        *self.tweak.get(15).expect("15 is a valid index in an aes block"),
                    ));
            // Remaining bytes
            for (j, byte) in target.iter_mut().enumerate().skip(1) {
                *byte = (2
                    * (self.tweak.get(j).expect("j is always in range of block size") % 128))
                    ^ select_hi_bit(
                        *self.tweak.get(j - 1).expect("j > 0 always because of the .skip(1)"),
                    );
            }
            self.tweak = target;
            // no need to zero target as it will be overwritten completely next iteration
        }

        self.block_num = block_num;
    }
}

/// An XTS-AES cipher configured with an initial tweak that can be advanced through the block
/// numbers for that tweak's data unit.
///
/// Encryption or decryption is per-block only; ciphertext stealing is not implemented at this
/// level.
struct XtsEncrypterTweaked<'a, A: Aes> {
    tweak_state: TweakState,
    enc_cipher: &'a A::EncryptCipher,
}

impl<'a, A: Aes> XtsEncrypterTweaked<'a, A> {
    fn advance_to_next_block_num(&mut self) {
        self.tweak_state.advance_to_block(self.tweak_state.block_num + 1)
    }

    /// Encrypt a block in place using the configured tweak and current block number.
    fn encrypt_block(&self, block: &mut crypto_provider::aes::AesBlock) {
        array_xor(block, &self.tweak_state.tweak);
        self.enc_cipher.encrypt(block);
        array_xor(block, &self.tweak_state.tweak);
    }
}

/// An XTS-AES cipher configured with an initial tweak that can be advanced through the block
/// numbers for that tweak's data unit.
///
/// Encryption or decryption is per-block only; ciphertext stealing is not implemented at this
/// level.
struct XtsDecrypterTweaked<'a, A: Aes> {
    tweak_state: TweakState,
    dec_cipher: &'a A::DecryptCipher,
}

impl<'a, A: Aes> XtsDecrypterTweaked<'a, A> {
    fn advance_to_next_block_num(&mut self) {
        self.tweak_state.advance_to_block(self.tweak_state.block_num + 1)
    }

    /// Get the current tweak state -- useful if needed to reset to an earlier block num.
    fn current_tweak(&self) -> TweakState {
        self.tweak_state.clone()
    }

    /// Set the tweak to a state captured via [current_tweak].
    fn set_tweak(&mut self, tweak_state: TweakState) {
        self.tweak_state = tweak_state;
    }
    fn decrypt_block(&self, block: &mut crypto_provider::aes::AesBlock) {
        // CC = C ^ T
        array_xor(block, &self.tweak_state.tweak);
        // PP = decrypt CC
        self.dec_cipher.decrypt(block);
        // P = PP ^ T
        array_xor(block, &self.tweak_state.tweak);
    }
}

/// Calculate `base = base ^ rhs` for each byte.
#[allow(clippy::expect_used)]
fn array_xor(base: &mut crypto_provider::aes::AesBlock, rhs: &crypto_provider::aes::AesBlock) {
    // hopefully this gets done smartly by the compiler (intel pxor, arm veorq, or equivalent).
    // This seems to happen in practice at opt level 3: https://gcc.godbolt.org/z/qvjE8joMv
    for i in 0..BLOCK_SIZE {
        *base.get_mut(i).expect("i is always a valid index for an AesBlock") ^=
            rhs.get(i).expect("i is always a valid index for an AesBlock");
    }
}

/// 1 if hi bit set, 0 if not.
fn select_hi_bit(byte: u8) -> u8 {
    // compiled as shr 7: https://gcc.godbolt.org/z/1rzvfshnx
    byte / 128
}

fn try_split_concat_key<const N: usize>(slice: &[u8]) -> Option<([u8; N], [u8; N])> {
    slice.get(0..N).and_then(|slice| slice.try_into().ok()).and_then(|k1: [u8; N]| {
        slice.get(N..).and_then(|slice| slice.try_into().ok()).map(|k2: [u8; N]| (k1, k2))
    })
}
