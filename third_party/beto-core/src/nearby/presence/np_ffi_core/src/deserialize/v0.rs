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
//! Core NP Rust FFI structures and methods for v0 advertisement deserialization.

use crate::common::*;
use crate::credentials::credential_book::CredentialBook;
use crate::utils::{FfiEnum, LocksLongerThan};
use alloc::vec::Vec;
use handle_map::{declare_handle_map, HandleLike, HandleMapDimensions, HandleMapFullError};
use np_adv::legacy::actions::ActionsDataElement;
use np_adv::legacy::{data_elements as np_adv_de, Ciphertext, PacketFlavorEnum, Plaintext};

/// Discriminant for possible results of V0 advertisement deserialization
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum DeserializedV0AdvertisementKind {
    /// The deserialized V0 advertisement was legible.
    /// The associated payload may be obtained via
    /// `DeserializedV0Advertisement#into_legible`.
    Legible = 0,
    /// The deserialized V0 advertisement is illegible,
    /// likely meaning that the receiver does not hold
    /// the proper credentials to be able to read
    /// the received advertisement.
    NoMatchingCredentials = 1,
}

/// Represents a deserialized V0 advertisement
#[repr(C)]
#[allow(missing_docs)]
pub enum DeserializedV0Advertisement {
    Legible(LegibleDeserializedV0Advertisement),
    NoMatchingCredentials,
}

impl FfiEnum for DeserializedV0Advertisement {
    type Kind = DeserializedV0AdvertisementKind;
    fn kind(&self) -> Self::Kind {
        match self {
            DeserializedV0Advertisement::Legible(_) => DeserializedV0AdvertisementKind::Legible,
            DeserializedV0Advertisement::NoMatchingCredentials => {
                DeserializedV0AdvertisementKind::NoMatchingCredentials
            }
        }
    }
}

impl DeserializedV0Advertisement {
    /// Attempts to deallocate memory utilized internally by this V0 advertisement
    /// (which contains a handle to actual advertisement contents behind-the-scenes).
    pub fn deallocate(self) -> DeallocateResult {
        match self {
            DeserializedV0Advertisement::Legible(adv) => adv.deallocate(),
            DeserializedV0Advertisement::NoMatchingCredentials => DeallocateResult::Success,
        }
    }

    pub(crate) fn allocate_with_contents<'m, M: np_adv::credential::MatchedCredential<'m>>(
        contents: np_adv::V0AdvContents<'m, M>,
    ) -> Result<Self, HandleMapFullError> {
        match contents {
            np_adv::V0AdvContents::Plaintext(plaintext_contents) => {
                let adv = LegibleDeserializedV0Advertisement::allocate_with_plaintext_contents(
                    plaintext_contents,
                )?;
                Ok(Self::Legible(adv))
            }
            np_adv::V0AdvContents::Decrypted(_) => {
                unimplemented!();
            }
            np_adv::V0AdvContents::NoMatchingCredentials => Ok(Self::NoMatchingCredentials),
        }
    }

    declare_enum_cast! {into_legible, Legible, LegibleDeserializedV0Advertisement}
}

/// Represents a deserialized V0 advertisement whose DE contents may be read
#[repr(C)]
pub struct LegibleDeserializedV0Advertisement {
    num_des: u8,
    payload: V0Payload,
    identity: DeserializedV0Identity,
}

