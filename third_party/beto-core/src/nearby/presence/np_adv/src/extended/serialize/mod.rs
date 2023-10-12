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

//! Serialization support for V1 advertisements.
//!
//! # Examples
//!
//! Serialize some DEs without an adv salt:
//!
//! ```
//! use np_adv::{
//!     extended::{data_elements::*, serialize::*, de_type::DeType },
//!     PublicIdentity
//! };
//! use np_adv::shared_data::TxPower;
//!
//! // no section identities or DEs need salt in this example
//! let mut adv_builder = AdvBuilder::new();
//! let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();
//!
//! section_builder.add_de(|_salt| TxPowerDataElement::from(TxPower::try_from(3).unwrap())).unwrap();
//!
//! // add some other DE with type = 1000
//! section_builder.add_de_res(|_salt|
//!     GenericDataElement::try_from( DeType::from(1000_u32), &[10, 11, 12, 13])
//! ).unwrap();
//!
//! section_builder.add_to_advertisement();
//!
//! assert_eq!(
//!     &[
//!         0x20, // adv header
//!         10, // section header
//!         0x03, // public identity
//!         0x15, 3, // tx power
//!         0x84, 0x87, 0x68, 10, 11, 12, 13, // other DE
//!     ],
//!     adv_builder.into_advertisement().as_slice()
//! );
//! ```
//!
//! Serialize some DEs in an adv with an encrypted section:
//!
//! ```
//! use np_adv::{
//!     de_type::EncryptedIdentityDataElementType,
//!     extended::{data_elements::*, serialize::*, de_type::DeType },
//! };
//! use rand::{Rng as _, SeedableRng as _};
//! use crypto_provider::{CryptoProvider, CryptoRng};
//! use crypto_provider_default::CryptoProviderImpl;
//! use np_adv::shared_data::TxPower;
//!
//! let mut adv_builder = AdvBuilder::new();
//!
//! // these would come from the credential//!
//! let mut rng = <CryptoProviderImpl as CryptoProvider>::CryptoRng::new();
//! let metadata_key: [u8; 16] = rng.gen();
//! let key_seed: [u8; 32] = rng.gen();
//! // use your preferred crypto impl
//! let key_seed_hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed);
//!
//! let mut section_builder = adv_builder.section_builder(MicEncrypted::new_random_salt(
//!     &mut rng,
//!     EncryptedIdentityDataElementType::Private,
//!     &metadata_key,
//!     &key_seed_hkdf,
//! )).unwrap();
//!
//! section_builder.add_de(|_salt| TxPowerDataElement::from(TxPower::try_from(3).unwrap())).unwrap();
//!
//! // add some other DE with type = 1000
//! section_builder.add_de_res(|salt|
//!     GenericDataElement::try_from(
//!         DeType::from(1000_u32),
//!         &do_fancy_crypto(salt.derive::<16>().expect("16 is a valid HKDF length")))
//! ).unwrap();
//!
//! section_builder.add_to_advertisement();
//!
//! // can't assert much about this since most of it is random
//! assert_eq!(
//!     0x20, // adv header
//!     adv_builder.into_advertisement().as_slice()[0]
//! );
//!
//! // A hypothetical function that uses the per-DE derived salt to do something like encrypt or
//! // otherwise scramble data
//! fn do_fancy_crypto(derived_salt: [u8; 16]) -> [u8; 16] {
//!     // flipping bits is just a nonsense example, do something real here
//!     derived_salt.iter().map(|b| !b)
//!         .collect::<Vec<_>>()
//!         .try_into().expect("array sizes match")
//! }
//! ```
use crate::extended::NP_V1_ADV_MAX_SECTION_COUNT;
use crate::{
    de_type::{EncryptedIdentityDataElementType, IdentityDataElementType},
    extended::{
        data_elements::EncryptionInfoDataElement,
        de_type::{DeType, ExtendedDataElementType},
        deserialize::{EncryptedIdentityMetadata, EncryptionInfo, SectionMic},
        section_signature_payload::*,
        to_array_view, DeLength, BLE_ADV_SVC_CONTENT_LEN, NP_ADV_MAX_SECTION_LEN,
    },
    DeLengthOutOfRange, PublicIdentity, NP_SVC_UUID,
};
use array_view::ArrayView;
use core::fmt;
use core::marker::PhantomData;
use crypto_provider::{
    aes::{
        ctr::{AesCtr, AesCtrNonce, NonceAndCounter},
        Aes128Key,
    },
    hmac::Hmac,
    CryptoProvider, CryptoRng,
};
use np_hkdf::v1_salt;
use np_hkdf::v1_salt::{DataElementOffset, V1Salt};
use sink::Sink;

