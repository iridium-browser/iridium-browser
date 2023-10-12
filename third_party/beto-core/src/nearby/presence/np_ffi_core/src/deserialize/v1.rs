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
//! Core NP Rust FFI structures and methods for v1 advertisement deserialization.

use crate::common::*;
use crate::credentials::credential_book::CredentialBook;
use crate::utils::*;
use alloc::vec::Vec;
use array_view::ArrayView;
use handle_map::{declare_handle_map, HandleLike, HandleMapDimensions, HandleMapFullError};

/// Representation of a deserialized V1 advertisement
#[repr(C)]
pub struct DeserializedV1Advertisement {
    num_legible_sections: u8,
    num_undecryptable_sections: u8,
    legible_sections: LegibleV1Sections,
}

impl DeserializedV1Advertisement {
    /// Gets the number of legible sections in this deserialized V1 advertisement.
    pub fn num_legible_sections(&self) -> u8 {
        self.num_legible_sections
    }

    /// Gets the number of undecryptable sections in this deserialized V1 advertisement.
    pub fn num_undecryptable_sections(&self) -> u8 {
        self.num_undecryptable_sections
    }

    /// Gets the legible section with the given index
    /// (which is bounded in `0..self.num_legible_sections()`)
    pub fn get_section(&self, legible_section_index: u8) -> GetV1SectionResult {
        match self.legible_sections.get() {
            Ok(sections_read_guard) => {
                sections_read_guard.get_section(self.legible_sections, legible_section_index)
            }
            Err(_) => GetV1SectionResult::Error,
        }
    }

    /// Attempts to deallocate memory utilized internally by this V1 advertisement
    /// (which contains a handle to actual advertisement contents behind-the-scenes).
    pub fn deallocate(self) -> DeallocateResult {
        self.legible_sections.deallocate().map(|_| ()).into()
    }

    pub(crate) fn allocate_with_contents<'m, M: np_adv::credential::MatchedCredential<'m>>(
        contents: np_adv::V1AdvContents<'m, M>,
    ) -> Result<Self, HandleMapFullError> {
        // 16-section limit enforced by np_adv
        let num_undecryptable_sections = contents.invalid_sections_count() as u8;
        let legible_sections = contents.into_valid_sections();
        let num_legible_sections = legible_sections.len() as u8;
        let legible_sections = LegibleV1Sections::allocate_with_contents(legible_sections)?;
        Ok(Self { num_undecryptable_sections, num_legible_sections, legible_sections })
    }
}

/// Internal, Rust-side implementation of a listing of legible sections
/// in a deserialized V1 advertisement
pub struct LegibleV1SectionsInternals {
    sections: Vec<DeserializedV1SectionInternals>,
}

impl LegibleV1SectionsInternals {
    fn get_section_internals(
        &self,
        legible_section_index: u8,
    ) -> Option<&DeserializedV1SectionInternals> {
        self.sections.get(legible_section_index as usize)
    }
    fn get_section(
        &self,
        legible_sections_handle: LegibleV1Sections,
        legible_section_index: u8,
    ) -> GetV1SectionResult {
        match self.get_section_internals(legible_section_index) {
            Some(section_ref) => {
                // Determine whether the section is plaintext
                // or decrypted to report back to the caller,
                // and also determine the number of contained DEs.
                let num_des = section_ref.num_des();
                let identity_tag = section_ref.identity_kind();
                GetV1SectionResult::Success(DeserializedV1Section {
                    legible_sections_handle,
                    legible_section_index,
                    num_des,
                    identity_tag,
                })
            }
            None => GetV1SectionResult::Error,
        }
    }
}

impl<'m, M: np_adv::credential::MatchedCredential<'m>>
    From<Vec<np_adv::V1DeserializedSection<'m, M>>> for LegibleV1SectionsInternals
{
    fn from(contents: Vec<np_adv::V1DeserializedSection<'m, M>>) -> Self {
        let sections = contents.into_iter().map(DeserializedV1SectionInternals::from).collect();
        Self { sections }
    }
}

fn get_legible_v1_sections_handle_map_dimensions() -> HandleMapDimensions {
    HandleMapDimensions {
        num_shards: global_num_shards(),
        max_active_handles: global_max_num_deserialized_v1_advertisements(),
    }
}

declare_handle_map! {legible_v1_sections, LegibleV1Sections, super::LegibleV1SectionsInternals, super::get_legible_v1_sections_handle_map_dimensions() }
use legible_v1_sections::LegibleV1Sections;

impl LocksLongerThan<LegibleV1Sections> for CredentialBook {}