impl LegibleDeserializedV0Advertisement {
    pub(crate) fn allocate_with_plaintext_contents(
        contents: np_adv::legacy::deserialize::PlaintextAdvContents,
    ) -> Result<Self, HandleMapFullError> {
        let data_elements = contents.to_data_elements();
        let num_des = data_elements.len() as u8;
        let payload = V0Payload::allocate_with_data_elements(data_elements)?;
        Ok(Self { num_des, payload, identity: DeserializedV0Identity::Plaintext })
    }
    /// Gets the number of data-elements in this adv's payload
    /// Suitable as an iteration bound for `Self.into_payload().get_de(...)`.
    pub fn num_des(&self) -> u8 {
        self.num_des
    }
    /// Destructures this legible advertisement into just the payload
    pub fn into_payload(self) -> V0Payload {
        self.payload
    }
    /// Destructures this legible advertisement into just the identity information
    pub fn into_identity(self) -> DeserializedV0Identity {
        self.identity
    }
    /// Deallocates the underlying handle of the payload
    pub fn deallocate(self) -> DeallocateResult {
        self.payload.deallocate().map(|_| ()).into()
    }
}

/// Discriminant for `DeserializedV0Identity`.
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum DeserializedV0IdentityKind {
    /// The deserialized identity was a plaintext identity.
    Plaintext = 0,
    /// The deserialized identity was some decrypted identity.
    Decrypted = 1,
}

/// Represents deserialized information about the V0 identity utilized
/// by a deserialized V0 advertisement
#[repr(C)]
#[allow(missing_docs)]
pub enum DeserializedV0Identity {
    Plaintext,
    // TODO: This gets a payload once we support creds
    Decrypted,
}

impl FfiEnum for DeserializedV0Identity {
    type Kind = DeserializedV0IdentityKind;
    fn kind(&self) -> Self::Kind {
        match self {
            DeserializedV0Identity::Plaintext => DeserializedV0IdentityKind::Plaintext,
            DeserializedV0Identity::Decrypted => DeserializedV0IdentityKind::Decrypted,
        }
    }
}

/// The internal data-structure used for storing
/// the payload of a deserialized V0 advertisement.
pub struct V0PayloadInternals {
    des: Vec<V0DataElement>,
}

impl V0PayloadInternals {
    /// Attempts to get the DE with the given index
    /// in this v0 payload.
    fn get_de(&self, index: u8) -> GetV0DEResult {
        match self.des.get(index as usize) {
            Some(de) => GetV0DEResult::Success(de.clone()),
            None => GetV0DEResult::Error,
        }
    }
}

fn get_v0_payload_handle_map_dimensions() -> HandleMapDimensions {
    HandleMapDimensions {
        num_shards: global_num_shards(),
        max_active_handles: global_max_num_deserialized_v0_advertisements(),
    }
}

declare_handle_map! {v0_payload, V0Payload, super::V0PayloadInternals, super::get_v0_payload_handle_map_dimensions() }
use v0_payload::V0Payload;

impl LocksLongerThan<V0Payload> for CredentialBook {}

impl V0Payload {
    pub(crate) fn allocate_with_data_elements<F: np_adv::legacy::PacketFlavor>(
        data_elements: Vec<np_adv::legacy::deserialize::PlainDataElement<F>>,
    ) -> Result<Self, HandleMapFullError> {
        Self::allocate(move || {
            let des = data_elements.into_iter().map(V0DataElement::from).collect();
            V0PayloadInternals { des }
        })
    }
    /// Gets the data-element with the given index in this v0 adv payload
    pub fn get_de(&self, index: u8) -> GetV0DEResult {
        match self.get() {
            Ok(read_guard) => read_guard.get_de(index),
            Err(_) => GetV0DEResult::Error,
        }
    }

    /// Deallocates any underlying data held by a V0Payload
    pub fn deallocate_payload(&self) -> DeallocateResult {
        self.deallocate().map(|_| ()).into()
    }
}

/// Discriminant of `GetV0DEResult`.
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum GetV0DEResultKind {
    /// The attempt to get the DE succeeded.
    /// The associated payload may be obtained via
    /// `GetV0DEResult#into_success`.
    Success = 0,
    /// The attempt to get the DE failed,
    /// possibly due to the requested index being
    /// out-of-bounds or due to the advertisement
    /// having been previously deallocated.
    Error = 1,
}

