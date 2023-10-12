// Copyright 2023 Google LLC
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

use crypto_provider::ed25519::{
    InvalidBytes, RawPrivateKey, RawPublicKey, RawSignature, Signature as _, SignatureError,
};

pub struct Ed25519;

impl crypto_provider::ed25519::Ed25519Provider for Ed25519 {
    type KeyPair = KeyPair;
    type PublicKey = PublicKey;
    type Signature = Signature;
}

pub struct KeyPair(bssl_crypto::ed25519::PrivateKey);

impl crypto_provider::ed25519::KeyPair for KeyPair {
    type PublicKey = PublicKey;
    type Signature = Signature;

    fn private_key(&self) -> RawPrivateKey {
        self.0.to_seed()
    }

    fn from_private_key(bytes: &RawPrivateKey) -> Self
    where
        Self: Sized,
    {
        let private_key = bssl_crypto::ed25519::PrivateKey::new_from_seed(bytes);
        Self(private_key)
    }

    fn sign(&self, msg: &[u8]) -> Self::Signature {
        Signature(self.0.sign(msg))
    }

    fn generate() -> Self {
        Self(bssl_crypto::ed25519::PrivateKey::generate())
    }

    fn public(&self) -> Self::PublicKey {
        PublicKey(self.0.public())
    }
}

pub struct Signature(bssl_crypto::ed25519::Signature);

impl crypto_provider::ed25519::Signature for Signature {
    fn from_bytes(bytes: &RawSignature) -> Self {
        Self(bssl_crypto::ed25519::Signature::from_bytes(*bytes))
    }

    fn to_bytes(&self) -> RawSignature {
        self.0.to_bytes()
    }
}

pub struct PublicKey(bssl_crypto::ed25519::PublicKey);

impl crypto_provider::ed25519::PublicKey for PublicKey {
    type Signature = Signature;

    fn from_bytes(bytes: &RawPublicKey) -> Result<Self, InvalidBytes>
    where
        Self: Sized,
    {
        Ok(Self(bssl_crypto::ed25519::PublicKey::from_bytes(*bytes)))
    }

    fn to_bytes(&self) -> RawPublicKey {
        self.0.to_bytes()
    }

    fn verify_strict(
        &self,
        message: &[u8],
        signature: &Self::Signature,
    ) -> Result<(), SignatureError> {
        self.0
            .verify(message, bssl_crypto::ed25519::Signature::from_bytes(signature.to_bytes()))
            .map_err(|_| SignatureError)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crypto_provider_test::ed25519::{run_rfc_test_vectors, run_wycheproof_test_vectors};

    #[test]
    fn wycheproof_test_ed25519_boringssl() {
        run_wycheproof_test_vectors::<Ed25519>()
    }

    #[test]
    fn rfc_test_ed25519_boringssl() {
        run_rfc_test_vectors::<Ed25519>()
    }
}