#[cfg(test)]
pub(crate) mod adv_tests;
#[cfg(test)]
mod de_header_tests;
#[cfg(test)]
pub(crate) mod section_tests;
#[cfg(test)]
mod test_vectors;

/// Builder for V1 advertisements.
#[derive(Debug)]
pub struct AdvBuilder {
    /// Contains the adv header byte
    adv: tinyvec::ArrayVec<[u8; BLE_ADV_SVC_CONTENT_LEN]>,
    section_count: usize,
}

impl AdvBuilder {
    /// Build an [AdvBuilder].
    pub fn new() -> AdvBuilder {
        let mut adv = tinyvec::ArrayVec::new();
        // version 1, 0bVVVRRRRR
        adv.push(0b00100000);

        AdvBuilder { adv, section_count: 0 }
    }

    /// Create a section builder.
    ///
    /// Returns `Err` if there isn't room in the advertisement for the section at its minimum length
    /// with its chosen identity.
    ///
    /// The builder will not accept more DEs than can fit given the space already used in the
    /// advertisement by previous sections, if any.
    ///
    /// Once the builder is populated, add it to the originating advertisement with
    /// [SectionBuilder.add_to_advertisement].
    pub fn section_builder<I: SectionIdentity>(
        &mut self,
        identity: I,
    ) -> Result<SectionBuilder<I>, AddSectionError> {
        // section header and identity prefix
        let prefix_len = 1 + I::PREFIX_LEN;
        let minimum_section_len = prefix_len + I::SUFFIX_LEN;
        // the max overall len available to the section
        let available_len = self.adv.capacity() - self.adv.len();

        if available_len < minimum_section_len {
            return Err(AddSectionError::InsufficientAdvSpace);
        }

        if self.section_count >= NP_V1_ADV_MAX_SECTION_COUNT {
            return Err(AddSectionError::MaxSectionCountExceeded);
        }

        let mut section = tinyvec::ArrayVec::new();
        // placeholder for section header and identity prefix
        section.resize(prefix_len, 0);

        Ok(SectionBuilder {
            section: CapacityLimitedVec {
                vec: section,
                // won't underflow: checked above
                capacity: available_len - I::SUFFIX_LEN,
            },
            identity,
            adv_builder: self,
            next_de_offset: I::INITIAL_DE_OFFSET,
        })
    }

    /// Convert the builder into an encoded advertisement.
    pub fn into_advertisement(self) -> EncodedAdvertisement {
        EncodedAdvertisement { adv: to_array_view(self.adv) }
    }

    /// Add the section, which must have come from a SectionBuilder generated from this, into this
    /// advertisement.
    fn add_section(&mut self, section: EncodedSection) {
        self.adv
            .try_extend_from_slice(section.as_slice())
            .expect("section capacity enforced in the section builder");
        self.section_count += 1;
    }

    fn header_byte(&self) -> u8 {
        self.adv[0]
    }
}

impl Default for AdvBuilder {
    fn default() -> Self {
        Self::new()
    }
}

/// Errors that can occur when adding a section to an advertisement
#[derive(Debug, PartialEq, Eq)]
pub enum AddSectionError {
    /// The advertisement doesn't have enough space to hold the minimum size of the section
    InsufficientAdvSpace,
    /// The advertisement can only hold a maximum of NP_V1_ADV_MAX_SECTION_COUNT number of sections
    MaxSectionCountExceeded,
}

