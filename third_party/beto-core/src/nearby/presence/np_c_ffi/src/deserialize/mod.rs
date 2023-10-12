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

use crate::{unwrap, PanicReason};
use np_ffi_core::common::*;
use np_ffi_core::credentials::credential_book::CredentialBook;
use np_ffi_core::deserialize::v0::*;
use np_ffi_core::deserialize::v1::*;
use np_ffi_core::deserialize::*;
use np_ffi_core::utils::FfiEnum;

mod v0;
mod v1;

/// Attempts to deserialize an advertisement with the given service-data
/// payload (presumed to be under the NP service UUID) using credentials
/// pulled from the given credential-book.
#[no_mangle]
pub extern "C" fn np_ffi_deserialize_advertisement(
    adv_payload: RawAdvertisementPayload,
    credential_book: CredentialBook,
) -> DeserializeAdvertisementResult {
    deserialize_advertisement(&adv_payload, credential_book)
}

/// Gets the tag of a `DeserializeAdvertisementResult` tagged-union.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializeAdvertisementResult_kind(
    result: DeserializeAdvertisementResult,
) -> DeserializeAdvertisementResultKind {
    result.kind()
}

/// Casts a `DeserializeAdvertisementResult` to the `V0` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializeAdvertisementResult_into_V0(
    result: DeserializeAdvertisementResult,
) -> DeserializedV0Advertisement {
    unwrap(result.into_v0(), PanicReason::EnumCastFailed)
}

/// Casts a `DeserializeAdvertisementResult` to the `V1` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializeAdvertisementResult_into_V1(
    result: DeserializeAdvertisementResult,
) -> DeserializedV1Advertisement {
    unwrap(result.into_v1(), PanicReason::EnumCastFailed)
}

/// Deallocates any internal data referenced by a `DeserializeAdvertisementResult`. This should only
/// be used if into_V0 or into_V1, have not been called yet as it shares the same underlying
/// resource.
#[no_mangle]
pub extern "C" fn np_ffi_deallocate_deserialize_advertisement_result(
    result: DeserializeAdvertisementResult,
) -> DeallocateResult {
    result.deallocate()
}

/// Deallocates any internal data referenced by a `DeserializedV0Advertisement`
#[no_mangle]
pub extern "C" fn np_ffi_deallocate_deserialized_V0_advertisement(
    adv: DeserializedV0Advertisement,
) -> DeallocateResult {
    adv.deallocate()
}

/// Deallocates any internal data referenced by a `DeserializedV1Advertisement`
#[no_mangle]
pub extern "C" fn np_ffi_deallocate_deserialized_V1_advertisement(
    adv: DeserializedV1Advertisement,
) -> DeallocateResult {
    adv.deallocate()
}
