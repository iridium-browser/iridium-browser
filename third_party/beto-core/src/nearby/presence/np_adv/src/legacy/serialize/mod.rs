// Copyright 2022 Google LLC
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

//! Serialization for V0 advertisements.
//!
//! # Examples
//!
//! Serializing a plaintext advertisement:
//!
//! ```
//! use np_adv::{legacy::{data_elements::*, serialize::*}, shared_data::*, PublicIdentity};
//! use np_adv::shared_data::TxPower;
//!
//! let mut builder = AdvBuilder::new(PublicIdentity::default());
//! builder
//!     .add_data_element(TxPowerDataElement::from(TxPower::try_from(3).expect("3 is a valid TxPower value")))
//!     .unwrap();
//! let packet = builder.into_advertisement().unwrap();
//! assert_eq!(
//!     &[
//!         0x00, // Adv Header
//!         0x03, // Public DE header
//!         0x15, 0x03, // tx power de
//!     ],
//!     packet.as_slice()
//! );
//! ```
//!
//! Serializing an encrypted advertisement:
//!
//! ```
//! use np_adv::{shared_data::*, de_type::*, legacy::{de_type::*, data_elements::*, serialize::*}};
//! use crypto_provider::CryptoProvider;
//! use crypto_provider_default::CryptoProviderImpl;
//! use ldt_np_adv::{salt_padder, LegacySalt, LdtEncrypterXtsAes128};
//!
//! // Generate these from proper CSPRNGs -- using fixed data here
//! let metadata_key = [0x33; 14];
//! let salt = LegacySalt::from([0x01, 0x02]);
//! let key_seed = [0x44; 32];
//! let ldt_enc = LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(
//!     &np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed).legacy_ldt_key()
//! );
//!
//! let mut builder = AdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
//!     EncryptedIdentityDataElementType::Private,
//!     salt,
//!     metadata_key,
//!     ldt_enc,
//! ));
//!
//! builder
//!     .add_data_element(TxPowerDataElement::from(TxPower::try_from(3).expect("3 is a valid TxPower value")))
//!     .unwrap();
//!
//! let packet = builder.into_advertisement().unwrap();
//! ```
use crate::{
    de_type::{EncryptedIdentityDataElementType, IdentityDataElementType},
    legacy::{
        de_type::{DataElementType, DeActualLength, DeEncodedLength, PlainDataElementType},
        Ciphertext, PacketFlavor, Plaintext, BLE_ADV_SVC_CONTENT_LEN, NP_MAX_DE_CONTENT_LEN,
    },
    DeLengthOutOfRange, NoIdentity, PublicIdentity,
};
use array_view::ArrayView;
use core::{convert, fmt, marker};
use crypto_provider::CryptoProvider;
use ldt_np_adv::NP_LEGACY_METADATA_KEY_LEN;

#[cfg(test)]
mod tests;

/// An identity used in serializing an advertisement.
pub trait Identity: fmt::Debug {
    /// The flavor of packet this identity produces
    type Flavor: PacketFlavor;
    /// The error returned if postprocessing fails
    type Error: fmt::Debug;
    /// How much space needs to be reserved for this identity's prefix bytes
    const OVERHEAD_LEN: usize;

    /// Perform identity-specific manipulation to the serialized DEs to produce the final
    /// advertisement format.
    ///
    /// `buf` is `OVERHEAD_LEN` bytes set aside for the identity's use, followed by all of the DEs
    /// added to a packet. It does not include the NP top level header.
    ///
    /// Returns `Ok` if postprocessing was successful and the packet is finished, or `Err` if
    /// postprocessing failed and the packet should be discarded.
    fn postprocess(&self, buf: &mut [u8]) -> Result<(), Self::Error>;
}

lazy_static::lazy_static! {
    // Avoid either a panic-able code path or an error case that never happens by precalculating.
    static ref PUBLIC_IDENTITY_DE_HEADER: u8 =
        encode_de_header_actual_len(DataElementType::PublicIdentity, DeActualLength::ZERO).unwrap();
}

impl Identity for PublicIdentity {
    type Flavor = Plaintext;
    type Error = convert::Infallible;
    // 1 byte for public DE header (0 content)
    const OVERHEAD_LEN: usize = 1;

    fn postprocess(&self, buf: &mut [u8]) -> Result<(), Self::Error> {
        buf[0] = *PUBLIC_IDENTITY_DE_HEADER;

        Ok(())
    }
}

impl Identity for NoIdentity {
    type Flavor = Plaintext;
    type Error = convert::Infallible;
    // No DE header
    const OVERHEAD_LEN: usize = 0;

    fn postprocess(&self, _buf: &mut [u8]) -> Result<(), Self::Error> {
        Ok(())
    }
}

/// Identity used for encrypted packets (private, trusted, provisioned) that encrypts other DEs
/// (as well as the metadata key).
pub struct LdtIdentity<C: CryptoProvider> {
    de_type: EncryptedIdentityDataElementType,
    salt: ldt_np_adv::LegacySalt,
    metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN],
    ldt_enc: ldt_np_adv::LdtEncrypterXtsAes128<C>,
    // keep C parameter alive for when A disappears into it
    _crypto_provider: marker::PhantomData<C>,
}

