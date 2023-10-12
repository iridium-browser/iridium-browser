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

use crate::common::*;
use crate::utils::FfiEnum;
use handle_map::{declare_handle_map, HandleLike, HandleMapDimensions, HandleMapFullError};

/// Internal, Rust-side implementation of a credential-book.
/// See [`CredentialBook`] for the FFI-side handles.
// TODO(b/283249542): Give this a real definition!
pub struct CredentialBookInternals;

fn get_credential_book_handle_map_dimensions() -> HandleMapDimensions {
    HandleMapDimensions {
        num_shards: global_num_shards(),
        max_active_handles: global_max_num_credential_books(),
    }
}

declare_handle_map! {credential_book, CredentialBook, super::CredentialBookInternals, super::get_credential_book_handle_map_dimensions()}
use credential_book::CredentialBook;

/// Discriminant for `CreateCredentialBookResult`
#[repr(u8)]
pub enum CreateCredentialBookResultKind {
    /// There was no space left to create a new credential book
    NoSpaceLeft = 0,
    /// We created a new credential book behind the given handle.
    /// The associated payload may be obtained via
    /// `CreateCredentialBookResult#into_success()`.
    Success = 1,
}

/// Result type for `create_credential_book`
#[repr(u8)]
#[allow(missing_docs)]
pub enum CreateCredentialBookResult {
    NoSpaceLeft = 0,
    Success(CredentialBook) = 1,
}

impl From<Result<CredentialBook, HandleMapFullError>> for CreateCredentialBookResult {
    fn from(result: Result<CredentialBook, HandleMapFullError>) -> Self {
        match result {
            Ok(book) => CreateCredentialBookResult::Success(book),
            Err(_) => CreateCredentialBookResult::NoSpaceLeft,
        }
    }
}

/// Allocates a new credential-book, returning a handle to the created object
pub fn create_credential_book() -> CreateCredentialBookResult {
    CredentialBook::allocate(|| CredentialBookInternals).into()
}

impl FfiEnum for CreateCredentialBookResult {
    type Kind = CreateCredentialBookResultKind;
    fn kind(&self) -> Self::Kind {
        match self {
            CreateCredentialBookResult::NoSpaceLeft => CreateCredentialBookResultKind::NoSpaceLeft,
            CreateCredentialBookResult::Success(_) => CreateCredentialBookResultKind::Success,
        }
    }
}

impl CreateCredentialBookResult {
    declare_enum_cast! {into_success, Success, CredentialBook}
}

/// Deallocates a credential-book by its handle
pub fn deallocate_credential_book(credential_book: CredentialBook) -> DeallocateResult {
    credential_book.deallocate().map(|_| ()).into()
}

/// A handle on a particular v0 shared credential stored within a credential book
pub struct V0SharedCredential;