/// Derived salt for an individual data element.
pub struct DeSalt<C: CryptoProvider> {
    salt: V1Salt<C>,
    de_offset: DataElementOffset,
}

impl<C: CryptoProvider> DeSalt<C> {
    /// Derive salt of the requested length.
    ///
    /// The length must be a valid HKDF-SHA256 length.
    pub fn derive<const N: usize>(&self) -> Option<[u8; N]> {
        self.salt.derive(Some(self.de_offset))
    }
}

/// An encoded NP V1 advertisement, starting with the NP advertisement header byte.
#[derive(Debug, PartialEq, Eq)]
pub struct EncodedAdvertisement {
    adv: ArrayView<u8, BLE_ADV_SVC_CONTENT_LEN>,
}

impl EncodedAdvertisement {
    /// Returns the advertisement as a slice.
    pub fn as_slice(&self) -> &[u8] {
        self.adv.as_slice()
    }
}

/// The encoded form of an advertisement section
type EncodedSection = ArrayView<u8, NP_ADV_MAX_SECTION_LEN>;

/// Accumulates data elements and encodes them into a section.
#[derive(Debug)]
pub struct SectionBuilder<'a, I: SectionIdentity> {
    /// Contains the section header, the identity-specified overhead, and any DEs added
    pub(crate) section: CapacityLimitedVec<u8, NP_ADV_MAX_SECTION_LEN>,
    identity: I,
    /// mut ref to enforce only one active section builder at a time
    adv_builder: &'a mut AdvBuilder,
    next_de_offset: DataElementOffset,
}

impl<'a, I: SectionIdentity> SectionBuilder<'a, I> {
    /// Add this builder to the advertisement that created it.
    pub fn add_to_advertisement(self) {
        let adv_builder = self.adv_builder;
        adv_builder.add_section(Self::build_section(
            self.section.into_inner(),
            self.identity,
            adv_builder,
        ))
    }

    /// Add a data element to the section with a closure that returns a `Result`.
    ///
    /// The provided `build_de` closure will be invoked with the derived salt for this DE.
    pub fn add_de_res<W: WriteDataElement, E, F: FnOnce(I::DerivedSalt) -> Result<W, E>>(
        &mut self,
        build_de: F,
    ) -> Result<(), AddDataElementError<E>> {
        let writer = build_de(self.identity.de_salt(self.next_de_offset))
            .map_err(AddDataElementError::BuildDeError)?;

        let orig_len = self.section.len();
        // since we own the writer, and it's immutable, no race risk writing header w/ len then
        // the contents as long as it's not simply an incorrect impl
        let de_header = writer.de_header();
        let content_len = self
            .section
            .try_extend_from_slice(de_header.serialize().as_slice())
            .ok_or(AddDataElementError::InsufficientSectionSpace)
            .and_then(|_| {
                let after_header_len = self.section.len();
                writer
                    .write_de_contents(&mut self.section)
                    .ok_or(AddDataElementError::InsufficientSectionSpace)
                    .map(|_| self.section.len() - after_header_len)
            })
            .map_err(|e| {
                // if anything went wrong, truncate any partial writes (e.g. just the header)
                self.section.truncate(orig_len);
                e
            })?;

        if content_len != de_header.len.as_usize() {
            panic!(
                "Buggy WriteDataElement impl: header len {}, actual written len {}",
                de_header.len.as_usize(),
                content_len
            );
        }

        self.next_de_offset = self.next_de_offset.incremented();

        Ok(())
    }

    /// Add a data element to the section with a closure that returns the data element directly.
    ///
    /// The provided `build_de` closure will be invoked with the derived salt for this DE.
    pub fn add_de<W: WriteDataElement, F: FnOnce(I::DerivedSalt) -> W>(
        &mut self,
        build_de: F,
    ) -> Result<(), AddDataElementError<()>> {
        self.add_de_res(|derived_salt| Ok::<_, ()>(build_de(derived_salt)))
    }