// Exclude sensitive members
impl<C: CryptoProvider> fmt::Debug for LdtIdentity<C> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "LdtIdentity {{ de_type: {:?}, salt: {:X?} }}", self.de_type, self.salt)
    }
}

impl<C: CryptoProvider> LdtIdentity<C> {
    /// Build an `LdtIdentity` for the provided identity, salt, metadata key, and ldt.
    pub fn new(
        de_type: EncryptedIdentityDataElementType,
        salt: ldt_np_adv::LegacySalt,
        metadata_key: [u8; NP_LEGACY_METADATA_KEY_LEN],
        ldt_enc: ldt_np_adv::LdtEncrypterXtsAes128<C>,
    ) -> LdtIdentity<C> {
        LdtIdentity { de_type, salt, metadata_key, ldt_enc, _crypto_provider: marker::PhantomData }
    }
}

impl<C: CryptoProvider> Identity for LdtIdentity<C> {
    type Flavor = Ciphertext;
    type Error = LdtPostprocessError;
    // Identity DE header + salt + metadata key
    const OVERHEAD_LEN: usize = 17;

    fn postprocess(&self, buf: &mut [u8]) -> Result<(), LdtPostprocessError> {
        let de_type = self.de_type.as_identity_data_element_type();
        // there's space for the identity DE header byte, but we don't count that in the DE length
        let actual_len: DeActualLength = buf
            .len()
            .checked_sub(1)
            .ok_or(LdtPostprocessError::InvalidLength)
            .and_then(|len| len.try_into().map_err(|_e| LdtPostprocessError::InvalidLength))?;
        // header
        buf[0] = encode_de_header_actual_len(id_de_type_as_generic_de_type(de_type), actual_len)
            .map_err(|_e| LdtPostprocessError::InvalidLength)?;
        buf[1..3].copy_from_slice(self.salt.bytes().as_slice());
        buf[3..17].copy_from_slice(&self.metadata_key);

        // encrypt everything after DE header and salt
        self.ldt_enc.encrypt(&mut buf[3..], &ldt_np_adv::salt_padder::<16, C>(self.salt)).map_err(
            |e| match e {
                // too short, not enough DEs -- should be caught by length validation above, though
                ldt::LdtError::InvalidLength(_) => LdtPostprocessError::InvalidLength,
            },
        )
    }
}

/// Something went wrong, or preconditions were not met, during identity postprocessing.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum LdtPostprocessError {
    /// The minimum size (2 bytes) or maximum size (6 bytes) of DE payload was not met
    InvalidLength,
}

/// Accumulates DEs, then massages the serialized DEs with the configured identity to produce
/// the final advertisement.
#[derive(Debug)]
pub struct AdvBuilder<I: Identity> {
    /// The first byte is for the adv header, then the next I::OVERHEAD_LEN bytes are set aside for
    /// use by [Identity::postprocess].
    buffer: [u8; BLE_ADV_SVC_CONTENT_LEN],
    /// How much of the buffer is consumed.
    /// Always <= buffer length
    len: usize,
    identity: I,
}

impl<I: Identity> AdvBuilder<I> {
    /// Create an empty AdvBuilder with the provided identity.
    pub fn new(identity: I) -> AdvBuilder<I> {
        // adv header + identity overhead
        let len = 1 + I::OVERHEAD_LEN;
        // check for broken identities
        assert!(len < BLE_ADV_SVC_CONTENT_LEN);
        AdvBuilder {
            // conveniently the first byte is already 0, which is the correct header for v0.
            // 3 bit version (000 since this is version 0), 5 bit reserved (also all 0)
            buffer: [0; BLE_ADV_SVC_CONTENT_LEN],
            len,
            identity,
        }
    }

    /// Add the data element to the packet buffer, if there is space.
    pub fn add_data_element<B: ToDataElementBundle<I::Flavor>>(
        &mut self,
        data_element: B,
    ) -> Result<(), AddDataElementError> {
        let remaining = self.buffer.len() - self.len;
        let bundle = data_element.to_de_bundle();
        // length including header byte
        let de_slice = bundle.contents_as_slice();
        let de_total_len = 1 + de_slice.len();
        if remaining < de_total_len {
            return Err(AddDataElementError::InsufficientAdvSpace);
        }

        // header
        self.buffer[self.len] =
            encode_de_header(bundle.de_type.as_generic_de_type(), bundle.encoded_len);
        self.len += 1;
        // de contents
        self.buffer[self.len..self.len + de_slice.len()].copy_from_slice(de_slice);
        self.len += de_slice.len();
        Ok(())
    }

    /// Return the finished advertisement (adv header + DEs), or `None` if the adv could not be
    /// built.
    pub fn into_advertisement(
        mut self,
    ) -> Result<ArrayView<u8, BLE_ADV_SVC_CONTENT_LEN>, I::Error> {
        // encrypt, if applicable
        self.identity
            // skip adv header for postprocessing
            .postprocess(&mut self.buffer[1..self.len])
            .map(|_| ArrayView::try_from_array(self.buffer, self.len).expect("len is always valid"))
    }
}