/// The result of `V0Payload#get_de`.
#[repr(C)]
#[allow(missing_docs)]
pub enum GetV0DEResult {
    Success(V0DataElement),
    Error,
}

impl FfiEnum for GetV0DEResult {
    type Kind = GetV0DEResultKind;
    fn kind(&self) -> Self::Kind {
        match self {
            GetV0DEResult::Success(_) => GetV0DEResultKind::Success,
            GetV0DEResult::Error => GetV0DEResultKind::Error,
        }
    }
}

impl GetV0DEResult {
    declare_enum_cast! {into_success, Success, V0DataElement}
}

/// Discriminant for `V0DataElement`.
#[repr(u8)]
pub enum V0DataElementKind {
    /// A transmission Power (Tx Power) data-element.
    /// The associated payload may be obtained via
    /// `V0DataElement#into_tx_power`.
    TxPower = 0,
    /// The Actions data-element.
    /// The associated payload may be obtained via
    /// `V0DataElement#into_actions`.
    Actions = 1,
}

/// Representation of a V0 data element.
#[repr(C)]
#[allow(missing_docs)]
#[derive(Clone)]
pub enum V0DataElement {
    TxPower(TxPower),
    Actions(V0Actions),
}

impl<F: np_adv::legacy::PacketFlavor> From<np_adv::legacy::deserialize::PlainDataElement<F>>
    for V0DataElement
{
    fn from(de: np_adv::legacy::deserialize::PlainDataElement<F>) -> Self {
        use np_adv::legacy::deserialize::PlainDataElement;
        match de {
            PlainDataElement::Actions(x) => V0DataElement::Actions(x.into()),
            PlainDataElement::TxPower(x) => V0DataElement::TxPower(x.into()),
        }
    }
}

impl FfiEnum for V0DataElement {
    type Kind = V0DataElementKind;
    fn kind(&self) -> Self::Kind {
        match self {
            V0DataElement::Actions(_) => V0DataElementKind::Actions,
            V0DataElement::TxPower(_) => V0DataElementKind::TxPower,
        }
    }
}

impl V0DataElement {
    declare_enum_cast! {into_tx_power, TxPower, TxPower}
    declare_enum_cast! {into_actions, Actions, V0Actions}
}

/// Representation of a transmission power,
/// as used for the Tx Power DE in V0 and V1.
#[derive(Clone)]
#[repr(C)]
pub struct TxPower {
    tx_power: i8,
}

impl TxPower {
    /// Yields this Tx Power value as an i8.
    pub fn as_i8(&self) -> i8 {
        self.tx_power
    }
}

impl From<np_adv_de::TxPowerDataElement> for TxPower {
    fn from(de: np_adv_de::TxPowerDataElement) -> Self {
        Self { tx_power: de.tx_power_value() }
    }
}

/// Representation of the Actions DE in V0.
#[derive(Clone)]
#[repr(C)]
pub enum V0Actions {
    /// A set of action bits which were present in a plaintext identity advertisement
    Plaintext(V0ActionBits),
    /// A set of action bits which were present in a encrypted identity advertisement
    Encrypted(V0ActionBits),
}

impl<F: np_adv::legacy::PacketFlavor> From<np_adv::legacy::actions::ActionsDataElement<F>>
    for V0Actions
{
    fn from(value: ActionsDataElement<F>) -> Self {
        match F::ENUM_VARIANT {
            PacketFlavorEnum::Plaintext => {
                Self::Plaintext(V0ActionBits { bitfield: value.as_u32() })
            }
            PacketFlavorEnum::Ciphertext => {
                Self::Encrypted(V0ActionBits { bitfield: value.as_u32() })
            }
        }
    }
}

#[repr(C)]
#[derive(Clone)]
/// The bitfield data of a VOActions data element
pub struct V0ActionBits {
    bitfield: u32,
}

