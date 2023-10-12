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

use crate::{
    legacy::{
        actions::*,
        data_elements::*,
        de_type::PlainDataElementType,
        deserialize::PlainDataElement,
        serialize::{DataElementBundle, ToDataElementBundle},
        Ciphertext, PacketFlavor, PacketFlavorEnum, Plaintext,
    },
    shared_data::{ContextSyncSeqNum, TxPower},
};
use rand_ext::rand::{self, distributions, prelude::SliceRandom as _};
use std::prelude::rust_2021::*;
use strum::IntoEnumIterator;

impl distributions::Distribution<ActionsDataElement<Plaintext>> for distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> ActionsDataElement<Plaintext> {
        let mut available_actions = ActionType::iter()
            .filter(|at| at.supports_flavor(PacketFlavorEnum::Plaintext))
            .collect::<Vec<_>>();
        available_actions.shuffle(rng);

        // choose some of the available actions.
        let selected_actions: &[ActionType] =
            &available_actions[0..rng.gen_range(0..available_actions.len())];

        let mut bits = ActionBits::default();

        for a in selected_actions {
            match a {
                ActionType::ContextSyncSeqNum => {
                    bits.set_action(ContextSyncSeqNum::try_from(rng.gen_range(0..=15)).unwrap())
                }
                // generating boolean actions with `true` since we already did our random selection
                // of which actions to use above
                ActionType::NearbyShare => bits.set_action(NearbyShare::from(true)),
                ActionType::Finder => bits.set_action(Finder::from(true)),
                ActionType::FastPairSass => bits.set_action(FastPairSass::from(true)),
                ActionType::ActiveUnlock
                | ActionType::PresenceManager
                | ActionType::InstantTethering
                | ActionType::PhoneHub => unreachable!("not plaintext actions"),
            }
        }

        ActionsDataElement::from(bits)
    }
}
impl distributions::Distribution<ActionsDataElement<Ciphertext>> for distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> ActionsDataElement<Ciphertext> {
        let mut available_actions = ActionType::iter()
            .filter(|at| at.supports_flavor(PacketFlavorEnum::Ciphertext))
            .collect::<Vec<_>>();
        available_actions.shuffle(rng);

        // choose some of the available actions
        let selected_actions: &[ActionType] =
            &available_actions[0..rng.gen_range(0..available_actions.len())];

        let mut bits = ActionBits::default();

        for a in selected_actions {
            match a {
                ActionType::ContextSyncSeqNum => {
                    bits.set_action(ContextSyncSeqNum::try_from(rng.gen_range(0..=15)).unwrap())
                }
                ActionType::ActiveUnlock => bits.set_action(ActiveUnlock::from(true)),
                // generating boolean actions with `true` since we already did our random selection
                // of which actions to use above
                ActionType::NearbyShare => bits.set_action(NearbyShare::from(true)),
                ActionType::PresenceManager => bits.set_action(PresenceManager::from(true)),
                ActionType::InstantTethering => bits.set_action(InstantTethering::from(true)),
                ActionType::PhoneHub => bits.set_action(PhoneHub::from(true)),
                ActionType::Finder | ActionType::FastPairSass => {
                    unreachable!("not ciphertext actions")
                }
            }
        }

        ActionsDataElement::from(bits)
    }
}

impl distributions::Distribution<TxPower> for distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> TxPower {
        let tx_power: i8 = rng.gen_range(-100..=20);
        TxPower::try_from(tx_power).unwrap()
    }
}

impl distributions::Distribution<TxPowerDataElement> for distributions::Standard {
    fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> TxPowerDataElement {
        let tx_power: TxPower = self.sample(rng);
        TxPowerDataElement::from(tx_power)
    }
}

/// Generate a random instance of the requested DE and return it wrapped in [PlainDataElement] along
/// with its bundle representation.
pub(crate) fn rand_de_and_bundle<F, D, E, R>(
    to_enum: E,
    rng: &mut R,
) -> (PlainDataElement<F>, DataElementBundle<F>)
where
    F: PacketFlavor,
    D: ToDataElementBundle<F>,
    E: Fn(D) -> PlainDataElement<F>,
    R: rand::Rng,
    distributions::Standard: distributions::Distribution<D>,
{
    let de = rng.gen::<D>();
    let bundle = de.to_de_bundle();
    (to_enum(de), bundle)
}

/// Generate a random instance of the requested de type, or `None` if that type does not support
/// plaintext.
pub(crate) fn random_de_plaintext<R>(
    de_type: PlainDataElementType,
    rng: &mut R,
) -> Option<(PlainDataElement<Plaintext>, DataElementBundle<Plaintext>)>
where
    R: rand::Rng,
{
    let opt = match de_type {
        PlainDataElementType::TxPower => Some(rand_de_and_bundle(PlainDataElement::TxPower, rng)),
        PlainDataElementType::Actions => Some(rand_de_and_bundle(PlainDataElement::Actions, rng)),
    };

    // make sure flavor support is consistent
    assert_eq!(opt.is_some(), de_type.supports_flavor(PacketFlavorEnum::Plaintext));

    opt
}

/// Generate a random instance of the requested de type, or `None` if that type does not support
/// ciphertext.
pub(crate) fn random_de_ciphertext<R>(
    de_type: PlainDataElementType,
    rng: &mut R,
) -> Option<(PlainDataElement<Ciphertext>, DataElementBundle<Ciphertext>)>
where
    R: rand::Rng,
{
    let opt = match de_type {
        PlainDataElementType::TxPower => Some(rand_de_and_bundle(PlainDataElement::TxPower, rng)),
        PlainDataElementType::Actions => Some(rand_de_and_bundle(PlainDataElement::Actions, rng)),
    };

    // make sure flavor support is consistent
    assert_eq!(opt.is_some(), de_type.supports_flavor(PacketFlavorEnum::Ciphertext));

    opt
}
