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

extern crate alloc;

use core::fmt::Debug;

use alloc::vec::Vec;

/// Marker trait for an elliptic curve used for diffie-hellman.
pub trait Curve {}

/// Collection of types used to provide a particular elliptic curve implementation.
pub trait EcdhProvider<C: Curve> {
    /// The public key type.
    type PublicKey: PublicKey<C>;
    /// The ephemeral secret type used to perform diffie-hellman and derive a shared secret.
    type EphemeralSecret: EphemeralSecret<C, Impl = Self>;
    /// The shared secret type.
    type SharedSecret: Into<[u8; 32]>;
}

/// Trait for an ephemeral secret used to perform diffie-hellamn and derive a shared secret.
pub trait EphemeralSecret<C: Curve>: Send {
    /// The associated ECDH provider.
    type Impl: EcdhProvider<C>;

    /// The error type associated with this ephemeral secret implementation.
    type Error: Debug;

    /// The random number generator to be used for generating a secret
    type Rng: crate::CryptoRng;

    /// Generates a new random ephemeral secret.
    fn generate_random(rng: &mut Self::Rng) -> Self;

    /// Returns the bytes of the public key for this ephemeral secret that is suitable for sending
    /// over the wire for key exchange.
    fn public_key_bytes(&self) -> Vec<u8>;

    /// Performs diffie-hellman key exchange using this ephemeral secret with the given public key
    /// `other_pub`.
    fn diffie_hellman(
        self,
        other_pub: &<Self::Impl as EcdhProvider<C>>::PublicKey,
    ) -> Result<<Self::Impl as EcdhProvider<C>>::SharedSecret, Self::Error>;
}

/// Trait for a public key used for elliptic curve diffie hellman.
pub trait PublicKey<E: Curve>: Sized + PartialEq + Debug {
    /// The error type associated with Public Key.
    type Error: Debug;

    /// Creates the public key from the given bytes. The format of the bytes is dependent on the
    /// curve used, but it must be the inverse of `to_bytes`.
    fn from_bytes(bytes: &[u8]) -> Result<Self, Self::Error>;

    /// Serializes the given public key to bytes. This format of the bytes is dependent on the
    /// curve, but it must be the inverse of `from_bytes`. Note that some formats, like P256 using
    /// the sec1 encoding, may return equivalent but different byte-representations due to point
    /// compression, so it is not necessarily true that `from_bytes(bytes)?.to_bytes() == bytes`
    /// (but it is always true that `from_bytes(key.to_bytes())? == key`).
    fn to_bytes(&self) -> Vec<u8>;
}