impl LegibleV1Sections {
    pub(crate) fn allocate_with_contents<'m, M: np_adv::credential::MatchedCredential<'m>>(
        contents: Vec<np_adv::V1DeserializedSection<'m, M>>,
    ) -> Result<Self, HandleMapFullError> {
        Self::allocate(move || LegibleV1SectionsInternals::from(contents))
    }
}

/// Discriminant for `GetV1SectionResult`
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum GetV1SectionResultKind {
    /// The attempt to get the section failed,
    /// possibly due to the section index being
    /// out-of-bounds or due to the underlying
    /// advertisement having already been deallocated.
    Error = 0,
    /// The attempt to get the section succeeded.
    /// The wrapped section may be obtained via
    /// `GetV1SectionResult#into_success`.
    Success = 1,
}

/// The result of attempting to get a particular V1 section
/// from its' index within the list of legible sections
/// via `DeserializedV1Advertisement::get_section`.
#[repr(C)]
#[allow(missing_docs)]
pub enum GetV1SectionResult {
    Error,
    Success(DeserializedV1Section),
}

impl FfiEnum for GetV1SectionResult {
    type Kind = GetV1SectionResultKind;
    fn kind(&self) -> Self::Kind {
        match self {
            GetV1SectionResult::Error => GetV1SectionResultKind::Error,
            GetV1SectionResult::Success(_) => GetV1SectionResultKind::Success,
        }
    }
}

impl GetV1SectionResult {
    declare_enum_cast! {into_success, Success, DeserializedV1Section}
}

/// The internal FFI-friendly representation of a deserialized v1 section
pub struct DeserializedV1SectionInternals {
    des: Vec<V1DataElement>,
    identity: DeserializedV1Identity,
}

impl DeserializedV1SectionInternals {
    /// Gets the number of data-elements in this section.
    fn num_des(&self) -> u8 {
        self.des.len() as u8
    }
    /// Gets the enum tag of the identity used for this section.
    fn identity_kind(&self) -> DeserializedV1IdentityKind {
        self.identity.kind()
    }
    /// Attempts to get the DE with the given index in this section.
    fn get_de(&self, index: u8) -> GetV1DEResult {
        match self.des.get(index as usize) {
            Some(de) => GetV1DEResult::Success(de.clone()),
            None => GetV1DEResult::Error,
        }
    }
}

impl<'m, M: np_adv::credential::MatchedCredential<'m>> From<np_adv::V1DeserializedSection<'m, M>>
    for DeserializedV1SectionInternals
{
    fn from(section: np_adv::V1DeserializedSection<'m, M>) -> Self {
        use np_adv::extended::deserialize::Section;
        use np_adv::V1DeserializedSection;
        match section {
            V1DeserializedSection::Plaintext(section) => {
                let des = section.data_elements().map(V1DataElement::from).collect();
                let identity = DeserializedV1Identity::Plaintext;
                Self { des, identity }
            }
            V1DeserializedSection::Decrypted(_) => {
                unimplemented!();
            }
        }
    }
}
/// Discriminant for `DeserializedV1Identity`.
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum DeserializedV1IdentityKind {
    /// The deserialized v1 identity was plaintext
    Plaintext = 0,
    /// The deserialized v1 identity corresponded
    /// to some kind of decrypted identity.
    Decrypted = 1,
}

/// Deserialized information about the identity
/// employed in a V1 adveritsement section.
#[derive(Clone, Copy)]
#[repr(C)]
#[allow(missing_docs)]
pub enum DeserializedV1Identity {
    Plaintext,
    // TODO: This gets a payload once we support creds
    Decrypted,
}

impl FfiEnum for DeserializedV1Identity {
    type Kind = DeserializedV1IdentityKind;
    fn kind(&self) -> Self::Kind {
        match self {
            DeserializedV1Identity::Plaintext => DeserializedV1IdentityKind::Plaintext,
            DeserializedV1Identity::Decrypted => DeserializedV1IdentityKind::Decrypted,
        }
    }
}

/// Handle to a deserialized V1 section
#[repr(C)]
pub struct DeserializedV1Section {
    legible_sections_handle: LegibleV1Sections,
    legible_section_index: u8,
    num_des: u8,
    identity_tag: DeserializedV1IdentityKind,
}

impl DeserializedV1Section {
    /// Gets the number of data elements contained in this section.
    /// Suitable as an iteration bound on `Self::get_de`.
    pub fn num_des(&self) -> u8 {
        self.num_des
    }

