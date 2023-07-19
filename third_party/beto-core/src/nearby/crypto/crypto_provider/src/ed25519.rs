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

use core::fmt::Debug;

/// Collection of types used to provide an implementation of ed25519, the Edwards-curve Digital
/// Signature Algorithm scheme using sha-512 (sha2) and Curve25519
pub trait Ed25519Provider {
    /// The keypair which includes both public and secret halves of an asymmetric key.
    type KeyPair: KeyPair<PublicKey = Self::PublicKey, Signature = Self::Signature>;
    /// The ed25519 public key, used when verifying a message
    type PublicKey: PublicKey<Signature = Self::Signature>;
    /// The ed25519 signature which is the result of signing a message
    type Signature: Signature;
}

/// The length of a ed25519 `Signature`, in bytes.
pub const SIGNATURE_LENGTH: usize = 64;

/// The length of an ed25519 `KeyPair`, in bytes.
pub const KEY_PAIR_LENGTH: usize = 64;

/// The length of an ed25519 `PublicKey`, in bytes.
pub const KEY_LENGTH: usize = 32;

/// The keypair which includes both public and secret halves of an asymmetric key.
pub trait KeyPair: Sized {
    /// The ed25519 public key, used when verifying a message
    type PublicKey: PublicKey;

    /// The ed25519 signature returned when signing a message
    type Signature: Signature;

    /// Converts the key-pair to an array of bytes consisting
    /// of the bytes of the private key followed by the bytes
    /// of the public key. This method should only ever be called
    /// by code which securely stores private credentials.
    fn to_bytes(&self) -> [u8; KEY_PAIR_LENGTH];

    /// Builds this key-pair from an array of bytes in the
    /// format yielded by `to_bytes`. This method should
    /// only ever be called by code which securely stores private
    /// credentials.
    fn from_bytes(bytes: [u8; KEY_PAIR_LENGTH]) -> Result<Self, InvalidBytes>
    where
        Self: Sized;

    /// Sign the given message and return a digital signature
    fn sign(&self, msg: &[u8]) -> Self::Signature;

    /// Generate an ed25519 keypair from a CSPRNG
    /// generate is not available in `no-std`
    #[cfg(feature = "std")]
    fn generate() -> Self;

    /// getter function for the Public Key of the key pair
    fn public(&self) -> Self::PublicKey;
}

/// An ed25519 signature
pub trait Signature: Sized {
    /// Create a new signature from a byte slice, and return an error on an invalid signature
    /// An `Ok` result does not guarantee that the Signature is valid, however it will catch a
    /// number of invalid signatures relatively inexpensively.
    fn from_bytes(bytes: &[u8]) -> Result<Self, InvalidSignature>;

    /// Returns a slice of the signature bytes
    fn to_bytes(&self) -> [u8; SIGNATURE_LENGTH];
}

/// An ed25519 public key
pub trait PublicKey {
    /// the signature type being used by verify
    type Signature: Signature;

    /// Builds this public key from an array of bytes in
    /// the format yielded by `to_bytes`.
    fn from_bytes(bytes: [u8; KEY_LENGTH]) -> Result<Self, InvalidBytes>
    where
        Self: Sized;

    /// Yields the bytes of the public key
    fn to_bytes(&self) -> [u8; KEY_LENGTH];

    /// Succeeds if the signature was a valid signature created by this Keypair on the prehashed_message.
    fn verify_strict(
        &self,
        message: &[u8],
        signature: &Self::Signature,
    ) -> Result<(), SignatureError>;
}

/// error returned when bad bytes are provided to generate keypair
#[derive(Debug)]
pub struct InvalidBytes;

/// Error returned if the verification on the signature + message fails
#[derive(Debug)]
pub struct SignatureError;

/// Error returned if invalid signature bytes are provided
#[derive(Debug)]
pub struct InvalidSignature;
