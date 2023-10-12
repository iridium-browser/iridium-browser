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
//! Credential-related data-types and functions

use crate::{unwrap, PanicReason};
use np_ffi_core::common::*;
use np_ffi_core::credentials::credential_book::CredentialBook;
use np_ffi_core::credentials::*;
use np_ffi_core::utils::FfiEnum;

/// Allocates a new credential-book, returning a handle to the created object
#[no_mangle]
pub extern "C" fn np_ffi_create_credential_book() -> CreateCredentialBookResult {
    create_credential_book()
}

/// Gets the tag of a `CreateCredentialBookResult` tagged enum.
#[no_mangle]
pub extern "C" fn np_ffi_CreateCredentialBookResult_kind(
    result: CreateCredentialBookResult,
) -> CreateCredentialBookResultKind {
    result.kind()
}

/// Casts a `CreateCredentialBookResult` to the `SUCCESS` variant, panicking in the
/// case where the passed value is of a different enum variant.
#[no_mangle]
pub extern "C" fn np_ffi_CreateCredentialBookResult_into_SUCCESS(
    result: CreateCredentialBookResult,
) -> CredentialBook {
    unwrap(result.into_success(), PanicReason::EnumCastFailed)
}

/// Deallocates a credential-book by its handle
#[no_mangle]
pub extern "C" fn np_ffi_deallocate_credential_book(
    credential_book: CredentialBook,
) -> DeallocateResult {
    deallocate_credential_book(credential_book)
}
