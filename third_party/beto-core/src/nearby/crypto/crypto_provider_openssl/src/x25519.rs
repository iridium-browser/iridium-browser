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

use crypto_provider::elliptic_curve::{EcdhProvider, EphemeralSecret, PublicKey};
use crypto_provider::x25519::X25519;
use openssl::derive::Deriver;
use openssl::error::ErrorStack;
use openssl::pkey::{Id, PKey, Private, Public};

/// Public key type for X25519 using OpenSSL's implementation.
#[derive(Debug)]
pub struct X25519PublicKey(PKey<Public>);

impl PartialEq for X25519PublicKey {
    fn eq(&self, other: &Self) -> bool {
        self.0.public_eq(&other.0)
    }
}

impl PublicKey<X25519> for X25519PublicKey {
    type Error = ErrorStack;

    fn from_bytes(bytes: &[u8]) -> Result<Self, Self::Error> {
        let key = PKey::public_key_from_raw_bytes(bytes, Id::X25519)?;
        Ok(X25519PublicKey(key))
    }

    fn to_bytes(&self) -> Vec<u8> {
        self.0.raw_public_key().unwrap()
    }
}

/// Private key type for X25519 using OpenSSL's implementation.
pub struct X25519PrivateKey(PKey<Private>);

impl EphemeralSecret<X25519> for X25519PrivateKey {
    type Impl = X25519Ecdh;
    type Error = ErrorStack;
    type Rng = ();

    fn generate_random(_rng: &mut Self::Rng) -> Self {
        let private_key = openssl::pkey::PKey::generate_x25519().unwrap();
        Self(private_key)
    }

    fn public_key_bytes(&self) -> Vec<u8> {
        self.0.raw_public_key().unwrap()
    }

    fn diffie_hellman(
        self,
        other_pub: &X25519PublicKey,
    ) -> Result<<Self::Impl as EcdhProvider<X25519>>::SharedSecret, Self::Error> {
        let mut deriver = Deriver::new(&self.0)?;
        deriver.set_peer(&other_pub.0)?;
        let mut buf = [0_u8; 32];
        let num_bytes_written = deriver.derive(&mut buf)?;
        debug_assert_eq!(num_bytes_written, 32);
        Ok(buf)
    }
}

#[cfg(test)]
impl crypto_provider_test::elliptic_curve::EphemeralSecretForTesting<X25519> for X25519PrivateKey {
    fn from_private_components(
        private_bytes: &[u8; 32],
        _public_key: &<Self::Impl as EcdhProvider<X25519>>::PublicKey,
    ) -> Result<Self, Self::Error> {
        let pkey = PKey::private_key_from_raw_bytes(private_bytes, Id::X25519)?;
        Ok(Self(pkey))
    }
}

/// The OpenSSL implementation of X25519 ECDH.
pub enum X25519Ecdh {}
impl EcdhProvider<X25519> for X25519Ecdh {
    type PublicKey = X25519PublicKey;
    type EphemeralSecret = X25519PrivateKey;
    type SharedSecret = [u8; 32];
}

#[cfg(test)]
mod tests {
    use core::marker::PhantomData;
    use crypto_provider_test::x25519::*;

    use super::X25519Ecdh;

    #[apply(x25519_test_cases)]
    fn x25519_tests(testcase: CryptoProviderTestCase<X25519Ecdh>) {
        testcase(PhantomData::<X25519Ecdh>)
    }
}
