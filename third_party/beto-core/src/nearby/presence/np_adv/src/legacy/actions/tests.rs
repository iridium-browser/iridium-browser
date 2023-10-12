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

extern crate std;

use crate::legacy::{
    actions::*, serialize::ToDataElementBundle, Ciphertext, PacketFlavorEnum, Plaintext,
};
use std::collections;

#[test]
fn set_context_sync_works() {
    let mut actions = ActionBits::<Plaintext>::default();

    actions.set_action(ContextSyncSeqNum::try_from(15).unwrap());
    assert_eq_hex(0xF0000000, actions.bits);

    assert_eq!(actions.bits_for_type(&ActionType::ContextSyncSeqNum), 15);
}

#[test]
fn set_context_sync_doesnt_clobber_neighboring_bit() {
    let mut actions = ActionBits::<Plaintext>::default();

    // set bit just below
    actions.bits |= 0x8000000;

    actions.set_action(ContextSyncSeqNum::try_from(15).unwrap());
    assert_eq_hex(0xF8000000, actions.bits);

    assert_eq!(actions.bits_for_type(&ActionType::ContextSyncSeqNum), 15);
}

#[test]
fn unset_context_sync_works() {
    let mut actions = ActionBits::<Plaintext> {
        // all 1s
        bits: u32::MAX,
        ..Default::default()
    };

    actions.set_action(ContextSyncSeqNum::try_from(0).unwrap());
    assert_eq_hex(0x0FFFFFFF, actions.bits);

    assert_eq!(actions.bits_for_type(&ActionType::ContextSyncSeqNum), 0);
}

#[test]
fn set_ns_works() {
    let mut actions = ActionBits::<Plaintext>::default();

    actions.set_action(NearbyShare::from(true));
    assert_eq_hex(0x00400000, actions.bits);

    assert_eq!(actions.bits_for_type(&ActionType::NearbyShare), 1);
    assert_eq!(actions.bits_for_type(&ActionType::ActiveUnlock), 0);
    assert_eq!(actions.bits_for_type(&ActionType::InstantTethering), 0);
}

#[test]
fn set_ns_doesnt_clobber_others() {
    let mut actions = ActionBits::<Plaintext>::default();

    // set neighboring bits
    actions.bits |= 0x00120000;

    actions.set_action(NearbyShare::from(true));
    assert_eq_hex(0x00520000, actions.bits);

    assert_eq!(actions.bits_for_type(&ActionType::NearbyShare), 1);
    assert_eq!(actions.bits_for_type(&ActionType::PhoneHub), 1);
    assert_eq!(actions.bits_for_type(&ActionType::FastPairSass), 1);
}

#[test]
fn unset_ns_works() {
    let mut actions = ActionBits::<Plaintext> {
        // all 1s
        bits: u32::MAX,
        ..Default::default()
    };

    actions.set_action(NearbyShare::from(false));
    assert_eq_hex(0xFFBFFFFF, actions.bits);
}

#[test]
fn set_last_bit_works() {
    let mut actions = ActionBits::<Plaintext>::default();

    actions.set_action(LastBit::from(true));
    assert_eq_hex(0x0100, actions.bits);
}

#[test]
fn set_last_bit_doesnt_clobber_others() {
    let mut actions = ActionBits::<Plaintext>::default();

    // set neighboring bits
    actions.bits |= 0x200;

    actions.set_action(LastBit::from(true));
    assert_eq_hex(0x300, actions.bits);
}

#[test]
fn unset_last_bit_works() {
    let mut actions = ActionBits::<Plaintext> {
        // all 1s
        bits: u32::MAX,
        ..Default::default()
    };

    actions.set_action(LastBit::from(false));
    assert_eq_hex(0xFFFFFEFF, actions.bits);
}

#[test]
fn bytes_used_works() {
    let mut actions = ActionBits::<Plaintext>::default();

    // Special-case: All-zeroes should lead to a single byte being used.
    assert_eq!(1, actions.bytes_used());

    actions.set_action(ContextSyncSeqNum::try_from(3).unwrap());
    assert_eq!(1, actions.bytes_used());

    actions.set_action(NearbyShare::from(true));
    assert_eq!(2, actions.bytes_used());

    actions.set_action(LastBit::from(true));
    assert_eq!(3, actions.bytes_used());

    actions.set_action(LastBit::from(false));
    assert_eq!(2, actions.bytes_used());
}