    /// Gets the enum tag of the identity employed by this deserialized section.
    pub fn identity_kind(&self) -> DeserializedV1IdentityKind {
        self.identity_tag
    }

    /// Gets the DE with the given index in this section.
    pub fn get_de(&self, de_index: u8) -> GetV1DEResult {
        // TODO: Once the `FromResidual` trait is stabilized, this can be simplified.
        match self.legible_sections_handle.get() {
            Ok(legible_sections_read_guard) => {
                match legible_sections_read_guard.get_section_internals(self.legible_section_index)
                {
                    Some(section_ref) => section_ref.get_de(de_index),
                    None => GetV1DEResult::Error,
                }
            }
            Err(_) => GetV1DEResult::Error,
        }
    }
}

/// Discriminant for the `GetV1DEResult` enum.
#[derive(Clone, Copy)]
#[repr(u8)]
pub enum GetV1DEResultKind {
    /// Attempting to get the DE at the given position failed,
    /// possibly due to the index being out-of-bounds or due
    /// to the whole advertisement having been previously deallocated.
    Error = 0,
    /// Attempting to get the DE at the given position succeeded.
    /// The underlying DE may be extracted with `GetV1DEResult#into_success`.
    Success = 1,
}

/// Represents the result of the `DeserializedV1Section#get_de` operation.
#[repr(C)]
#[allow(missing_docs)]
pub enum GetV1DEResult {
    Error,
    Success(V1DataElement),
}

impl FfiEnum for GetV1DEResult {
    type Kind = GetV1DEResultKind;
    fn kind(&self) -> Self::Kind {
        match self {
            GetV1DEResult::Error => GetV1DEResultKind::Error,
            GetV1DEResult::Success(_) => GetV1DEResultKind::Success,
        }
    }
}

impl GetV1DEResult {
    declare_enum_cast! {into_success, Success, V1DataElement}
}

/// FFI-transmissible representation of a V1 data-element
#[derive(Clone)]
#[repr(C)]
pub enum V1DataElement {
    /// A "generic" V1 data-element, for which we have no
    /// particular information about its schema (just
    /// a DE type code and a byte payload.)
    Generic(GenericV1DataElement),
}

impl V1DataElement {
    // Note: not using declare_enum_cast! for this one, because if V1DataElement
    // gets more variants, this will have a different internal implementation
    /// Converts a `V1DataElement` to a `GenericV1DataElement` which
    /// only maintains information about the DE's type-code and payload.
    pub fn to_generic(self) -> GenericV1DataElement {
        match self {
            V1DataElement::Generic(x) => x,
        }
    }
}

impl<'a> From<np_adv::extended::deserialize::DataElement<'a>> for V1DataElement {
    fn from(de: np_adv::extended::deserialize::DataElement<'a>) -> Self {
        let de_type = V1DEType::from(de.de_type());
        let contents_as_slice = de.contents();
        //Guaranteed not to panic due DE size limit.
        #[allow(clippy::unwrap_used)]
        let array_view: ArrayView<u8, 127> = ArrayView::try_from_slice(contents_as_slice).unwrap();
        let payload = ByteBuffer::from_array_view(array_view);
        Self::Generic(GenericV1DataElement { de_type, payload })
    }
}

/// FFI-transmissible representation of a generic V1 data-element.
/// This representation is stable, and so you may directly
/// reference this struct's fields if you wish.
#[derive(Clone)]
#[repr(C)]
pub struct GenericV1DataElement {
    /// The DE type code of this generic data-element.
    pub de_type: V1DEType,
    /// The raw data-element byte payload, up to
    /// 127 bytes in length.
    pub payload: ByteBuffer<127>,
}

impl GenericV1DataElement {
    /// Gets the DE-type of this generic V1 data element.
    pub fn de_type(&self) -> V1DEType {
        self.de_type
    }
    /// Destructures this `GenericV1DataElement` into just the
    /// DE payload byte-buffer.
    pub fn into_payload(self) -> ByteBuffer<127> {
        self.payload
    }
}

/// Representation of the data-element type tag
/// of a V1 data element.
#[derive(Clone, Copy)]
#[repr(C)]
pub struct V1DEType {
    code: u32,
}

impl From<np_adv::extended::de_type::DeType> for V1DEType {
    fn from(de_type: np_adv::extended::de_type::DeType) -> Self {
        let code = de_type.as_u32();
        Self { code }
    }
}

impl V1DEType {
    /// Yields this V1 DE type code as a u32.
    pub fn to_u32(&self) -> u32 {
        self.code
    }
}
