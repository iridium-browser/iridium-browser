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

//! Simple implementations of credentials. These can be combined with the provided crypto material
//! implementations to have a working credential type.
//!
//! ```rust
//! use np_adv::credential::{
//!     simple::SimpleV0Credential,
//!     v0::MinimumFootprintV0CryptoMaterial,
//! };
//! type MyV0Credential = SimpleV0Credential<MinimumFootprintV0CryptoMaterial, ()>;
//! ```

use core::fmt::Debug;

use super::*;
use super::{v0::*, v1::*};

/// A simple implementation of [`V0Credential`] that wraps a [`V0CryptoMaterial`] and some `T` data that
/// will be exposed via the [`MatchedCredential`].
pub struct SimpleV0Credential<C, T>
where
    C: V0CryptoMaterial,
    T: Debug + Eq,
{
    material: C,
    match_data: T,
}

impl<C, T> SimpleV0Credential<C, T>
where
    C: V0CryptoMaterial,
    T: Debug + Eq,
{
    /// Construct a new credential.
    ///
    /// `material` will be returned by [V0Credential::crypto_material].
    /// `match_data` will be returned by [SimpleV0Credential::matched], wrapped in [SimpleMatchedCredential].
    pub fn new(material: C, match_data: T) -> Self {
        Self { material, match_data }
    }
}

impl<C, T> MatchableCredential for SimpleV0Credential<C, T>
where
    C: V0CryptoMaterial,
    T: Debug + Eq,
{
    type Matched<'m> = SimpleMatchedCredential<'m, T> where Self: 'm;

    fn matched(&'_ self) -> Self::Matched<'_> {
        SimpleMatchedCredential { data: &self.match_data }
    }
}

impl<C, T> V0Credential for SimpleV0Credential<C, T>
where
    C: V0CryptoMaterial,
    T: Debug + Eq,
{
    type CryptoMaterial = C;

    fn crypto_material(&self) -> &Self::CryptoMaterial {
        &self.material
    }
}

/// A simple implementation of [V1Credential] that wraps a [V1CryptoMaterial] and some `T` data that
/// will be exposed via the [MatchedCredential].
pub struct SimpleV1Credential<C, T>
where
    C: V1CryptoMaterial,
    T: Debug + Eq,
{
    material: C,
    match_data: T,
}

impl<C, T> SimpleV1Credential<C, T>
where
    C: V1CryptoMaterial,
    T: Debug + Eq,
{
    /// Construct a new credential.
    ///
    /// `material` will be returned by [V1Credential::crypto_material].
    /// `match_data` will be returned by [SimpleV1Credential::matched], wrapped in [SimpleMatchedCredential].
    pub fn new(material: C, match_data: T) -> Self {
        Self { material, match_data }
    }
}

impl<C, T> MatchableCredential for SimpleV1Credential<C, T>
where
    C: V1CryptoMaterial,
    T: Debug + Eq,
{
    type Matched<'m> = SimpleMatchedCredential<'m, T> where Self: 'm;

    fn matched(&'_ self) -> Self::Matched<'_> {
        SimpleMatchedCredential { data: &self.match_data }
    }
}

impl<C, T> V1Credential for SimpleV1Credential<C, T>
where
    C: V1CryptoMaterial,
    T: Debug + Eq,
{
    type CryptoMaterial = C;

    fn crypto_material(&self) -> &Self::CryptoMaterial {
        &self.material
    }
}

/// The [MatchedCredential] used by [SimpleV0Credential]
/// and by [SimpleV1Credential].
#[derive(Debug, PartialEq, Eq)]
pub struct SimpleMatchedCredential<'m, T: Debug + PartialEq + Eq> {
    data: &'m T,
}

impl<'m, T: Debug + PartialEq + Eq> SimpleMatchedCredential<'m, T> {
    /// Construct a new instance that wraps `data`.
    pub fn new(data: &'m T) -> Self {
        Self { data }
    }

    /// Returns the underlying matched credential data.
    pub fn matched_data(&self) -> &'m T {
        self.data
    }
}

impl<'m, T: Debug + PartialEq + Eq> MatchedCredential<'m> for SimpleMatchedCredential<'m, T> {}