#[test]
fn write_de_empty_actions() {
    // The special case of no action bits set should still occupy one byte [of all zeroes].
    let de = ActionsDataElement::<Plaintext>::from(ActionBits::default()).to_de_bundle();

    assert_eq!(&[0x00], de.contents_as_slice());
}

#[test]
fn write_de_one_action_byte() {
    let mut action = ActionBits::default();
    action.set_action(ContextSyncSeqNum::try_from(7).unwrap());
    let de = ActionsDataElement::<Plaintext>::from(action).to_de_bundle();

    assert_eq!(&[0x70], de.contents_as_slice());
}

#[test]
fn write_de_three_action_bytes() {
    let mut action = ActionBits::default();
    action.set_action(LastBit::from(true));
    let de = ActionsDataElement::<Plaintext>::from(action).to_de_bundle();

    assert_eq!(&[0, 0, 1], de.contents_as_slice());
}

#[test]
fn write_de_all_plaintext_actions() {
    let mut action = all_plaintext_actions();
    action.set_action(LastBit::from(true));
    let de = ActionsDataElement::<Plaintext>::from(action).to_de_bundle();

    // byte 0: context sync
    // byte 1: nearby share, finder, fp sass
    // byte 2: last bit
    assert_eq!(&[0x90, 0x46, 0x01], de.contents_as_slice());
}

#[test]
fn write_de_all_encrypted_actions() {
    let mut action = all_ciphertext_actions();
    action.set_action(LastBit::from(true));
    let de = ActionsDataElement::<Ciphertext>::from(action).to_de_bundle();

    // byte 1: context sync num = 9, 4 unused bits
    // byte 2: active unlock, nearby share, instant tethering, phone hub,
    // presence manager, last 3 bits unused
    // byte 3: last bit
    assert_eq!(&[0x90, 0xF8, 0x01], de.contents_as_slice());
}

#[test]
fn action_element_nonzero_len() {
    for t in ActionType::iter() {
        assert!(t.bits_len() > 0);
    }
}

#[test]
fn action_element_bits_dont_overlap() {
    let type_to_bits =
        ActionType::iter().map(|t| (t, t.all_bits())).collect::<collections::HashMap<_, _>>();

    for t in ActionType::iter() {
        let bits = type_to_bits.get(&t).unwrap();

        for (_, other_bits) in type_to_bits.iter().filter(|(other_type, _)| t != **other_type) {
            assert_eq!(0, bits & other_bits, "type {t:?}");
        }
    }
}

#[test]
fn action_type_all_bits() {
    assert_eq!(0xF0000000, ActionType::ContextSyncSeqNum.all_bits());
    assert_eq!(0x00800000, ActionType::ActiveUnlock.all_bits());
    assert_eq!(0x00020000, ActionType::FastPairSass.all_bits());
}

#[test]
fn action_type_all_bits_in_per_type_masks() {
    for t in ActionType::iter().filter(|t| t.supports_flavor(PacketFlavorEnum::Plaintext)) {
        assert_eq!(t.all_bits(), t.all_bits() & *ALL_PLAINTEXT_ELEMENT_BITS);
    }
    for t in ActionType::iter().filter(|t| t.supports_flavor(PacketFlavorEnum::Ciphertext)) {
        assert_eq!(t.all_bits(), t.all_bits() & *ALL_CIPHERTEXT_ELEMENT_BITS);
    }
}

#[test]
fn action_bits_try_from_flavor_mismatch_plaintext() {
    assert_eq!(
        FlavorNotSupported { flavor: PacketFlavorEnum::Plaintext },
        ActionBits::<Plaintext>::try_from(ActionType::PresenceManager.all_bits()).unwrap_err()
    );
    assert_eq!(
        0xF0000000,
        ActionBits::<Plaintext>::try_from(ActionType::ContextSyncSeqNum.all_bits()).unwrap().bits
    );
}

#[test]
fn action_bits_try_from_flavor_mismatch_ciphertext() {
    assert_eq!(
        FlavorNotSupported { flavor: PacketFlavorEnum::Ciphertext },
        ActionBits::<Ciphertext>::try_from(ActionType::Finder.all_bits()).unwrap_err()
    );
    assert_eq!(
        0xF0000000,
        ActionBits::<Ciphertext>::try_from(ActionType::ContextSyncSeqNum.all_bits()).unwrap().bits
    );
}

