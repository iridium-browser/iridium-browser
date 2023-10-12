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

use super::*;
use crate::{
    extended::serialize::{section_tests::SectionBuilderExt, AdvBuilder},
    shared_data::TxPower,
    PublicIdentity,
};

#[test]
fn serialize_tx_power_de() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder.add_de_res(|_| TxPower::try_from(3_i8).map(TxPowerDataElement::from)).unwrap();

    assert_eq!(
        &[
            3,    // section header
            0x03, // public identity
            0x15, // len 1 type 0x05
            3
        ],
        section_builder.into_section().as_slice()
    );
}

#[test]
fn serialize_actions_de_empty() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder.add_de_res(|_| ActionsDataElement::try_from_actions(&[])).unwrap();

    assert_eq!(
        &[
            2,    // section header
            0x03, // public identity
            0x06, // len 0 type 0x06
        ],
        section_builder.into_section().as_slice()
    );
}

#[test]
fn serialize_actions_de_non_empty() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder
        .add_de_res(|_| ActionsDataElement::try_from_actions(&[1, 1, 2, 3, 5, 8]))
        .unwrap();

    assert_eq!(
        &[
            8,    // section header
            0x03, // public identity
            0x66, // len 6 type 0x06
            1, 1, 2, 3, 5, 8 // fibonacci, of course
        ],
        section_builder.into_section().as_slice()
    );
}

#[test]
fn serialize_context_sync_seq_num_de() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder
        .add_de_res(|_| ContextSyncSeqNum::try_from(3).map(ContextSyncSeqNumDataElement::from))
        .unwrap();

    assert_eq!(
        &[
            4,    // section header
            0x03, // public identity
            0x81, 0x13, // len 1 type 0x13
            3,    // seq num
        ],
        section_builder.into_section().as_slice()
    );
}

#[test]
fn serialize_connectivity_info_de_bluetooth() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder.add_de(|_| ConnectivityInfoDataElement::bluetooth([1; 4], [2; 6])).unwrap();

    assert_eq!(
        &[
            14,   // section header
            0x03, // public identity
            0x8B, 0x11, // len 11 type 0x11
            1,    // connectivity type
            1, 1, 1, 1, // svc id
            2, 2, 2, 2, 2, 2 // mac
        ],
        section_builder.into_section().as_slice()
    );
}

#[test]
fn serialize_connectivity_info_de_mdns() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder.add_de(|_| ConnectivityInfoDataElement::mdns([1; 4], 2)).unwrap();

    assert_eq!(
        &[
            9,    // section header
            0x03, // public identity
            0x86, 0x11, // len 11 type 0x11
            2,    // connectivity type
            1, 1, 1, 1, // svc id
            2  // port
        ],
        section_builder.into_section().as_slice()
    );
}

#[test]
fn serialize_connectivity_info_de_wifi_direct() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder
        .add_de(|_| ConnectivityInfoDataElement::wifi_direct([1; 10], [2; 10], [3; 2], 4))
        .unwrap();

    assert_eq!(
        &[
            27,   // section header
            0x03, // public identity
            0x98, 0x11, // len 24 type 0x11
            3,    // connectivity type
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // ssid
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // password
            3, 3, // freq
            4  // port
        ],
        section_builder.into_section().as_slice()
    );
}

#[test]
fn serialize_connectivity_capabilities_de_wifi_direct() {
    let mut adv_builder = AdvBuilder::new();
    let mut section_builder = adv_builder.section_builder(PublicIdentity::default()).unwrap();

    section_builder
        .add_de(|_| ConnectivityCapabilityDataElement::wifi_direct([1; 3], [2; 3]))
        .unwrap();

    assert_eq!(
        &[
            10,   // section header
            0x03, // public identity
            0x87, 0x12, // len 7 type 0x12
            2,    // connectivity type
            1, 1, 1, // supported
            2, 2, 2, // connected
        ],
        section_builder.into_section().as_slice()
    );
}
