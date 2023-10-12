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
use np_ffi_core::deserialize::v1::*;
use np_ffi_core::utils::FfiEnum;

/// Gets the number of legible sections on a deserialized V1 advertisement.
/// Suitable as an index bound for the second argument of
/// `np_ffi_DeserializedV1Advertisement#get_section`.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV1Advertisement_get_num_legible_sections(
    adv: DeserializedV1Advertisement,
) -> u8 {
    adv.num_legible_sections()
}

/// Gets the number of sections on a deserialized V1 advertisement which
/// were unable to be decrypted with the credentials that the receiver possesses.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV1Advertisement_get_num_undecryptable_sections(
    adv: DeserializedV1Advertisement,
) -> u8 {
    adv.num_undecryptable_sections()
}

/// Gets the legible section with the given index in a deserialized V1 advertisement.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV1Advertisement_get_section(
    adv: DeserializedV1Advertisement,
    legible_section_index: u8,
) -> GetV1SectionResult {
    adv.get_section(legible_section_index)
}

/// Gets the tag of the `GetV1SectionResult` tagged-union.
#[no_mangle]
pub extern "C" fn np_ffi_GetV1SectionResult_kind(
    result: GetV1SectionResult,
) -> GetV1SectionResultKind {
    result.kind()
}

/// Casts a `GetV1SectionResult` to the `Success` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_GetV1SectionResult_into_SUCCESS(
    result: GetV1SectionResult,
) -> DeserializedV1Section {
    unwrap(result.into_success(), PanicReason::EnumCastFailed)
}

/// Gets the number of data elements in a deserialized v1 section.
/// Suitable as an iteration bound for the second argument of
/// `np_ffi_DeserializedV1Section_get_de`.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV1Section_get_num_des(section: DeserializedV1Section) -> u8 {
    section.num_des()
}

/// Gets the tag of the identity tagged-union used for the passed section.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV1Section_get_identity_kind(
    section: DeserializedV1Section,
) -> DeserializedV1IdentityKind {
    section.identity_kind()
}

/// Gets the data-element with the given index in the passed section.
#[no_mangle]
pub extern "C" fn np_ffi_DeserializedV1Section_get_de(
    section: DeserializedV1Section,
    de_index: u8,
) -> GetV1DEResult {
    section.get_de(de_index)
}

/// Gets the tag of the `GetV1DEResult` tagged-union.
#[no_mangle]
pub extern "C" fn np_ffi_GetV1DEResult_kind(result: GetV1DEResult) -> GetV1DEResultKind {
    result.kind()
}

/// Casts a `GetV1DEResult` to the `Success` vartiant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_GetV1DEResult_into_SUCCESS(result: GetV1DEResult) -> V1DataElement {
    unwrap(result.into_success(), PanicReason::EnumCastFailed)
}

/// Converts a `V1DataElement` to a `GenericV1DataElement` which
/// only maintains information about the DE's type-code and payload.
#[no_mangle]
pub extern "C" fn np_ffi_V1DataElement_to_generic(de: V1DataElement) -> GenericV1DataElement {
    de.to_generic()
}

/// Extracts the numerical value of the given V1 DE type code as
/// an unsigned 32-bit integer.
#[no_mangle]
pub extern "C" fn np_ffi_V1DEType_to_uint32_t(de_type: V1DEType) -> u32 {
    de_type.to_u32()
}
