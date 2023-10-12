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

//! Wrappers around NP's usage of ed25519 signatures.
//!
//! All of NP's usages of ed25519 signatures are performed
//! with "context" bytes prepended to the payload to be signed
//! or verified. These "context" bytes allow for usage of the
//! same base key-pair for different purposes in the protocol.
#![no_std]
#![forbid(unsafe_code)]
#![deny(missing_docs, clippy::indexing_slicing)]

extern crate core;

use array_view::ArrayView;
use crypto_provider::ed25519::{
    Ed25519Provider, KeyPair as _, PublicKey as _, RawPrivateKey, RawPublicKey, RawSignature,
    Signature as _, SignatureError,
};
use crypto_provider::CryptoProvider;
use sink::{Sink, SinkWriter};
use tinyvec::ArrayVec;

/// Convenient type-alias for a crypto-provider's Ed25519 key-pair
type CpKeyPair<C> = <<C as CryptoProvider>::Ed25519 as Ed25519Provider>::KeyPair;

/// Type-alias for the Ed25519 public key type
type CpPublicKey<C> = <<C as CryptoProvider>::Ed25519 as Ed25519Provider>::PublicKey;

/// Type-alias for the Ed25519 signature type
type CpSignature<C> = <<C as CryptoProvider>::Ed25519 as Ed25519Provider>::Signature;

/// Maximum length of the combined (context len byte) + (context bytes) + (signing payload)
/// byte-array which an ed25519 signature will be computed over. This is deliberately
/// chosen to be large enough to incorporate an entire v1 adv as the signing payload.
pub const MAX_SIGNATURE_BUFFER_LEN: usize = 512;

/// Representation of an Ed25519 key-pair using the given
/// [`CryptoProvider`] to back its implementation.
/// Contains both the public and secret halves of an
/// asymmetric key, and so it may be used to
/// both sign and verify message signatures.
pub struct KeyPair<C: CryptoProvider>(CpKeyPair<C>);

impl<C: CryptoProvider> KeyPair<C> {
    /// Returns the `KeyPair`'s private key bytes. This method should only ever be called by code
    /// which securely stores private credentials.
    pub fn private_key(&self) -> RawPrivateKey {
        self.0.private_key()
    }

    /// Builds this key-pair from an array of its private key bytes in the yielded by `private_key`.
    /// This method should only ever be called by code which securely stores private credentials.
    pub fn from_private_key(private_key: &RawPrivateKey) -> Self {
        Self(CpKeyPair::<C>::from_private_key(private_key))
    }

    /// Sign the given message with the given context and
    /// return a digital signature. The message is represented
    /// using a [`SinkWriter`] to allow the caller to construct
    /// the payload to sign without requiring a fully-assembled
    /// payload available as a slice.
    ///
    /// If the message writer writes too much data (greater than 256 bytes),
    /// this will return `None` instead of a valid signature,
    /// and so uses in `np_adv` will use `.expect` on the returned value
    /// to indicate that this length constraint has been considered.
    pub fn sign_with_context<W: SinkWriter<DataType = u8>>(
        &self,
        context: &SignatureContext,
        msg_writer: W,
    ) -> Option<Signature<C>> {
        let mut buffer = context.create_signature_buffer();
        buffer.try_extend_from_writer(msg_writer).map(|_| Signature(self.0.sign(buffer.as_ref())))
    }

    /// Gets the public key of this key-pair
    pub fn public(&self) -> PublicKey<C> {
        PublicKey { public_key: self.0.public() }
    }

    /// Generates an ed25519 keypair from a CSPRNG
    /// generate is not available in `no-std`
    #[cfg(feature = "std")]
    pub fn generate() -> Self {
        Self(CpKeyPair::<C>::generate())
    }
}

/// Error raised when attempting to deserialize a key-pair
/// from a byte-array, but the bytes do not represent a valid
/// ed25519 key-pair
#[derive(Debug)]
pub struct InvalidKeyPairBytes;

/// Errors yielded when attempting to verify an ed25519 signature.
#[derive(Debug, PartialEq, Eq)]
pub enum SignatureVerificationError {
    /// The payload that we attempted to verify the signature of was too big
    PayloadTooBig,
    /// The signature we were checking was invalid for the given payload
    SignatureInvalid,
}

impl From<SignatureError> for SignatureVerificationError {
    fn from(_: SignatureError) -> Self {
        Self::SignatureInvalid
    }
}

/// Representation of an Ed25519 public key used for
/// signature verification.
pub struct PublicKey<C: CryptoProvider> {
    public_key: CpPublicKey<C>,
}

impl<C: CryptoProvider> PublicKey<C> {
    /// Succeeds if the signature was a valid signature created via the corresponding
    /// keypair to this public key using the given [`SignatureContext`] on the given
    /// message payload. The message payload is represented
    /// using a [`SinkWriter`] to allow the caller to construct
    /// the payload to sign without requiring a fully-assembled
    /// payload available as a slice.
    ///
    /// If the message writer writes too much data (greater than 256 bytes),
    /// this will return `None` instead of a valid signature,
    /// and so uses in `np_adv` will use `.expect` on the returned value
    /// to indicate that this length constraint has been considered.
    pub fn verify_signature_with_context<W: SinkWriter<DataType = u8>>(
        &self,
        context: &SignatureContext,
        msg_writer: W,
        signature: &Signature<C>,
    ) -> Result<(), SignatureVerificationError> {
        let mut buffer = context.create_signature_buffer();
        let maybe_write_success = buffer.try_extend_from_writer(msg_writer);
        match maybe_write_success {
            Some(_) => {
                self.public_key.verify_strict(buffer.as_ref(), &signature.0)?;
                Ok(())
            }
            None => Err(SignatureVerificationError::PayloadTooBig),
        }
    }