    /// Convert a section builder's contents into an encoded section.
    ///
    /// Implemented without self to avoid partial-move issues.
    fn build_section(
        mut section_contents: tinyvec::ArrayVec<[u8; NP_ADV_MAX_SECTION_LEN]>,
        mut identity: I,
        adv_builder: &AdvBuilder,
    ) -> EncodedSection {
        // there is space because the capacity for DEs was restricted to allow it
        section_contents.resize(section_contents.len() + I::SUFFIX_LEN, 0);

        section_contents[0] = section_contents
            .len()
            .try_into()
            .ok()
            .and_then(|len: u8| len.checked_sub(1))
            .expect("section length is always <=255 and non-negative");

        identity.postprocess(
            adv_builder.header_byte(),
            section_contents[0],
            &mut section_contents[1..],
        );

        to_array_view(section_contents)
    }
}

/// Errors for adding a DE to a section
#[derive(Debug, PartialEq, Eq)]
pub enum AddDataElementError<E> {
    /// An error occurred when invoking the DE builder closure.
    BuildDeError(E),
    /// Too much data to fit into the section
    InsufficientSectionSpace,
}

/// The identity used for an individual section.
pub trait SectionIdentity {
    /// How much space needs to be reserved for this identity's prefix bytes after the section
    /// header and before other DEs
    const PREFIX_LEN: usize;

    /// How much space needs to be reserved after the DEs
    const SUFFIX_LEN: usize;

    /// The DE offset to use for any DEs added to the section
    const INITIAL_DE_OFFSET: DataElementOffset;

    /// Postprocess the contents of the section (the data after the section header byte), which will
    /// start with [Self::PREFIX_LEN] bytes set aside for the identity's use, and similarly end with
    /// [Self::SUFFIX_LEN] bytes, with DEs (if any) in the middle.
    fn postprocess(&mut self, adv_header_byte: u8, section_header: u8, section_contents: &mut [u8]);

    /// The type of derived salt produced for a DE sharing a section with this identity.
    type DerivedSalt;

    /// Produce a `Self::Output` salt for a DE.
    fn de_salt(&self, de_offset: DataElementOffset) -> Self::DerivedSalt;
}

impl SectionIdentity for PublicIdentity {
    /// 1 byte of public identity DE header
    const PREFIX_LEN: usize = 1;
    const SUFFIX_LEN: usize = 0;
    /// Room for the public DE
    const INITIAL_DE_OFFSET: DataElementOffset = DataElementOffset::ZERO.incremented();

    fn postprocess(
        &mut self,
        _adv_header_byte: u8,
        _section_header: u8,
        section_contents: &mut [u8],
    ) {
        section_contents[0..1].copy_from_slice(
            DeHeader { len: DeLength::ZERO, de_type: IdentityDataElementType::Public.type_code() }
                .serialize()
                .as_slice(),
        )
    }

    type DerivedSalt = ();

    fn de_salt(&self, _de_offset: DataElementOffset) -> Self::DerivedSalt {}
}

/// Encrypts the data elements and protects integrity with a MIC using key material derived from
/// an NP identity.
pub struct MicEncrypted<'a, C: CryptoProvider> {
    identity_type: EncryptedIdentityDataElementType,
    salt: V1Salt<C>,
    identity_payload: &'a [u8; 16],
    aes_key: Aes128Key,
    mic_hmac_key: np_hkdf::NpHmacSha256Key<C>,
}

impl<'a, C: CryptoProvider> MicEncrypted<'a, C> {
    /// Build a [MicEncrypted] from the provided identity info with a random salt.
    pub fn new_random_salt(
        rng: &mut C::CryptoRng,
        identity_type: EncryptedIdentityDataElementType,
        identity_payload: &'a [u8; 16],
        keys: &impl np_hkdf::UnsignedSectionKeys<C>,
    ) -> Self {
        let salt: V1Salt<C> = rng.gen::<[u8; 16]>().into();
        Self::new(identity_type, salt, identity_payload, keys)
    }

