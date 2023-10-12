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

//! The "Actions" data element and associated types.
//!
//! This DE is somewhat more complex than other DEs. Whether or not it supports a particular flavor
//! depends on the actions set, so it has to be treated as two separate types based on which
//! flavor type parameter is used.
use crate::{
    legacy::{
        data_elements::{DataElement, DataElementDeserializeError},
        de_type::{DataElementType, PlainDataElementType},
        serialize::{DataElementBundle, ToDataElementBundle},
        Ciphertext, PacketFlavor, PacketFlavorEnum, Plaintext,
    },
    shared_data::ContextSyncSeqNum,
};
#[cfg(feature = "devtools")]
use core::ops::Range;
use core::{marker, num, ops};
use nom::{bytes, combinator, error};
use strum::IntoEnumIterator as _;

mod macros;
#[cfg(test)]
pub(crate) mod tests;

/// Actions DE.
/// Only as many DE payload bytes will be present as needed to represent all set bits that are encoded,
/// with a lower bound of 1 byte in the special case of no set action bits, and an upper bound
/// of 3 bytes occupied by the DE payload.
#[derive(Debug, PartialEq, Eq)]
pub struct ActionsDataElement<F: PacketFlavor> {
    action: ActionBits<F>,
}

impl<F: PacketFlavor> ActionsDataElement<F> {
    /// Returns the actions bits as a u32. The upper limit of an actions field is 3 bytes,
    /// so the last bytes of this u32 will always be 0
    pub fn as_u32(self) -> u32 {
        self.action.bits
    }

    /// Return whether a boolean action type is set in this data element, or `None` if the given
    /// action type does not represent a boolean.
    pub fn has_action(&self, action_type: &ActionType) -> Option<bool> {
        (action_type.bits_len() == 1).then_some(self.action.bits_for_type(action_type) != 0)
    }

    /// Return the context sync sequence number.
    pub fn context_sync_seq_num(&self) -> ContextSyncSeqNum {
        ContextSyncSeqNum::try_from(self.action.bits_for_type(&ActionType::ContextSyncSeqNum) as u8)
            .expect("Masking with ActionType::ContextSyncSeqNum should always be in range")
    }
}

pub(crate) const ACTIONS_MAX_LEN: usize = 3;

impl<F> ActionsDataElement<F>
where
    F: PacketFlavor,
    ActionsDataElement<F>: DataElement,
{
    pub(crate) const ACTIONS_LEN: ops::RangeInclusive<usize> = (1..=ACTIONS_MAX_LEN);

    /// Generic deserialize, not meant to be called directly -- use [DataElement] impls instead.
    fn deserialize(de_contents: &[u8]) -> Result<Self, DataElementDeserializeError> {
        combinator::all_consuming::<&[u8], _, error::Error<&[u8]>, _>(combinator::map(
            bytes::complete::take_while_m_n(0, ACTIONS_MAX_LEN, |_| true),
            |bytes: &[u8]| {
                let mut action_bytes = [0_u8; 4];
                action_bytes[..bytes.len()].copy_from_slice(bytes);
                u32::from_be_bytes(action_bytes)
            },
        ))(de_contents)
        .map_err(|_| DataElementDeserializeError::DeserializeError {
            de_type: Self::DE_TYPE_VARIANT,
        })
        .map(|(_remaining, actions)| actions)
        .and_then(|action_bits_num| {
            let action = ActionBits::try_from(action_bits_num).map_err(|e| {
                DataElementDeserializeError::FlavorNotSupported {
                    de_type: Self::DE_TYPE_VARIANT,
                    flavor: e.flavor,
                }
            })?;
            Ok(Self { action })
        })
    }
}

impl<F: PacketFlavor> From<ActionBits<F>> for ActionsDataElement<F> {
    fn from(action: ActionBits<F>) -> Self {
        Self { action }
    }
}

impl<F: PacketFlavor> ToDataElementBundle<F> for ActionsDataElement<F> {
    fn to_de_bundle(&self) -> DataElementBundle<F> {
        let action_byte_len = self.action.bytes_used();
        let slice = &self.action.bits.to_be_bytes()[..action_byte_len];

        DataElementBundle::try_from(PlainDataElementType::Actions, slice)
            .expect("Length < max DE size")
    }
}

impl DataElement for ActionsDataElement<Plaintext> {
    const DE_TYPE_VARIANT: DataElementType = DataElementType::Actions;

    fn supports_flavor(flavor: PacketFlavorEnum) -> bool {
        match flavor {
            PacketFlavorEnum::Plaintext => true,
            PacketFlavorEnum::Ciphertext => false,
        }
    }

