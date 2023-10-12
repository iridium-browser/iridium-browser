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

//! Credential types used in deserialization.
//!
//! While simple implementations are provided to get started with, there is likely opportunity for
//! efficiency gains with implementations tailored to suit (e.g. caching a few hot credentials
//! rather than reading from disk every time, etc).

use core::fmt::Debug;
use crypto_provider::CryptoProvider;

use self::{
    simple::{SimpleV0Credential, SimpleV1Credential},
    source::OwnedBothCredentialSource,
    v0::MinimumFootprintV0CryptoMaterial,
    v1::MinimumFootprintV1CryptoMaterial,
};

pub mod simple;
pub mod source;
pub mod v0;
pub mod v1;

/// A credential which has an associated [`MatchedCredential`]
pub trait MatchableCredential {
    /// The [MatchedCredential] provided by this [`MatchableCredential`].
    type Matched<'m>: MatchedCredential<'m>
    where
        Self: 'm;

    /// Returns the subset of credential data that should be associated with a successfully
    /// decrypted advertisement or advertisement section.
    fn matched(&self) -> Self::Matched<'_>;
}

/// Convenient type-level function for referring to the match data for a [`MatchableCredential`].
pub type MatchedCredFromCred<'s, C> = <C as MatchableCredential>::Matched<'s>;

/// The portion of a credential's data to be bundled with the advertisement content it was used to
/// decrypt.
///
/// As it is `Debug` and `Eq`, implementors should not hold any cryptographic material to avoid
/// accidental logging, timing side channels on comparison, etc, or should use custom impls of
/// those traits rather than deriving them.
pub trait MatchedCredential<'m>: Debug + PartialEq + Eq {}

/// A V0 credential containing some [`v0::V0CryptoMaterial`]
pub trait V0Credential: MatchableCredential {
    /// The [v0::V0CryptoMaterial] provided by this V0Credential impl.
    type CryptoMaterial: v0::V0CryptoMaterial;

    /// Returns the crypto material associated with the credential.
    ///
    /// Used to decrypted encrypted advertisement content.
    fn crypto_material(&self) -> &Self::CryptoMaterial;
}

/// A V1 credential containing some [`v1::V1CryptoMaterial`]
pub trait V1Credential: MatchableCredential {
    /// The [v1::V1CryptoMaterial] provided by this Credential impl.
    type CryptoMaterial: v1::V1CryptoMaterial;

    /// Returns the crypto material associated with the credential.
    ///
    /// Used to decrypt encrypted advertisement content.
    fn crypto_material(&self) -> &Self::CryptoMaterial;
}

/// An owned credential store that contains minimum footprint crypto materials
pub type MinimumFootprintCredentialSource<T> = OwnedBothCredentialSource<
    SimpleV0Credential<MinimumFootprintV0CryptoMaterial, T>,
    SimpleV1Credential<MinimumFootprintV1CryptoMaterial, T>,
>;