    /// Build a [MicEncrypted] from the provided identity info.
    fn new(
        identity_type: EncryptedIdentityDataElementType,
        salt: V1Salt<C>,
        identity_payload: &'a [u8; 16],
        keys: &impl np_hkdf::UnsignedSectionKeys<C>,
    ) -> Self {
        MicEncrypted {
            identity_type,
            salt,
            identity_payload,
            aes_key: keys.aes_key(),
            mic_hmac_key: keys.hmac_key(),
        }
    }

    /// Build a [MicEncrypted] from the provided identity info. Exposed outside of this crate for
    /// testing purposes only.
    #[cfg(any(test, feature = "testing"))]
    pub fn new_for_testing(
        identity_type: EncryptedIdentityDataElementType,
        salt: V1Salt<C>,
        identity_payload: &'a [u8; 16],
        keys: &impl np_hkdf::UnsignedSectionKeys<C>,
    ) -> Self {
        Self::new(identity_type, salt, identity_payload, keys)
    }
}

impl<'a, C: CryptoProvider> SectionIdentity for MicEncrypted<'a, C> {
    const PREFIX_LEN: usize =
        EncryptionInfo::TOTAL_DE_LEN + EncryptedIdentityMetadata::TOTAL_DE_LEN;
    /// Length of mic
    const SUFFIX_LEN: usize = SectionMic::CONTENTS_LEN;
    /// Room for the mic, encryption info, and identity DEs
    const INITIAL_DE_OFFSET: DataElementOffset =
        DataElementOffset::ZERO.incremented().incremented();

    fn postprocess(
        &mut self,
        adv_header_byte: u8,
        section_header: u8,
        section_contents: &mut [u8],
    ) {
        // prefix byte layout:
        // 0-18: Encryption Info DE (header + scheme + salt)
        // 19-20: Identity DE header
        // 21-36: Identity DE contents (metadata key)

        // Encryption Info DE
        let encryption_info_bytes = EncryptionInfoDataElement::mic(
            self.salt.as_slice().try_into().expect("Salt should be 16 bytes"),
        )
        .serialize();
        section_contents[0..19].copy_from_slice(&encryption_info_bytes);

        // Identity DE
        let identity_header = identity_de_header(self.identity_type, self.identity_payload);
        section_contents[19..21].copy_from_slice(identity_header.serialize().as_slice());
        section_contents[21..37].copy_from_slice(self.identity_payload);

        // DE offset for identity is 1: Encryption Info DE, Identity DE, then other DEs
        let nonce: AesCtrNonce = self
            .de_salt(v1_salt::DataElementOffset::from(1))
            .derive()
            .expect("AES-CTR nonce is a valid HKDF length");

        let mut cipher = C::AesCtr128::new(&self.aes_key, NonceAndCounter::from_nonce(nonce));

        let ciphertext_end = section_contents.len() - SectionMic::CONTENTS_LEN;

        // encrypt just the part that should be ciphertext: identity DE contents and subsequent DEs
        cipher.encrypt(&mut section_contents[21..ciphertext_end]);

        // calculate MAC per the spec
        let mut section_hmac = self.mic_hmac_key.build_hmac();
        // svc uuid
        section_hmac.update(NP_SVC_UUID.as_slice());
        // adv header
        section_hmac.update(&[adv_header_byte]);
        // section header
        section_hmac.update(&[section_header]);
        // encryption info
        section_hmac.update(&encryption_info_bytes);
        // derived salt
        section_hmac.update(&nonce);
        // identity header + ciphertext
        section_hmac.update(&section_contents[19..ciphertext_end]);

        let mic: [u8; 32] = section_hmac.finalize();

        // write truncated MIC
        section_contents[ciphertext_end..].copy_from_slice(&mic[..SectionMic::CONTENTS_LEN]);
    }

    type DerivedSalt = DeSalt<C>;

    fn de_salt(&self, de_offset: DataElementOffset) -> Self::DerivedSalt {
        DeSalt { salt: V1Salt::from(*self.salt.as_array_ref()), de_offset }
    }
}

