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

use crypto_provider::elliptic_curve::*;

/// Trait for an ephemeral secret for functions used in testing.
pub trait EphemeralSecretForTesting<C: Curve>: EphemeralSecret<C> {
    /// Creates an ephemeral secret from the given private and public components.
    fn from_private_components(
        _private_bytes: &[u8; 32],
        _public_key: &<Self::Impl as EcdhProvider<C>>::PublicKey,
    ) -> Result<Self, Self::Error>
    where
        Self: Sized;
}