    fn deserialize<F: PacketFlavor>(
        de_contents: &[u8],
    ) -> Result<Self, DataElementDeserializeError> {
        match F::ENUM_VARIANT {
            PacketFlavorEnum::Plaintext => ActionsDataElement::deserialize(de_contents),
            PacketFlavorEnum::Ciphertext => Err(DataElementDeserializeError::FlavorNotSupported {
                de_type: DataElementType::Actions,
                flavor: F::ENUM_VARIANT,
            }),
        }
    }
}

impl DataElement for ActionsDataElement<Ciphertext> {
    const DE_TYPE_VARIANT: DataElementType = DataElementType::Actions;

    fn supports_flavor(flavor: PacketFlavorEnum) -> bool {
        match flavor {
            PacketFlavorEnum::Plaintext => false,
            PacketFlavorEnum::Ciphertext => true,
        }
    }

    fn deserialize<F: PacketFlavor>(
        de_contents: &[u8],
    ) -> Result<Self, DataElementDeserializeError> {
        match F::ENUM_VARIANT {
            PacketFlavorEnum::Plaintext => Err(DataElementDeserializeError::FlavorNotSupported {
                de_type: DataElementType::Actions,
                flavor: F::ENUM_VARIANT,
            }),
            PacketFlavorEnum::Ciphertext => ActionsDataElement::deserialize(de_contents),
        }
    }
}

/// Container for the 24 bits defined for "actions" (feature flags and the like).
/// This internally stores a u32, but only the 24 highest bits of this
/// field will actually ever be populated.
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub struct ActionBits<F: PacketFlavor> {
    bits: u32,
    // marker for element type
    flavor: marker::PhantomData<F>,
}

impl<F: PacketFlavor> Default for ActionBits<F> {
    fn default() -> Self {
        ActionBits {
            bits: 0, // no bits set
            flavor: marker::PhantomData,
        }
    }
}

/// At least one action doesn't support the required flavor
#[derive(PartialEq, Eq, Debug)]
pub struct FlavorNotSupported {
    flavor: PacketFlavorEnum,
}

lazy_static::lazy_static! {
    /// All bits for plaintext action types: 1 where a plaintext action could have a bit, 0 elsewhere.
    static ref ALL_PLAINTEXT_ELEMENT_BITS: u32 = ActionType::iter()
        .filter(|t| t.supports_flavor(PacketFlavorEnum::Plaintext))
        .map(|t| t.all_bits())
        .fold(0_u32, |accum, bits| accum | bits);
}

lazy_static::lazy_static! {
    /// All bits for ciphertext action types: 1 where a ciphertext action could have a bit, 0 elsewhere.
    static ref ALL_CIPHERTEXT_ELEMENT_BITS: u32 = ActionType::iter()
        .filter(|t| t.supports_flavor(PacketFlavorEnum::Ciphertext))
        .map(|t| t.all_bits())
        .fold(0_u32, |accum, bits| accum | bits);
}

impl<F: PacketFlavor> ActionBits<F> {
    /// Tries to create ActionBits from a u32, returning error in the event a specific bit is set for
    /// an unsupported flavor
    pub fn try_from(value: u32) -> Result<Self, FlavorNotSupported> {
        let ok_bits: u32 = match F::ENUM_VARIANT {
            PacketFlavorEnum::Plaintext => *ALL_PLAINTEXT_ELEMENT_BITS,
            PacketFlavorEnum::Ciphertext => *ALL_CIPHERTEXT_ELEMENT_BITS,
        };

        // no bits set beyond what's allowed for this flavor
        if value | ok_bits == ok_bits {
            Ok(Self { bits: value, flavor: marker::PhantomData })
        } else {
            Err(FlavorNotSupported { flavor: F::ENUM_VARIANT })
        }
    }

