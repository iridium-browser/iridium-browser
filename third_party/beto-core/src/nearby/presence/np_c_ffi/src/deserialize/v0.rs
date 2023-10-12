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

use crate::{panic, unwrap, PanicReason};
use np_ffi_core::common::DeallocateResult;
use np_ffi_core::deserialize::v0::v0_payload::V0Payload;
use np_ffi_core::deserialize::v0::*;
use np_ffi_core::utils::FfiEnum;

/// Gets the tag of a `DeserializedV0Advertisement` tagged-union.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV0Advertisement_kind(
    result: DeserializedV0Advertisement,
) -> DeserializedV0AdvertisementKind {
    result.kind()
}

/// Casts a `DeserializedV0Advertisement` to the `Legible` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV0Advertisement_into_LEGIBLE(
    adv: DeserializedV0Advertisement,
) -> LegibleDeserializedV0Advertisement {
    unwrap(adv.into_legible(), PanicReason::EnumCastFailed)
}

/// Gets the number of DEs in a legible deserialized advertisement.
/// Suitable as an iteration bound for `V0Payload#get_de`.
#[no_mangle]
pub extern "C" fn np_ffi_LegibleDeserializedV0Advertisement_get_num_des(
    adv: LegibleDeserializedV0Advertisement,
) -> u8 {
    adv.num_des()
}

/// Gets just the data-element payload of a `LegibleDeserializedV0Advertisement`.
#[no_mangle]
pub extern "C" fn np_ffi_LegibleDeserializedV0Advertisement_into_payload(
    adv: LegibleDeserializedV0Advertisement,
) -> V0Payload {
    adv.into_payload()
}

/// Gets just the identity information associated with a `LegibleDeserializedV0Advertisement`.
#[no_mangle]
pub extern "C" fn np_ffi_LegibleDeserializedV0Advertisement_into_identity(
    adv: LegibleDeserializedV0Advertisement,
) -> DeserializedV0Identity {
    adv.into_identity()
}

/// Deallocates any internal data of a `LegibleDeserializedV0Advertisement`
#[no_mangle]
pub extern "C" fn np_ffi_deallocate_legible_v0_advertisement(
    adv: LegibleDeserializedV0Advertisement,
) -> DeallocateResult {
    adv.deallocate()
}

/// Gets the tag of the `DeserializedV0Identity` tagged-union.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV0Identity_kind(
    identity: DeserializedV0Identity,
) -> DeserializedV0IdentityKind {
    identity.kind()
}

/// Attempts to get the data-element with the given index in the passed v0 adv payload
#[no_mangle]
pub extern "C" fn np_ffi_V0Payload_get_de(payload: V0Payload, index: u8) -> GetV0DEResult {
    payload.get_de(index)
}

/// Deallocates any internal data of a `V0Payload`
#[no_mangle]
pub extern "C" fn np_ffi_deallocate_v0_payload(payload: V0Payload) -> DeallocateResult {
    payload.deallocate_payload()
}

/// Gets the tag of a `GetV0DEResult` tagged-union.
#[no_mangle]
pub extern "C" fn np_ffi_GetV0DEResult_kind(result: GetV0DEResult) -> GetV0DEResultKind {
    result.kind()
}

/// Casts a `GetV0DEResult` to the `Success` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_GetV0DEResult_into_SUCCESS(result: GetV0DEResult) -> V0DataElement {
    unwrap(result.into_success(), PanicReason::EnumCastFailed)
}

/// Gets the tag of a `V0DataElement` tagged-union.
#[no_mangle]
pub extern "C" fn np_ffi_V0DataElement_kind(de: V0DataElement) -> V0DataElementKind {
    de.kind()
}

/// Casts a `V0DataElement` to the `TxPower` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_V0DataElement_into_TX_POWER(de: V0DataElement) -> TxPower {
    unwrap(de.into_tx_power(), PanicReason::EnumCastFailed)
}

/// Casts a `V0DataElement` to the `Actions` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_V0DataElement_into_ACTIONS(de: V0DataElement) -> V0Actions {
    unwrap(de.into_actions(), PanicReason::EnumCastFailed)
}

/// Return whether a boolean action type is set in this data element
#[no_mangle]
pub extern "C" fn np_ffi_V0Actions_has_action(
    actions: V0Actions,
    action_type: BooleanActionType,
) -> bool {
    match actions.has_action(&action_type) {
        Ok(b) => b,
        Err(_) => panic(PanicReason::InvalidActionBits),
    }
}

/// Gets the 4 bit context sync sequence number as a u8 from this data element
#[no_mangle]
pub extern "C" fn np_ffi_V0Actions_get_context_sync_sequence_number(actions: V0Actions) -> u8 {
    match actions.get_context_sync_seq_num() {
        Ok(b) => b,
        Err(_) => panic(PanicReason::InvalidActionBits),
    }
}

/// Return whether a boolean action type is set in this data element
#[no_mangle]
pub extern "C" fn np_ffi_V0Actions_as_u32(actions: V0Actions) -> u32 {
    actions.as_u32()
}
