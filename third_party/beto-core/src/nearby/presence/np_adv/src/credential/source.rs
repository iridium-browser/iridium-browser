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

//! Traits defining sources for credentials. These are used in the deserialization path to provide
//! the credentials to try. [`SliceCredentialSource`] and [`OwnedCredentialSource`] implementations
//! are also defined in this module.

use super::*;
use alloc::vec::Vec;

/// A source of credentials to try when decrypting advertisements,
/// which really just wraps an iterator over a given credential type.
pub trait CredentialSource<C: MatchableCredential> {
    /// The iterator type produced that emits credentials
    type Iterator<'a>: Iterator<Item = &'a C>
    where
        Self: 'a,
        C: 'a;

    /// Iterate over the available credentials
    fn iter(&self) -> Self::Iterator<'_>;
}

/// Trait for combined credential sources able to yield credential sources for both V0 and V1.
pub trait BothCredentialSource<C0, C1>
where
    C0: V0Credential,
    C1: V1Credential,
{
    /// The type of the underlying credential-source for v0 credentials
    type V0Source: CredentialSource<C0>;
    /// The type of the underlying credential-source for v1 credentials
    type V1Source: CredentialSource<C1>;

    /// Gets a source for v0 credentials maintained by this `BothCredentialSource`.
    fn v0(&self) -> &Self::V0Source;

    /// Gets a source for v1 credentials maintained by this `BothCredentialSource`.
    fn v1(&self) -> &Self::V1Source;

    /// Convenient function alias to [`self.v0().iter()`] for iterating
    /// over v0 credentials.
    fn iter_v0(&self) -> <Self::V0Source as CredentialSource<C0>>::Iterator<'_> {
        self.v0().iter()
    }

    /// Convenient function alias to the [`CredentialSource<C1>#iter()`] for iterating
    /// over v0 credentials.
    fn iter_v1(&self) -> <Self::V1Source as CredentialSource<C1>>::Iterator<'_> {
        self.v1().iter()
    }
}

/// A simple [CredentialSource] that just iterates over a provided slice of credentials
pub struct SliceCredentialSource<'c, C: MatchableCredential> {
    credentials: &'c [C],
}

impl<'c, C: MatchableCredential> SliceCredentialSource<'c, C> {
    /// Construct the credential source from the provided credentials.
    pub fn new(credentials: &'c [C]) -> Self {
        Self { credentials }
    }
}

impl<'c, C: MatchableCredential> CredentialSource<C> for SliceCredentialSource<'c, C> {
    type Iterator<'i>  = core::slice::Iter<'i, C>
    where Self: 'i;

    fn iter(&'_ self) -> Self::Iterator<'_> {
        self.credentials.iter()
    }
}

/// A simple credential source which owns all of its credentials.
pub struct OwnedCredentialSource<C: MatchableCredential> {
    credentials: Vec<C>,
}

impl<C: MatchableCredential> OwnedCredentialSource<C> {
    /// Constructs an owned credential source from the given credentials
    pub fn new(credentials: Vec<C>) -> Self {
        Self { credentials }
    }
}

impl<C: MatchableCredential> CredentialSource<C> for OwnedCredentialSource<C> {
    type Iterator<'i>  = core::slice::Iter<'i, C>
    where Self: 'i;

    fn iter(&'_ self) -> Self::Iterator<'_> {
        self.credentials.iter()
    }
}

/// An owned credential source for both v0 and v1 credentials,
pub struct OwnedBothCredentialSource<C0, C1>
where
    C0: V0Credential,
    C1: V1Credential,
{
    v0_source: OwnedCredentialSource<C0>,
    v1_source: OwnedCredentialSource<C1>,
}

impl<C0, C1> OwnedBothCredentialSource<C0, C1>
where
    C0: V0Credential,
    C1: V1Credential,
{
    /// Creates a new `OwnedBothCredentialSource` from credential-lists
    /// for both V0 and V1
    pub fn new(v0_credentials: Vec<C0>, v1_credentials: Vec<C1>) -> Self {
        let v0_source = OwnedCredentialSource::new(v0_credentials);
        let v1_source = OwnedCredentialSource::new(v1_credentials);
        Self { v0_source, v1_source }
    }

    /// Creates a new credential source that is empty.
    pub fn new_empty() -> Self {
        Self::new(Vec::new(), Vec::new())
    }
}

impl<C0, C1> BothCredentialSource<C0, C1> for OwnedBothCredentialSource<C0, C1>
where
    C0: V0Credential,
    C1: V1Credential,
{
    type V0Source = OwnedCredentialSource<C0>;
    type V1Source = OwnedCredentialSource<C1>;

    fn v0(&self) -> &Self::V0Source {
        &self.v0_source
    }
    fn v1(&self) -> &Self::V1Source {
        &self.v1_source
    }
}
