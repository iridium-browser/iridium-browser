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
//! Core NP Rust FFI structures and methods for advertisement deserialization.

use crate::common::*;
use crate::credentials::credential_book::CredentialBook;
use crate::deserialize::v0::*;
use crate::deserialize::v1::*;
use crate::utils::FfiEnum;
use crypto_provider_default::CryptoProviderImpl;
use handle_map::{HandleLike, HandleMapFullError, HandleNotPresentError};

pub mod v0;
pub mod v1;

/// Discriminant for `DeserializeAdvertisementResult`.
#[repr(u8)]
pub enum DeserializeAdvertisementResultKind {
    /// Deserializing the advertisement failed, for some reason or another.
    Error = 0,
    /// The advertisement was correctly deserialized, and it's a V0 advertisement.
    /// `DeserializeAdvertisementResult#into_v0()` is the corresponding cast
    /// to the associated enum variant.
    V0 = 1,
    /// The advertisement was correctly deserialized, and it's a V1 advertisement.
    /// `DeserializeAdvertisementResult#into_v1()` is the corresponding cast
    /// to the associated enum variant.
    V1 = 2,
}

/// The result of calling `np_ffi_deserialize_advertisement`.
/// Must be explicitly deallocated after use with
/// a corresponding `np_ffi_deallocate_deserialize_advertisement_result`
#[repr(u8)]
pub enum DeserializeAdvertisementResult {
    /// Deserializing the advertisement failed, for some reason or another.
    /// `DeserializeAdvertisementResultKind::Error` is the associated enum tag.
    Error,
    /// The advertisement was correctly deserialized, and it's a V0 advertisement.
    /// `DeserializeAdvertisementResultKind::V0` is the associated enum tag.
    V0(DeserializedV0Advertisement),
    /// The advertisement was correctly deserialized, and it's a V1 advertisement.
    /// `DeserializeAdvertisementResultKind::V1` is the associated enum tag.
    V1(DeserializedV1Advertisement),
}

impl FfiEnum for DeserializeAdvertisementResult {
    type Kind = DeserializeAdvertisementResultKind;
    fn kind(&self) -> Self::Kind {
        match self {
            DeserializeAdvertisementResult::Error => DeserializeAdvertisementResultKind::Error,
            DeserializeAdvertisementResult::V0(_) => DeserializeAdvertisementResultKind::V0,
            DeserializeAdvertisementResult::V1(_) => DeserializeAdvertisementResultKind::V1,
        }
    }
}

impl DeserializeAdvertisementResult {
    declare_enum_cast! {into_v0, V0, DeserializedV0Advertisement}
    declare_enum_cast! {into_v1, V1, DeserializedV1Advertisement}

    /// Deallocates any internal data referenced by a `DeserializeAdvertisementResult`
    pub fn deallocate(self) -> DeallocateResult {
        match self {
            DeserializeAdvertisementResult::Error => DeallocateResult::Success,
            DeserializeAdvertisementResult::V0(adv) => adv.deallocate(),
            DeserializeAdvertisementResult::V1(adv) => adv.deallocate(),
        }
    }
}

//TODO: Once the `FromResidual` trait is stabilized, we won't need to do this
enum DeserializeAdvertisementSuccess {
    V0(DeserializedV0Advertisement),
    V1(DeserializedV1Advertisement),
}

struct DeserializeAdvertisementError;

impl From<HandleMapFullError> for DeserializeAdvertisementError {
    fn from(_: HandleMapFullError) -> Self {
        DeserializeAdvertisementError
    }
}

impl From<HandleNotPresentError> for DeserializeAdvertisementError {
    fn from(_: HandleNotPresentError) -> Self {
        DeserializeAdvertisementError
    }
}

impl From<np_adv::AdvDeserializationError> for DeserializeAdvertisementError {
    fn from(_: np_adv::AdvDeserializationError) -> Self {
        DeserializeAdvertisementError
    }
}

type DefaultV0Credential = np_adv::credential::simple::SimpleV0Credential<
    np_adv::credential::v0::MinimumFootprintV0CryptoMaterial,
    (),
>;
type DefaultV1Credential = np_adv::credential::simple::SimpleV1Credential<
    np_adv::credential::v1::MinimumFootprintV1CryptoMaterial,
    (),
>;
type DefaultBothCredentialSource =
    np_adv::credential::source::OwnedBothCredentialSource<DefaultV0Credential, DefaultV1Credential>;

fn deserialize_advertisement_from_slice_internal(
    adv_payload: &[u8],
    credential_book: CredentialBook,
) -> Result<DeserializeAdvertisementSuccess, DeserializeAdvertisementError> {
    // Deadlock Safety: Credential-book locks always live longer than deserialized advs.
    let _credential_book_read_guard = credential_book.get()?;

    //TODO(b/283249542): Use an actual credential source
    let cred_source: DefaultBothCredentialSource = DefaultBothCredentialSource::new_empty();

    let deserialized_advertisement =
        np_adv::deserialize_advertisement::<_, _, _, _, CryptoProviderImpl>(
            adv_payload,
            &cred_source,
        )?;
    match deserialized_advertisement {
        np_adv::DeserializedAdvertisement::V0(adv_contents) => {
            let adv_handle = DeserializedV0Advertisement::allocate_with_contents(adv_contents)?;
            Ok(DeserializeAdvertisementSuccess::V0(adv_handle))
        }
        np_adv::DeserializedAdvertisement::V1(adv_contents) => {
            let adv_handle = DeserializedV1Advertisement::allocate_with_contents(adv_contents)?;
            Ok(DeserializeAdvertisementSuccess::V1(adv_handle))
        }
    }
}

/// Attempts to deserialize an advertisement with the given payload.
/// Suitable for langs which have a suitably expressive slice-type.
pub fn deserialize_advertisement_from_slice(
    adv_payload: &[u8],
    credential_book: CredentialBook,
) -> DeserializeAdvertisementResult {
    let result = deserialize_advertisement_from_slice_internal(adv_payload, credential_book);
    match result {
        Ok(DeserializeAdvertisementSuccess::V0(x)) => DeserializeAdvertisementResult::V0(x),
        Ok(DeserializeAdvertisementSuccess::V1(x)) => DeserializeAdvertisementResult::V1(x),
        Err(_) => DeserializeAdvertisementResult::Error,
    }
}

/// Attempts to deserialize an advertisement with the given payload.
/// Suitable for langs which don't have an expressive-enough slice type.
pub fn deserialize_advertisement(
    adv_payload: &RawAdvertisementPayload,
    credential_book: CredentialBook,
) -> DeserializeAdvertisementResult {
    deserialize_advertisement_from_slice(adv_payload.as_slice(), credential_book)
}
