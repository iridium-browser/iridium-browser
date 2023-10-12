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

//! Placeholder crate for an unimplemented CP. Can be used to satisfy the trait bounds of
//! the uber CryptoProvider trait, when only a subset of the associated types have real implementations
//! Can be removed once no one else is depending on it.

#![allow(unused_variables)]

use std::fmt::Debug;

use crypto_provider::ed25519::{RawPrivateKey, RawPublicKey, RawSignature};
use crypto_provider::{
    aead::aes_gcm_siv::AesGcmSiv,
    aead::{Aead, AeadError},
    aes::{
        cbc::{AesCbcIv, AesCbcPkcs7Padded, DecryptionError},
        ctr::{AesCtr, NonceAndCounter},
        Aes, Aes128Key, Aes256Key, AesBlock, AesCipher, AesDecryptCipher, AesEncryptCipher,
    },
    ed25519,
    ed25519::{Ed25519Provider, InvalidBytes, KeyPair, Signature, SignatureError},
    elliptic_curve::{EcdhProvider, EphemeralSecret, PublicKey},
    hkdf::{Hkdf, InvalidLength},
    hmac::{Hmac, MacError},
    p256::{P256PublicKey, P256},
    x25519::X25519,
};

#[derive(Default, Clone, Debug, PartialEq, Eq)]
pub struct CryptoProviderStubs;

impl crypto_provider::CryptoProvider for CryptoProviderStubs {
    type HkdfSha256 = HkdfStubs;
    type HmacSha256 = HmacStubs;
    type HkdfSha512 = HkdfStubs;
    type HmacSha512 = HmacStubs;
    type AesCbcPkcs7Padded = AesCbcPkcs7PaddedStubs;
    type X25519 = X25519Stubs;
    type P256 = P256Stubs;
    type Sha256 = Sha2Stubs;
    type Sha512 = Sha2Stubs;
    type Aes128 = Aes128Impl;
    type Aes256 = Aes256Impl;
    type AesCtr128 = Aes128Stubs;
    type AesCtr256 = Aes256Stubs;
    type Ed25519 = Ed25519Stubs;
    type Aes128GcmSiv = Aes128Stubs;
    type Aes256GcmSiv = Aes256Stubs;
    type CryptoRng = ();

    fn constant_time_eq(_a: &[u8], _b: &[u8]) -> bool {
        unimplemented!()
    }
}

pub struct Aes128Impl;

impl Aes for Aes128Impl {
    type Key = Aes128Key;
    type EncryptCipher = Aes128Stubs;
    type DecryptCipher = Aes128Stubs;
}

pub struct Aes256Impl;

impl Aes for Aes256Impl {
    type Key = Aes256Key;
    type EncryptCipher = Aes256Stubs;
    type DecryptCipher = Aes256Stubs;
}

#[derive(Clone)]
pub struct HkdfStubs;

impl Hkdf for HkdfStubs {
    fn new(_salt: Option<&[u8]>, _ikm: &[u8]) -> Self {
        unimplemented!()
    }

    fn expand_multi_info(
        &self,
        _info_components: &[&[u8]],
        _okm: &mut [u8],
    ) -> Result<(), InvalidLength> {
        unimplemented!()
    }

    fn expand(&self, _info: &[u8], _okm: &mut [u8]) -> Result<(), InvalidLength> {
        unimplemented!()
    }
}

pub struct HmacStubs;

impl Hmac<32> for HmacStubs {
    fn new_from_key(_key: [u8; 32]) -> Self {
        unimplemented!()
    }

    fn new_from_slice(_key: &[u8]) -> Result<Self, crypto_provider::hmac::InvalidLength> {
        unimplemented!()
    }

    fn update(&mut self, _data: &[u8]) {
        unimplemented!()
    }

    fn finalize(self) -> [u8; 32] {
        unimplemented!()
    }

    fn verify_slice(self, _tag: &[u8]) -> Result<(), MacError> {
        unimplemented!()
    }

    fn verify(self, _tag: [u8; 32]) -> Result<(), MacError> {
        unimplemented!()
    }

    fn verify_truncated_left(self, _tag: &[u8]) -> Result<(), MacError> {
        unimplemented!()
    }
}

impl Hmac<64> for HmacStubs {
    fn new_from_key(_key: [u8; 64]) -> Self {
        unimplemented!()
    }