    /// Builds an ed25519 public key from an array of bytes in
    /// the format yielded by `to_bytes`.
    pub fn from_bytes(bytes: &RawPublicKey) -> Result<Self, InvalidPublicKeyBytes> {
        CpPublicKey::<C>::from_bytes(bytes)
            .map(|public_key| Self { public_key })
            .map_err(|_| InvalidPublicKeyBytes)
    }

    /// Yields the bytes of this ed25519 public key
    pub fn to_bytes(&self) -> RawPublicKey {
        self.public_key.to_bytes()
    }
}

impl<C: CryptoProvider> Clone for PublicKey<C> {
    fn clone(&self) -> Self {
        Self::from_bytes(&self.to_bytes()).unwrap()
    }
}

/// Error raised when attempting to deserialize a public key
/// from a byte-array, but the bytes do not represent a valid
/// ed25519 public key
#[derive(Debug)]
pub struct InvalidPublicKeyBytes;

/// Representation of an Ed25519 message signature,
/// which can be checked against a [`PublicKey`]
/// for validity.
pub struct Signature<C: CryptoProvider>(CpSignature<C>);

impl<C: CryptoProvider> Signature<C> {
    /// Returns a slice of the signature bytes
    pub fn to_bytes(&self) -> RawSignature {
        self.0.to_bytes()
    }
}

/// Error raised when attempting to construct a [`Signature`]
/// from a byte-array which is not of the proper length or format.
#[derive(Debug)]
pub struct InvalidSignatureBytes;

impl<C: CryptoProvider> TryFrom<&[u8]> for Signature<C> {
    type Error = InvalidSignatureBytes;
    fn try_from(bytes: &[u8]) -> Result<Self, Self::Error> {
        bytes
            .try_into()
            .map(|sig| Self(CpSignature::<C>::from_bytes(sig)))
            .map_err(|_| InvalidSignatureBytes)
    }
}

/// Minimum length (in bytes) for a [`SignatureContext`] (which cannot be empty).
pub const MIN_SIGNATURE_CONTEXT_LEN: usize = 1;

/// Maximum length (in bytes) for a [`SignatureContext`] (which uses a 8-bit length field).
pub const MAX_SIGNATURE_CONTEXT_LEN: usize = 255;

/// (Non-empty) context bytes to use in the construction of NP's
/// Ed25519 signatures. The context bytes should uniquely
/// identify the component of the protocol performing the
/// signature/verification (e.g: advertisement signing,
/// connection signing), and should be between 1 and
/// 255 bytes in length.
pub struct SignatureContext {
    data: ArrayView<u8, MAX_SIGNATURE_CONTEXT_LEN>,
}

impl SignatureContext {
    /// Creates a signature buffer with size bounded by MAX_SIGNATURE_BUFFER_LEN
    /// which is pre-populated with the contents yielded by
    /// [`SignatureContext#write_length_prefixed`].
    fn create_signature_buffer(&self) -> impl Sink<u8> + AsRef<[u8]> {
        let mut buffer = ArrayVec::<[u8; MAX_SIGNATURE_BUFFER_LEN]>::new();
        self.write_length_prefixed(&mut buffer).expect("Context should always fit into sig buffer");
        buffer
    }

    /// Writes the contents of this signature context, prefixed
    /// by the length of the context payload to the given byte-sink.
    /// If writing to the sink failed at some point during this operation,
    /// `None` will be returned, and the data written to the sink should
    /// be considered to be invalid.
    fn write_length_prefixed<S: Sink<u8>>(&self, sink: &mut S) -> Option<()> {
        let length_byte = self.data.len() as u8;
        sink.try_push(length_byte)?;
        sink.try_extend_from_slice(self.data.as_slice())
    }

    /// Attempts to construct a signature context from the utf-8 bytes
    /// of the given string. Returns `None` if the passed string
    /// is invalid to use for signature context bytes.
    pub const fn from_string_bytes(data_str: &str) -> Result<Self, SignatureContextInvalidLength> {
        let data_bytes = data_str.as_bytes();
        Self::from_bytes(data_bytes)
    }

    #[allow(clippy::indexing_slicing)]
    const fn from_bytes(bytes: &[u8]) -> Result<Self, SignatureContextInvalidLength> {
        let num_bytes = bytes.len();
        if num_bytes < MIN_SIGNATURE_CONTEXT_LEN || num_bytes > MAX_SIGNATURE_CONTEXT_LEN {
            Err(SignatureContextInvalidLength)
        } else {
            let mut array = [0u8; MAX_SIGNATURE_CONTEXT_LEN];
            let mut i = 0;
            while i < num_bytes {
                array[i] = bytes[i];
                i += 1;
            }
            let data = ArrayView::const_from_array(array, bytes.len());
            Ok(Self { data })
        }
    }
}

/// Error raised when attempting to construct a
/// [`SignatureContext`] out of data with an
/// invalid length in bytes.
#[derive(Debug)]
pub struct SignatureContextInvalidLength;

impl TryFrom<&[u8]> for SignatureContext {
    type Error = SignatureContextInvalidLength;

    fn try_from(value: &[u8]) -> Result<Self, Self::Error> {
        Self::from_bytes(value)
    }
}