#[allow(missing_docs)]
#[repr(u8)]
/// The possible boolean action types which can be present in an Actions data element
pub enum BooleanActionType {
    ActiveUnlock = 8,
    NearbyShare = 9,
    InstantTethering = 10,
    PhoneHub = 11,
    PresenceManager = 12,
    Finder = 13,
    FastPairSass = 14,
}

impl From<&BooleanActionType> for np_adv::legacy::actions::ActionType {
    fn from(value: &BooleanActionType) -> Self {
        match value {
            BooleanActionType::ActiveUnlock => np_adv::legacy::actions::ActionType::ActiveUnlock,
            BooleanActionType::NearbyShare => np_adv::legacy::actions::ActionType::NearbyShare,
            BooleanActionType::InstantTethering => {
                np_adv::legacy::actions::ActionType::InstantTethering
            }
            BooleanActionType::PhoneHub => np_adv::legacy::actions::ActionType::PhoneHub,
            BooleanActionType::Finder => np_adv::legacy::actions::ActionType::Finder,
            BooleanActionType::FastPairSass => np_adv::legacy::actions::ActionType::FastPairSass,
            BooleanActionType::PresenceManager => {
                np_adv::legacy::actions::ActionType::PresenceManager
            }
        }
    }
}

/// Error returned if action bits inside of a V0Actions struct are invalid. If the struct was
/// created by the np_adv deserializer, the bits will always be valid, they are only invalid if a
/// user reaches in and changes them to something invalid.
#[derive(Debug)]
pub struct InvalidActionBits;

impl V0Actions {
    /// Gets the V0 Action bits as represented by a u32 where the last 8 bits are
    /// always 0 since V0 actions can only hold up to 24 bits.
    pub fn as_u32(&self) -> u32 {
        match self {
            V0Actions::Plaintext(bits) => bits.bitfield,
            V0Actions::Encrypted(bits) => bits.bitfield,
        }
    }

    /// Return whether a boolean action type is set in this data element
    #[allow(clippy::expect_used)]
    pub fn has_action(&self, action_type: &BooleanActionType) -> Result<bool, InvalidActionBits> {
        match self {
            V0Actions::Plaintext(action_bits) => {
                let bits = np_adv::legacy::actions::ActionBits::<Plaintext>::try_from(
                    action_bits.bitfield,
                )
                .map_err(|_| InvalidActionBits)?;

                let actions_de = np_adv::legacy::actions::ActionsDataElement::from(bits);
                Ok(actions_de
                    .has_action(&action_type.into())
                    .expect("BooleanActionType only has one bit"))
            }
            V0Actions::Encrypted(action_bits) => {
                let bits = np_adv::legacy::actions::ActionBits::<Ciphertext>::try_from(
                    action_bits.bitfield,
                )
                .map_err(|_| InvalidActionBits)?;

                let actions_de = np_adv::legacy::actions::ActionsDataElement::from(bits);
                Ok(actions_de
                    .has_action(&action_type.into())
                    .expect("BooleanActionType only has one bit"))
            }
        }
    }

    /// Return the context sequence number from this data element
    #[allow(clippy::expect_used)]
    pub fn get_context_sync_seq_num(&self) -> Result<u8, InvalidActionBits> {
        match self {
            V0Actions::Plaintext(action_bits) => {
                let bits = np_adv::legacy::actions::ActionBits::<Plaintext>::try_from(
                    action_bits.bitfield,
                )
                .map_err(|_| InvalidActionBits)?;

                let actions_de = np_adv::legacy::actions::ActionsDataElement::from(bits);
                Ok(actions_de.context_sync_seq_num().as_u8())
            }
            V0Actions::Encrypted(action_bits) => {
                let bits = np_adv::legacy::actions::ActionBits::<Ciphertext>::try_from(
                    action_bits.bitfield,
                )
                .map_err(|_| InvalidActionBits)?;
                let actions_de = np_adv::legacy::actions::ActionsDataElement::from(bits);
                Ok(actions_de.context_sync_seq_num().as_u8())
            }
        }
    }
}