    fn new_from_slice(_key: &[u8]) -> Result<Self, crypto_provider::hmac::InvalidLength> {
        unimplemented!()
    }

    fn update(&mut self, _data: &[u8]) {
        unimplemented!()
    }

    fn finalize(self) -> [u8; 64] {
        unimplemented!()
    }

    fn verify_slice(self, _tag: &[u8]) -> Result<(), MacError> {
        unimplemented!()
    }

    fn verify(self, _tag: [u8; 64]) -> Result<(), MacError> {
        unimplemented!()
    }

    fn verify_truncated_left(self, _tag: &[u8]) -> Result<(), MacError> {
        unimplemented!()
    }
}

pub struct AesCbcPkcs7PaddedStubs;

impl AesCbcPkcs7Padded for AesCbcPkcs7PaddedStubs {
    fn encrypt(_key: &Aes256Key, _iv: &AesCbcIv, _message: &[u8]) -> Vec<u8> {
        unimplemented!()
    }

    fn decrypt(
        _key: &Aes256Key,
        _iv: &AesCbcIv,
        _ciphertext: &[u8],
    ) -> Result<Vec<u8>, DecryptionError> {
        unimplemented!()
    }
}

pub struct X25519Stubs;

impl EcdhProvider<X25519> for X25519Stubs {
    type PublicKey = EcdhPubKey;
    type EphemeralSecret = EphSecretStubs;
    type SharedSecret = [u8; 32];
}

pub struct EphSecretStubs;

impl EphemeralSecret<X25519> for EphSecretStubs {
    type Impl = X25519Stubs;
    type Error = ();
    type Rng = ();

    fn generate_random(_rng: &mut Self::Rng) -> Self {
        unimplemented!()
    }

    fn public_key_bytes(&self) -> Vec<u8> {
        unimplemented!()
    }

    fn diffie_hellman(
        self,
        _other_pub: &<Self::Impl as EcdhProvider<X25519>>::PublicKey,
    ) -> Result<<Self::Impl as EcdhProvider<X25519>>::SharedSecret, Self::Error> {
        unimplemented!()
    }
}

impl EphemeralSecret<P256> for EphSecretStubs {
    type Impl = P256Stubs;
    type Error = ();
    type Rng = ();

    fn generate_random(_rng: &mut Self::Rng) -> Self {
        unimplemented!()
    }

    fn public_key_bytes(&self) -> Vec<u8> {
        unimplemented!()
    }

    fn diffie_hellman(
        self,
        _other_pub: &<Self::Impl as EcdhProvider<P256>>::PublicKey,
    ) -> Result<<Self::Impl as EcdhProvider<P256>>::SharedSecret, Self::Error> {
        unimplemented!()
    }
}

#[derive(Debug, PartialEq)]
pub struct EcdhPubKey;

impl PublicKey<X25519> for EcdhPubKey {
    type Error = ();

    fn from_bytes(_bytes: &[u8]) -> Result<Self, Self::Error> {
        unimplemented!()
    }

    fn to_bytes(&self) -> Vec<u8> {
        unimplemented!()
    }
}

#[derive(Debug, PartialEq, Eq)]
pub struct PublicKeyStubs;

impl P256PublicKey for PublicKeyStubs {
    type Error = ();

    fn from_sec1_bytes(_bytes: &[u8]) -> Result<Self, Self::Error> {
        unimplemented!()
    }

    fn to_sec1_bytes(&self) -> Vec<u8> {
        unimplemented!()
    }

    fn to_affine_coordinates(&self) -> Result<([u8; 32], [u8; 32]), Self::Error> {
        unimplemented!()
    }

    fn from_affine_coordinates(_x: &[u8; 32], _y: &[u8; 32]) -> Result<Self, Self::Error> {
        unimplemented!()
    }
}

pub struct P256Stubs;

impl EcdhProvider<P256> for P256Stubs {
    type PublicKey = PublicKeyStubs;
    type EphemeralSecret = EphSecretStubs;
    type SharedSecret = [u8; 32];
}

pub struct Sha2Stubs;

impl crypto_provider::sha2::Sha256 for Sha2Stubs {
    fn sha256(_input: &[u8]) -> [u8; 32] {
        unimplemented!()
    }
}