/// Encrypts the data elements and protects integrity with an np_ed25519 signature
/// using key material derived from an NP identity.
pub struct SignedEncrypted<'a, C: CryptoProvider> {
    identity_type: EncryptedIdentityDataElementType,
    salt: V1Salt<C>,
    metadata_key: &'a [u8; 16],
    key_pair: &'a np_ed25519::KeyPair<C>,
    aes_key: Aes128Key,
    _marker: PhantomData<C>,
}

impl<'a, C: CryptoProvider> SignedEncrypted<'a, C> {
    /// Build a [SignedEncrypted] from the provided identity material with a random salt.
    pub fn new_random_salt(
        rng: &mut C::CryptoRng,
        identity_type: EncryptedIdentityDataElementType,
        metadata_key: &'a [u8; 16],
        key_pair: &'a np_ed25519::KeyPair<C>,
        key_seed_hkdf: &np_hkdf::NpKeySeedHkdf<C>,
    ) -> Self {
        let salt: V1Salt<C> = rng.gen::<[u8; 16]>().into();
        Self::new(identity_type, salt, metadata_key, key_pair, key_seed_hkdf)
    }

    /// Build a [SignedEncrypted] from the provided identity material.
    pub(crate) fn new(
        identity_type: EncryptedIdentityDataElementType,
        salt: V1Salt<C>,
        metadata_key: &'a [u8; 16],
        key_pair: &'a np_ed25519::KeyPair<C>,
        key_seed_hkdf: &np_hkdf::NpKeySeedHkdf<C>,
    ) -> Self {
        Self {
            identity_type,
            salt,
            metadata_key,
            key_pair,
            aes_key: key_seed_hkdf.extended_signed_section_aes_key(),
            _marker: Default::default(),
        }
    }
}

impl<'a, C: CryptoProvider> SectionIdentity for SignedEncrypted<'a, C> {
    const PREFIX_LEN: usize =
        EncryptionInfo::TOTAL_DE_LEN + EncryptedIdentityMetadata::TOTAL_DE_LEN;
    /// Ed25519 signature
    const SUFFIX_LEN: usize = crypto_provider::ed25519::SIGNATURE_LENGTH;
    /// Room for the encryption info and identity DEs
    const INITIAL_DE_OFFSET: DataElementOffset =
        DataElementOffset::ZERO.incremented().incremented();

    fn postprocess(
        &mut self,
        adv_header_byte: u8,
        section_header: u8,
        section_contents: &mut [u8],
    ) {
        let encryption_info_bytes = EncryptionInfoDataElement::signature(
            self.salt.as_slice().try_into().expect("Salt should be 16 bytes"),
        )
        .serialize();
        section_contents[0..19].copy_from_slice(&encryption_info_bytes);

        let identity_header = identity_de_header(self.identity_type, self.metadata_key);
        section_contents[19..21].copy_from_slice(identity_header.serialize().as_slice());
        section_contents[21..37].copy_from_slice(self.metadata_key);

        let nonce: AesCtrNonce = self
            .de_salt(v1_salt::DataElementOffset::from(1))
            .derive()
            .expect("AES-CTR nonce is a valid HKDF length");

        let (before_sig, sig) =
            section_contents.split_at_mut(section_contents.len() - Self::SUFFIX_LEN);
        let (encryption_info, after_encryption_info) =
            before_sig.split_at(EncryptionInfo::TOTAL_DE_LEN);

        let encryption_info: &[u8; EncryptionInfo::TOTAL_DE_LEN] =
            encryption_info.try_into().unwrap();

        // we need to sign the 16-byte IV, which doesn't have to actually fit in the adv, but we
        // don't need a bigger buffer here since we won't be including the 66 bytes for the sig +
        // header.
        // If the stack usage ever becomes a problem, we can investigate pre hashing for the
        // signature.
        let nonce_ref = &nonce;
        let section_signature_payload = SectionSignaturePayload::from_serialized_parts(
            adv_header_byte,
            section_header,
            encryption_info,
            nonce_ref,
            after_encryption_info,
        );

        let signature = section_signature_payload.sign(self.key_pair);

        sig[0..64].copy_from_slice(&signature.to_bytes());

        let mut cipher = C::AesCtr128::new(&self.aes_key, NonceAndCounter::from_nonce(nonce));

        // encrypt just the part that should be ciphertext: identity DE contents and subsequent DEs
        cipher.encrypt(&mut section_contents[21..]);
    }