#[test]
fn actions_de_deser_plaintext_with_ciphertext_action() {
    assert_eq!(
        DataElementDeserializeError::FlavorNotSupported {
            de_type: DataElementType::Actions,
            flavor: PacketFlavorEnum::Plaintext
        },
        <ActionsDataElement<Plaintext> as DataElement>::deserialize::<Plaintext>(&[
            // active unlock bit set
            0x00, 0x80, 0x00,
        ])
        .unwrap_err()
    );
}

#[test]
fn actions_de_deser_ciphertext_with_plaintext_action() {
    assert_eq!(
        DataElementDeserializeError::FlavorNotSupported {
            de_type: DataElementType::Actions,
            flavor: PacketFlavorEnum::Ciphertext
        },
        <ActionsDataElement<Ciphertext> as DataElement>::deserialize::<Ciphertext>(&[
            // Finder bit set
            0x00, 0x00, 0x80,
        ])
        .unwrap_err()
    );
}

#[test]
fn context_sync_seq_num_works() {
    let mut action_bits = ActionBits::<Plaintext>::default();
    action_bits.set_action(ContextSyncSeqNum::try_from(15).unwrap());
    let action_de = ActionsDataElement::from(action_bits);
    assert_eq!(15, action_de.context_sync_seq_num().as_u8());
}

#[test]
fn context_sync_seq_num_default_zero() {
    let action_bits = ActionBits::<Plaintext>::default();
    let action_de = ActionsDataElement::from(action_bits);
    assert_eq!(0, action_de.context_sync_seq_num().as_u8());
}

#[test]
fn has_action_plaintext_works() {
    let mut action_bits = ActionBits::<Plaintext>::default();
    action_bits.set_action(ContextSyncSeqNum::try_from(15).unwrap());
    action_bits.set_action(NearbyShare::from(true));
    let action_de = ActionsDataElement::from(action_bits);
    assert_eq!(action_de.has_action(&ActionType::NearbyShare), Some(true));
    assert_eq!(action_de.has_action(&ActionType::ActiveUnlock), Some(false));
    assert_eq!(action_de.has_action(&ActionType::PhoneHub), Some(false));
}

#[test]
fn has_action_encrypted_works() {
    let mut action_bits = ActionBits::<Ciphertext>::default();
    action_bits.set_action(ContextSyncSeqNum::try_from(15).unwrap());
    action_bits.set_action(NearbyShare::from(true));
    action_bits.set_action(ActiveUnlock::from(true));
    let action_de = ActionsDataElement::from(action_bits);
    assert_eq!(action_de.has_action(&ActionType::NearbyShare), Some(true));
    assert_eq!(action_de.has_action(&ActionType::ActiveUnlock), Some(true));
    assert_eq!(action_de.has_action(&ActionType::PhoneHub), Some(false));
    assert_eq!(action_de.has_action(&ActionType::ContextSyncSeqNum), None);
}

// hypothetical action using the last bit
#[derive(Debug)]
struct LastBit {
    enabled: bool,
}
impl From<bool> for LastBit {
    fn from(value: bool) -> Self {
        LastBit { enabled: value }
    }
}
impl ActionElement for LastBit {
    const HIGH_BIT_INDEX: u32 = 23;
    const BITS_LEN: u32 = 1;

    // don't want to add a variant for this test only type
    const ACTION_TYPE: ActionType = ActionType::ActiveUnlock;

    fn supports_flavor(_flavor: PacketFlavorEnum) -> bool {
        true
    }
}

macros::boolean_element_to_plaintext_element!(LastBit);
macros::boolean_element_to_encrypted_element!(LastBit);

fn assert_eq_hex(expected: u32, actual: u32) {
    assert_eq!(expected, actual, "{expected:#010X} != {actual:#010X}");
}

pub(crate) fn all_plaintext_actions() -> ActionBits<Plaintext> {
    let mut action = ActionBits::default();
    action.set_action(ContextSyncSeqNum::try_from(9).unwrap());
    action.set_action(NearbyShare::from(true));
    action.set_action(Finder::from(true));
    action.set_action(FastPairSass::from(true));
    action
}

pub(crate) fn all_ciphertext_actions() -> ActionBits<Ciphertext> {
    let mut action = ActionBits::default();
    action.set_action(ContextSyncSeqNum::try_from(9).unwrap());
    action.set_action(ActiveUnlock::from(true));
    action.set_action(NearbyShare::from(true));
    action.set_action(InstantTethering::from(true));
    action.set_action(PhoneHub::from(true));
    action.set_action(PresenceManager::from(true));
    action
}