impl crypto_provider::sha2::Sha512 for Sha2Stubs {
    fn sha512(_input: &[u8]) -> [u8; 64] {
        unimplemented!()
    }
}

pub struct Aes128Stubs;

impl AesCipher for Aes128Stubs {
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        unimplemented!()
    }
}

impl AesDecryptCipher for Aes128Stubs {
    fn decrypt(&self, block: &mut AesBlock) {
        unimplemented!()
    }
}

impl AesEncryptCipher for Aes128Stubs {
    fn encrypt(&self, block: &mut AesBlock) {
        unimplemented!()
    }
}

impl AesCtr for Aes128Stubs {
    type Key = Aes128Key;

    fn new(_key: &Self::Key, _nonce_and_counter: NonceAndCounter) -> Self {
        unimplemented!()
    }

    fn encrypt(&mut self, _data: &mut [u8]) {
        unimplemented!()
    }

    fn decrypt(&mut self, _data: &mut [u8]) {
        unimplemented!()
    }
}

impl Aead for Aes128Stubs {
    const TAG_SIZE: usize = 16;
    type Nonce = [u8; 12];
    type Key = Aes128Key;

    fn new(key: &Self::Key) -> Self {
        unimplemented!()
    }

    fn encrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        unimplemented!()
    }

    fn decrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        unimplemented!()
    }
}

impl AesGcmSiv for Aes128Stubs {}

pub struct Aes256Stubs;

impl AesCipher for Aes256Stubs {
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        unimplemented!()
    }
}

impl AesEncryptCipher for Aes256Stubs {
    fn encrypt(&self, block: &mut AesBlock) {
        unimplemented!()
    }
}

impl AesDecryptCipher for Aes256Stubs {
    fn decrypt(&self, block: &mut AesBlock) {
        unimplemented!()
    }
}

impl AesCtr for Aes256Stubs {
    type Key = Aes256Key;

    fn new(_key: &Self::Key, _nonce_and_counter: NonceAndCounter) -> Self {
        unimplemented!()
    }

    fn encrypt(&mut self, _data: &mut [u8]) {
        unimplemented!()
    }

    fn decrypt(&mut self, _data: &mut [u8]) {
        unimplemented!()
    }
}

impl Aead for Aes256Stubs {
    const TAG_SIZE: usize = 16;
    type Nonce = [u8; 12];
    type Key = Aes256Key;

    fn new(key: &Self::Key) -> Self {
        unimplemented!()
    }

    fn encrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        unimplemented!()
    }

    fn decrypt(&self, msg: &mut Vec<u8>, aad: &[u8], nonce: &[u8; 12]) -> Result<(), AeadError> {
        unimplemented!()
    }
}

impl AesGcmSiv for Aes256Stubs {}

pub struct Ed25519Stubs;

impl Ed25519Provider for Ed25519Stubs {
    type KeyPair = KeyPairStubs;
    type PublicKey = PublicKeyStubs;
    type Signature = SignatureStubs;
}

impl ed25519::PublicKey for PublicKeyStubs {
    type Signature = SignatureStubs;

    fn from_bytes(bytes: &RawPublicKey) -> Result<Self, InvalidBytes>
    where
        Self: Sized,
    {
        unimplemented!()
    }

    fn to_bytes(&self) -> RawPublicKey {
        unimplemented!()
    }

    fn verify_strict(
        &self,
        _message: &[u8],
        _signature: &Self::Signature,
    ) -> Result<(), SignatureError> {
        unimplemented!()
    }
}

pub struct SignatureStubs;

impl Signature for SignatureStubs {
    fn from_bytes(_bytes: &RawSignature) -> Self {
        unimplemented!()
    }

    fn to_bytes(&self) -> RawSignature {
        unimplemented!()
    }
}

pub struct KeyPairStubs;

impl KeyPair for KeyPairStubs {
    type PublicKey = PublicKeyStubs;
    type Signature = SignatureStubs;

    fn private_key(&self) -> RawPrivateKey {
        unimplemented!()
    }

    fn from_private_key(_bytes: &RawPrivateKey) -> Self
    where
        Self: Sized,
    {
        unimplemented!()
    }

    fn sign(&self, _msg: &[u8]) -> Self::Signature {
        unimplemented!()
    }

    fn generate() -> Self {
        unimplemented!()
    }

    fn public(&self) -> Self::PublicKey {
        unimplemented!()
    }
}