    type DerivedSalt = DeSalt<C>;

    fn de_salt(&self, de_offset: DataElementOffset) -> Self::DerivedSalt {
        DeSalt { salt: V1Salt::from(*self.salt.as_array_ref()), de_offset }
    }
}

/// For DE structs that only implement one DE type, rather than multi-type impls.
pub trait SingleTypeDataElement {
    /// The DE type for the DE.
    const DE_TYPE: DeType;
}

/// Writes data for a V1 DE into a provided buffer.
///
/// V1 data elements can be hundreds of bytes, so we ideally wouldn't even stack allocate a buffer
/// big enough for that, hence an abstraction that writes into an existing buffer.
pub trait WriteDataElement {
    /// Returns the DE header that will be serialized into the section.
    fn de_header(&self) -> DeHeader;
    /// Write just the contents of the DE, returning `Some` if all contents could be written and
    /// `None` otherwise.
    fn write_de_contents<S: Sink<u8>>(&self, sink: &mut S) -> Option<()>;
}

// convenience impl for &W
impl<W: WriteDataElement> WriteDataElement for &W {
    fn de_header(&self) -> DeHeader {
        (*self).de_header()
    }

    fn write_de_contents<S: Sink<u8>>(&self, sink: &mut S) -> Option<()> {
        (*self).write_de_contents(sink)
    }
}

/// Serialization-specific representation of a DE header
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct DeHeader {
    /// The length of the content of the DE
    len: DeLength,
    pub(crate) de_type: DeType,
}

impl DeHeader {
    /// Build a DeHeader from the provided type and length
    pub fn new(de_type: DeType, len: DeLength) -> Self {
        DeHeader { de_type, len }
    }

    /// Serialize the DE header as per the V1 DE header format:
    /// - 1 byte form for length <= 3 bits, type <= 4 bits: `0LLLTTTT`
    /// - multi byte form: `0b1LLLLLLL [0b1TTTTTTT ...] 0b0TTTTTTT`
    ///   - the shortest possible encoding must be used (no empty prefix type bytes)
    ///
    /// We assume that a 32-bit de type is sufficient, which would take at most 5 7-bit chunks to
    /// encode, resulting in a total length of 6 bytes with the initial length byte.
    pub(crate) fn serialize(&self) -> ArrayView<u8, 6> {
        let mut buffer = [0; 6];
        let de_type = self.de_type.as_u32();
        let hi_bit = 0x80_u8;
        let len = self.len.len;
        if len < 8 && de_type < 16 {
            buffer[0] = len << 4 | de_type as u8;
            ArrayView::try_from_array(buffer, 1).expect("1 is a valid length")
        } else {
            // length w/ extended bit
            buffer[0] = hi_bit | len;

            // expand to a u64 so we can represent all 5 7-bit chunks of a u32, shifted so that
            // it fills the top 5 * 7 = 35 bits after the high bit, which is left unset so that
            // the MSB can be interpreted as a 7-bit chunk with an unset high bit.
            let mut type64 = (de_type as u64) << (64 - 35 - 1);
            let mut remaining_chunks = 5;
            let mut chunks_written = 0;
            // write 7 bit chunks, skipping leading 0 chunks
            while remaining_chunks > 0 {
                let chunk = type64.to_be_bytes()[0];
                remaining_chunks -= 1;

                // shift 7 more bits up, leaving the high bit unset
                type64 = (type64 << 7) & (u64::MAX >> 1);

                if chunks_written == 0 && chunk == 0 {
                    // skip leading all-zero chunks
                    continue;
                }

                buffer[1 + chunks_written] = chunk;
                chunks_written += 1;
            }
            if chunks_written > 0 {
                // fill in high bits for all but the last
                for byte in buffer[1..chunks_written].iter_mut() {
                    *byte |= hi_bit;
                }

                ArrayView::try_from_array(buffer, 1 + chunks_written).expect("length is at most 6")
            } else {
                // type byte is a leading 0 bit w/ 0 type, so use the existing 0 byte
                ArrayView::try_from_array(buffer, 2).expect("2 is a valid length")
            }
        }
    }
}