    /// Set the bits for the provided element.
    /// Bits outside the range set by the action will be unaffected.
    pub fn set_action<E: ToActionElement<F>>(&mut self, to_element: E) {
        let element = to_element.to_action_element();
        let len = element.len.get();

        // validate that the element is not horribly broken
        debug_assert!(len + element.index <= 32);
        // must not have bits set past the high `len` bits
        debug_assert_eq!(0, element.bits >> (8 - len));

        // 0-extend to u32
        let byte_extended = element.bits as u32;
        // Shift so that the high bit is at the desired index.
        // Won't overflow since length > 0.
        let bits_in_position = byte_extended << (32 - len - element.index);

        // We want to effectively clear out the bits already in place, so we don't want to just |=.
        // Instead, we construct a u32 with all 1s above and below the relevant bits and &=, so that
        // if the new bits are 0, the stored bits will be cleared.

        // avoid overflow when index = 0 -- need zero 1 bits to the left in that case
        let left_1s = u32::MAX.checked_shl(32 - element.index).unwrap_or(0);
        // avoid underflow when index + len = 32 -- zero 1 bits to the right
        let right_1s = u32::MAX.checked_shr(element.index + len).unwrap_or(0);
        let mask = left_1s | right_1s;
        let bits_for_other_actions = self.bits & mask;
        self.bits = bits_for_other_actions | bits_in_position;
    }

    /// How many bytes (1-3) are needed to represent the set bits, starting from the most
    /// significant bit. The lower bound of 1 is because the unique special case of
    /// an actions field of all zeroes is required by the spec to occupy exactly one byte.
    pub(crate) fn bytes_used(&self) -> usize {
        let bits_used = 32 - self.bits.trailing_zeros();
        let raw_count = (bits_used as usize + 7) / 8;
        if raw_count == 0 {
            1 // Uncommon case - should only be hit for all-zero action bits
        } else {
            raw_count
        }
    }

    /// Return the bits for a given action type as the low bits in the returned u32.
    ///
    /// For example, when extracting the bits `B` from `0bXXXXXXXXXXBBBBBBXXXXXXXXXXXXXXXX`, the
    /// return value will be `0b00000000000000000000000000BBBBBB`.
    pub fn bits_for_type(&self, action_type: &ActionType) -> u32 {
        self.bits << action_type.high_bit_index() >> (32 - action_type.bits_len())
    }
}

/// The encoded form of an individual action element.
#[derive(Debug, Clone, Copy)]
pub struct ActionElementBits<F: PacketFlavor> {
    /// Offset from the high bit in `ActionBits.bits`, which would be bit 0 of byte 0 of the big-endian
    /// representation.
    /// Must leave enough room for `len` bits in a u32; that is, `index + len <= 32`.
    index: u32,
    /// Number of bits used.
    /// `len + index <= 32` must be true.
    len: num::NonZeroU32,
    /// Returns the bits to set as the lower `len` bits of the byte.
    bits: u8,
    /// Marker for whether it can be used in plaintext or encrypted data elements.
    flavor: marker::PhantomData<F>,
}

/// Core trait for an individual action
pub trait ActionElement {
    /// The offset from the high bit in the eventual bit sequence of all actions.
    /// See [ActionElementBits.index].
    ///
    /// Each implementation must have a non-overlapping sequence of bits defined by
    /// `HIGH_BIT_INDEX` and `BITS_LEN` w.r.t every other implementation.
    const HIGH_BIT_INDEX: u32;
    /// The number of high bits in a `u8` that should be used when assembling the complete
    /// action bit vector.
    ///
    /// Must be >0.
    const BITS_LEN: u32;
    /// Forces implementations to have a matching enum variant so the enum can be kept up to date.
    const ACTION_TYPE: ActionType;

    /// Returns whether or not this action supports the provided `flavor`.
    ///
    /// Must match the implementations of [ToActionElement].
    fn supports_flavor(flavor: PacketFlavorEnum) -> bool;

    /// Returns true if the bits for this element are all zero, or if the flavor is supported.
    fn action_is_supported_or_not_set(bits: u32, flavor: PacketFlavorEnum) -> bool {
        let shifted = bits << Self::HIGH_BIT_INDEX;
        let masked = shifted & (!(u32::MAX >> Self::BITS_LEN));

        (masked == 0) || Self::supports_flavor(flavor)
    }
}

/// An analog of `Into` tailored to converting structs modeling specific action elements into the
/// representation needed by [ActionBits].
pub trait ToActionElement<F: PacketFlavor>: ActionElement {
    /// Convert the high-level representation of an element into the literal bits needed.
    fn to_action_element(&self) -> ActionElementBits<F> {
        ActionElementBits {
            index: Self::HIGH_BIT_INDEX,
            len: Self::BITS_LEN.try_into().expect("all elements must have nonzero len"),
            bits: self.bits(),
            flavor: marker::PhantomData,
        }
    }

    /// Returns the bits that should be set starting at `Self::INDEX`.
    ///
    /// Must not have more than the low `len()` bits set.
    fn bits(&self) -> u8;
}

