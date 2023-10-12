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

use crate::RcRng;
use alloc::vec::Vec;
use core::marker::PhantomData;
use crypto_provider::elliptic_curve::{EcdhProvider, EphemeralSecret, PublicKey};
use crypto_provider::x25519::X25519;
use rand::RngCore;
use rand_core::{CryptoRng, SeedableRng};

/// The RustCrypto implementation of X25519 ECDH.
pub struct X25519Ecdh<R> {
    _marker: PhantomData<R>,
}

impl<R: CryptoRng + RngCore + SeedableRng + Send> EcdhProvider<X25519> for X25519Ecdh<R> {
    type PublicKey = X25519PublicKey;
    type EphemeralSecret = X25519EphemeralSecret<R>;
    type SharedSecret = [u8; 32];
}

/// A X25519 ephemeral secret used for Diffie-Hellman.
pub struct X25519EphemeralSecret<R: CryptoRng + RngCore + SeedableRng> {
    secret: x25519_dalek::EphemeralSecret,
    marker: PhantomData<R>,
}

impl<R: CryptoRng + RngCore + SeedableRng + Send> EphemeralSecret<X25519>
    for X25519EphemeralSecret<R>
{
    type Impl = X25519Ecdh<R>;
    type Error = Error;
    type Rng = RcRng<R>;

    fn generate_random(rng: &mut Self::Rng) -> Self {
        Self {
            secret: x25519_dalek::EphemeralSecret::random_from_rng(&mut rng.0),
            marker: Default::default(),
        }
    }

    fn public_key_bytes(&self) -> Vec<u8> {
        let pubkey: x25519_dalek::PublicKey = (&self.secret).into();
        pubkey.to_bytes().into()
    }

    fn diffie_hellman(
        self,
        other_pub: &<X25519Ecdh<R> as EcdhProvider<X25519>>::PublicKey,
    ) -> Result<<X25519Ecdh<R> as EcdhProvider<X25519>>::SharedSecret, Self::Error> {
        Ok(x25519_dalek::EphemeralSecret::diffie_hellman(self.secret, &other_pub.0).to_bytes())
    }
}

#[cfg(test)]
impl<R: CryptoRng + RngCore + SeedableRng + Send>
    crypto_provider_test::elliptic_curve::EphemeralSecretForTesting<X25519>
    for X25519EphemeralSecret<R>
{
    fn from_private_components(
        private_bytes: &[u8; 32],
        _public_key: &X25519PublicKey,
    ) -> Result<Self, Self::Error> {
        Ok(Self {
            secret: x25519_dalek::EphemeralSecret::random_from_rng(crate::testing::MockCryptoRng {
                values: private_bytes.iter(),
            }),
            marker: Default::default(),
        })
    }
}

/// A X25519 public key.
#[derive(Debug, PartialEq, Eq)]
pub struct X25519PublicKey(x25519_dalek::PublicKey);

impl PublicKey<X25519> for X25519PublicKey {
    type Error = Error;

    fn from_bytes(bytes: &[u8]) -> Result<Self, Self::Error> {
        let byte_sized: [u8; 32] = bytes.try_into().map_err(|_| Error::WrongSize)?;
        Ok(Self(byte_sized.into()))
    }

    fn to_bytes(&self) -> Vec<u8> {
        self.0.as_bytes().to_vec()
    }
}

/// Error type for the RustCrypto implementation of x25519.
#[derive(Debug)]
pub enum Error {
    /// Unexpected size for the given input.
    WrongSize,
}

#[cfg(test)]
mod tests {
    use super::X25519Ecdh;
    use core::marker::PhantomData;
    use crypto_provider_test::x25519::*;
    use rand::rngs::StdRng;

    #[apply(x25519_test_cases)]
    fn x25519_tests(testcase: CryptoProviderTestCase<X25519Ecdh<StdRng>>) {
        testcase(PhantomData::<X25519Ecdh<StdRng>>)
    }
}