fn identity_de_header(
    id_type: EncryptedIdentityDataElementType,
    metadata_key: &[u8; 16],
) -> DeHeader {
    DeHeader {
        de_type: id_type.type_code(),
        len: metadata_key
            .len()
            .try_into()
            .map_err(|_e| DeLengthOutOfRange)
            .and_then(|len: u8| len.try_into())
            .expect("metadata key is a valid DE length"),
    }
}

/// A wrapper around a fixed-size tinyvec that can have its capacity further constrained to handle
/// dynamic size limits.
#[derive(Debug)]
pub(crate) struct CapacityLimitedVec<T, const N: usize>
where
    T: fmt::Debug + Clone,
    [T; N]: tinyvec::Array + fmt::Debug,
    <[T; N] as tinyvec::Array>::Item: fmt::Debug + Clone,
{
    /// constraint on the occupied space in `vec`.
    /// Invariant: `vec.len() <= constraint` and `vec.capacity() >= capacity`.
    capacity: usize,
    vec: tinyvec::ArrayVec<[T; N]>,
}

impl<T, const N: usize> CapacityLimitedVec<T, N>
where
    T: fmt::Debug + Clone,
    [T; N]: tinyvec::Array + fmt::Debug,
    <[T; N] as tinyvec::Array>::Item: fmt::Debug + Clone,
{
    pub(crate) fn len(&self) -> usize {
        self.vec.len()
    }

    fn capacity(&self) -> usize {
        self.capacity
    }

    fn truncate(&mut self, len: usize) {
        self.vec.truncate(len);
    }

    fn into_inner(self) -> tinyvec::ArrayVec<[T; N]> {
        self.vec
    }
}

impl<T, const N: usize> Sink<<[T; N] as tinyvec::Array>::Item> for CapacityLimitedVec<T, N>
where
    T: fmt::Debug + Clone,
    [T; N]: tinyvec::Array + fmt::Debug,
    <[T; N] as tinyvec::Array>::Item: fmt::Debug + Clone,
{
    fn try_extend_from_slice(&mut self, items: &[<[T; N] as tinyvec::Array>::Item]) -> Option<()> {
        if items.len() > (self.capacity() - self.len()) {
            return None;
        }
        // won't panic: just checked the length
        self.vec.extend_from_slice(items);
        Some(())
    }

    fn try_push(&mut self, item: <[T; N] as tinyvec::Array>::Item) -> Option<()> {
        if self.len() == self.capacity() {
            // already full
            None
        } else {
            self.vec.push(item);
            Some(())
        }
    }
}

impl<T, const N: usize> AsRef<[<[T; N] as tinyvec::Array>::Item]> for CapacityLimitedVec<T, N>
where
    T: fmt::Debug + Clone,
    [T; N]: tinyvec::Array + fmt::Debug,
    <[T; N] as tinyvec::Array>::Item: fmt::Debug + Clone,
{
    fn as_ref(&self) -> &[<[T; N] as tinyvec::Array>::Item] {
        self.vec.as_slice()
    }
}
impl<T, const N: usize> AsMut<[<[T; N] as tinyvec::Array>::Item]> for CapacityLimitedVec<T, N>
where
    T: fmt::Debug + Clone,
    [T; N]: tinyvec::Array + fmt::Debug,
    <[T; N] as tinyvec::Array>::Item: fmt::Debug + Clone,
{
    fn as_mut(&mut self) -> &mut [<[T; N] as tinyvec::Array>::Item] {
        self.vec.as_mut_slice()
    }
}
