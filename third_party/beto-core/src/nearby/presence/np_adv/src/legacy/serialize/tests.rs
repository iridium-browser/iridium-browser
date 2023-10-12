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

use crate::legacy::actions::FastPairSass;
use crate::legacy::actions::NearbyShare;
use crate::{
    de_type::EncryptedIdentityDataElementType,
    legacy::{actions::*, data_elements::*, serialize::*},
    shared_data::TxPower,
};
use crypto_provider_default::CryptoProviderImpl;
use ldt_np_adv::{salt_padder, LdtEncrypterXtsAes128, LegacySalt};
use std::vec;

#[test]
fn public_identity_packet_serialization() {
    let mut builder = AdvBuilder::new(PublicIdentity::default());

    let tx_power = TxPower::try_from(3).unwrap();
    let mut action = ActionBits::default();
    action.set_action(NearbyShare::from(true));
    builder.add_data_element(TxPowerDataElement::from(tx_power)).unwrap();
    builder.add_data_element(ActionsDataElement::from(action)).unwrap();

    let packet = builder.into_advertisement().unwrap();
    assert_eq!(
        &[
            0x00, // Adv Header
            0x03, // Public DE header
            0x15, 0x03, // Tx Power DE with value 3
            0x26, 0x00, 0x40, // Actions DE w/ bit 9
        ],
        packet.as_slice()
    );
}

#[test]
fn no_identity_packet_serialization() {
    let mut builder = AdvBuilder::new(NoIdentity::default());

    let tx_power = TxPower::try_from(3).unwrap();
    let mut action = ActionBits::default();
    action.set_action(NearbyShare::from(true));
    builder.add_data_element(TxPowerDataElement::from(tx_power)).unwrap();
    builder.add_data_element(ActionsDataElement::from(action)).unwrap();

    let packet = builder.into_advertisement().unwrap();
    assert_eq!(
        &[
            0x00, // Adv Header
            0x15, 0x03, // Tx Power DE with value 3
            0x26, 0x00, 0x40, // Actions DE w/ bit 9
        ],
        packet.as_slice()
    );
}

#[test]
fn packet_limits_capacity() {
    let mut builder = AdvBuilder::new(PublicIdentity::default());
    // 2 + 1 left out of 24 payload bytes
    builder.len = 21;
    let mut bits = ActionBits::default();
    bits.set_action(NearbyShare::from(true));
    bits.set_action(FastPairSass::from(true));

    assert_eq!(Ok(()), builder.add_data_element(ActionsDataElement::from(bits)));

    // too small for 2+ 1 DE
    builder.len = 22;
    assert_eq!(
        Err(AddDataElementError::InsufficientAdvSpace),
        builder.add_data_element(ActionsDataElement::from(bits))
    );
}

#[test]
fn ldt_packet_serialization() {
    // don't care about the HMAC since we're not decrypting
    let hkdf = np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&[0; 32]);
    let ldt = LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(&hkdf.legacy_ldt_key());
    let metadata_key = [0x33; 14];
    let salt = LegacySalt::from([0x01, 0x02]);

    let mut ciphertext = vec![];
    ciphertext.extend_from_slice(&metadata_key);
    // tx power & action DEs
    ciphertext.extend_from_slice(&[0x15, 0x03, 0x26, 0x00, 0x10]);
    ldt.encrypt(&mut ciphertext, &salt_padder::<16, CryptoProviderImpl>(salt)).unwrap();

    let mut builder = AdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
        EncryptedIdentityDataElementType::Private,
        salt,
        metadata_key,
        ldt,
    ));

    let tx_power = TxPower::try_from(3).unwrap();
    let mut action = ActionBits::default();
    action.set_action(PhoneHub::from(true));
    builder.add_data_element(TxPowerDataElement::from(tx_power)).unwrap();
    builder.add_data_element(ActionsDataElement::from(action)).unwrap();

    let packet = builder.into_advertisement().unwrap();
    // header
    let mut expected = vec![0x00];
    // private header with five bytes after it
    expected.push(0x51);
    expected.extend_from_slice(salt.bytes());
    expected.extend_from_slice(&ciphertext);
    assert_eq!(&expected, packet.as_slice());
}

#[test]
fn ldt_packet_cant_encrypt_without_des() {
    let metadata_key = [0x33; 14];
    let salt = LegacySalt::from([0x01, 0x02]);

    let builder = AdvBuilder::new(LdtIdentity::<CryptoProviderImpl>::new(
        EncryptedIdentityDataElementType::Private,
        salt,
        metadata_key,
        LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(
            &np_hkdf::NpKeySeedHkdf::<CryptoProviderImpl>::new(&[0xFE; 32]).legacy_ldt_key(),
        ),
    ));

    // not enough ciphertext
    assert_eq!(Err(LdtPostprocessError::InvalidLength), builder.into_advertisement());
}

#[test]
fn nearby_share_action() {
    let mut builder = AdvBuilder::new(PublicIdentity::default());

    let mut action = ActionBits::default();
    action.set_action(NearbyShare::from(true));

    let actions_de = ActionsDataElement::from(action);
    builder.add_data_element(actions_de).unwrap();

    let tx_power_de = TxPowerDataElement::from(TxPower::try_from(-100).unwrap());
    builder.add_data_element(tx_power_de).unwrap();

    assert_eq!(
        &[
            0x00, // version 0
            0x03, // public identity
            0x26, // length 2, DE type 6 for actions
            0x00, 0x40, // bit 9 is active
            0x15, // length 1, DE type 5 for tx power
            0x9C, // tx power of -100
        ],
        builder.into_advertisement().unwrap().as_slice()
    );
}