/// Errors that can occur for [AdvBuilder.add_data_element].
#[derive(Debug, PartialEq, Eq)]
pub enum AddDataElementError {
    /// The provided DE can't fit in the remaining space
    InsufficientAdvSpace,
}

/// The serialized form of a data element.
#[derive(Clone, Debug)]
pub struct DataElementBundle<F: PacketFlavor> {
    de_type: PlainDataElementType,
    /// Data element payload
    data: DePayload,
    /// The header-encoded form of `data`'s length
    encoded_len: DeEncodedLength,
    /// Type marker for whether this DE can be used in encrypted or plaintext packets.
    flavor: marker::PhantomData<F>,
}

impl<F: PacketFlavor> DataElementBundle<F> {
    /// Returns `Err` if the provided `data` or requested [PacketFlavor] are not valid for
    /// `de_type`.
    pub(crate) fn try_from(
        de_type: PlainDataElementType,
        data: &[u8],
    ) -> Result<DataElementBundle<F>, DeBundleError> {
        if !de_type.supports_flavor(F::ENUM_VARIANT) {
            return Err(DeBundleError::InvalidFlavor);
        }

        let mut buffer = [0; NP_MAX_DE_CONTENT_LEN];
        buffer
            .get_mut(0..data.len())
            .map(|dest| dest.copy_from_slice(data))
            .ok_or(DeBundleError::InvalidLength)?;

        let payload: DePayload = ArrayView::try_from_array(buffer, data.len())
            .expect("data already copied into buffer")
            .into();

        // get the encoded length now so we know the length is valid for the DE type
        let encoded_len = de_type
            .as_generic_de_type()
            .encoded_len_for_actual_len(payload.len())
            .map_err(|_| DeBundleError::InvalidLength)?;

        Ok(DataElementBundle { de_type, data: payload, encoded_len, flavor: marker::PhantomData })
    }

    /// The data contained in the DE, excluding the header byte
    pub(crate) fn contents_as_slice(&self) -> &[u8] {
        self.data.as_slice()
    }
}

/// Errors that can occur when building a [DataElementBundle]
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum DeBundleError {
    /// The DE type does not support the requested flavor
    InvalidFlavor,
    /// The data is too long to fit in a DE, or is invalid for the DE type
    InvalidLength,
}

/// Implemented by higher-level types that represent data elements.
pub trait ToDataElementBundle<F: PacketFlavor> {
    /// Serialize `self` into the impl-specific byte encoding used for that DE type.
    fn to_de_bundle(&self) -> DataElementBundle<F>;
}

// for cases where it's more convenient to already have a DataElementBundle than a DE
impl<F: PacketFlavor> ToDataElementBundle<F> for DataElementBundle<F> {
    fn to_de_bundle(&self) -> DataElementBundle<F> {
        Self {
            de_type: self.de_type,
            data: self.data.clone(),
            encoded_len: self.encoded_len,
            flavor: marker::PhantomData,
        }
    }
}

// Biggest size a DE could possibly be is the entire payload
#[derive(Clone, PartialEq, Eq, Debug)]
struct DePayload {
    payload: ArrayView<u8, { NP_MAX_DE_CONTENT_LEN }>,
}

impl DePayload {
    /// The actual length of the payload
    fn len(&self) -> DeActualLength {
        self.payload
            .len()
            .try_into()
            .expect("Payload is an array of the max size which always has a valid length")
    }

    fn as_slice(&self) -> &[u8] {
        self.payload.as_slice()
    }
}

impl From<ArrayView<u8, { NP_MAX_DE_CONTENT_LEN }>> for DePayload {
    fn from(array: ArrayView<u8, { NP_MAX_DE_CONTENT_LEN }>) -> Self {
        Self { payload: array }
    }
}

/// Encode a DE type and length into a DE header byte.
pub(crate) fn encode_de_header(de_type: DataElementType, header_len: DeEncodedLength) -> u8 {
    // 4 high bits are length, 4 low bits are type
    (header_len.as_u8() << 4) | de_type.type_code().as_u8()
}

/// Encode a DE type and length into a DE header byte.
pub(crate) fn encode_de_header_actual_len(
    de_type: DataElementType,
    header_len: DeActualLength,
) -> Result<u8, DeLengthOutOfRange> {
    de_type
        .encoded_len_for_actual_len(header_len)
        // 4 high bits are length, 4 low bits are type
        .map(|len| encode_de_header(de_type, len))
}

pub(crate) fn id_de_type_as_generic_de_type(
    id_de_type: IdentityDataElementType,
) -> DataElementType {
    match id_de_type {
        IdentityDataElementType::Private => DataElementType::PrivateIdentity,
        IdentityDataElementType::Trusted => DataElementType::TrustedIdentity,
        IdentityDataElementType::Public => DataElementType::PublicIdentity,
        IdentityDataElementType::Provisioned => DataElementType::ProvisionedIdentity,
    }
}