/// Provides a way to iterate over all action types.
#[derive(Clone, Copy, strum_macros::EnumIter, PartialEq, Eq, Hash, Debug)]
#[allow(missing_docs)]
pub enum ActionType {
    ContextSyncSeqNum,
    ActiveUnlock,
    NearbyShare,
    InstantTethering,
    PhoneHub,
    Finder,
    FastPairSass,
    PresenceManager,
}

impl ActionType {
    /// A u32 with all possible bits for this action type set
    const fn all_bits(&self) -> u32 {
        (u32::MAX << (32_u32 - self.bits_len())) >> self.high_bit_index()
    }

    /// Get the range of the bits occupied used by this bit index. For example, if the action type
    /// uses the 5th and 6th bits, the returned range will be (5..7).
    /// (0 is the index of the most significant bit).
    #[cfg(feature = "devtools")]
    pub const fn bits_range_for_devtools(&self) -> Range<u32> {
        let high_bit_index = self.high_bit_index();
        high_bit_index..high_bit_index + self.bits_len()
    }

    const fn high_bit_index(&self) -> u32 {
        match self {
            ActionType::ContextSyncSeqNum => ContextSyncSeqNum::HIGH_BIT_INDEX,
            ActionType::ActiveUnlock => ActiveUnlock::HIGH_BIT_INDEX,
            ActionType::NearbyShare => NearbyShare::HIGH_BIT_INDEX,
            ActionType::InstantTethering => InstantTethering::HIGH_BIT_INDEX,
            ActionType::PhoneHub => PhoneHub::HIGH_BIT_INDEX,
            ActionType::Finder => Finder::HIGH_BIT_INDEX,
            ActionType::FastPairSass => FastPairSass::HIGH_BIT_INDEX,
            ActionType::PresenceManager => PresenceManager::HIGH_BIT_INDEX,
        }
    }

    const fn bits_len(&self) -> u32 {
        match self {
            ActionType::ContextSyncSeqNum => ContextSyncSeqNum::BITS_LEN,
            ActionType::ActiveUnlock => ActiveUnlock::BITS_LEN,
            ActionType::NearbyShare => NearbyShare::BITS_LEN,
            ActionType::InstantTethering => InstantTethering::BITS_LEN,
            ActionType::PhoneHub => PhoneHub::BITS_LEN,
            ActionType::Finder => Finder::BITS_LEN,
            ActionType::FastPairSass => FastPairSass::BITS_LEN,
            ActionType::PresenceManager => PresenceManager::BITS_LEN,
        }
    }

    pub(crate) fn supports_flavor(&self, flavor: PacketFlavorEnum) -> bool {
        match self {
            ActionType::ContextSyncSeqNum => ContextSyncSeqNum::supports_flavor(flavor),
            ActionType::ActiveUnlock => ActiveUnlock::supports_flavor(flavor),
            ActionType::NearbyShare => NearbyShare::supports_flavor(flavor),
            ActionType::InstantTethering => InstantTethering::supports_flavor(flavor),
            ActionType::PhoneHub => PhoneHub::supports_flavor(flavor),
            ActionType::Finder => Finder::supports_flavor(flavor),
            ActionType::FastPairSass => FastPairSass::supports_flavor(flavor),
            ActionType::PresenceManager => PresenceManager::supports_flavor(flavor),
        }
    }
}

impl ActionElement for ContextSyncSeqNum {
    const HIGH_BIT_INDEX: u32 = 0;
    const BITS_LEN: u32 = 4;
    const ACTION_TYPE: ActionType = ActionType::ContextSyncSeqNum;

    fn supports_flavor(flavor: PacketFlavorEnum) -> bool {
        match flavor {
            PacketFlavorEnum::Plaintext => true,
            PacketFlavorEnum::Ciphertext => true,
        }
    }
}

impl ToActionElement<Plaintext> for ContextSyncSeqNum {
    fn bits(&self) -> u8 {
        self.as_u8()
    }
}

impl ToActionElement<Ciphertext> for ContextSyncSeqNum {
    fn bits(&self) -> u8 {
        self.as_u8()
    }
}

// enabling an element for public adv requires privacy approval due to fingerprinting risk

macros::boolean_element!(ActiveUnlock, 8, ciphertext_only);
macros::boolean_element!(NearbyShare, 9, plaintext_and_ciphertext);
macros::boolean_element!(InstantTethering, 10, ciphertext_only);
macros::boolean_element!(PhoneHub, 11, ciphertext_only);
macros::boolean_element!(PresenceManager, 12, ciphertext_only);
macros::boolean_element!(Finder, 13, plaintext_only);
macros::boolean_element!(FastPairSass, 14, plaintext_only);
