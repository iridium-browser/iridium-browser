use std::{collections::HashSet, fmt};

use gimli::{Encoding, RunTimeEndian, UnitHeader, UnitIndex, UnitSectionOffset, UnitType};
use object::{
    write::{Object as WritableObject, SectionId},
    BinaryFormat, Object, ObjectSection, SectionKind,
};
use tracing::debug;

use crate::{
    error::{Error, Result},
    ext::{CompressedDataRangeExt, EndianityExt, IndexSectionExt, PackageFormatExt},
    index::{write_index, Bucketable, Contribution, ContributionOffset, IndexEntry},
    relocate::RelocationMap,
    strings::PackageStringTable,
    Session,
};

/// New-type'd index (constructed from `gimli::DwoId`) with a custom `Debug` implementation to
/// print in hexadecimal.
#[derive(Copy, Clone, Eq, Hash, PartialEq)]
pub(crate) struct DwoId(pub(crate) u64);

impl Bucketable for DwoId {
    fn index(&self) -> u64 {
        self.0
    }
}

impl fmt::Debug for DwoId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "DwoId({:#x})", self.0)
    }
}

impl From<gimli::DwoId> for DwoId {
    fn from(dwo_id: gimli::DwoId) -> Self {
        Self(dwo_id.0)
    }
}

/// New-type'd index (constructed from `gimli::DebugTypeSignature`) with a custom `Debug`
/// implementation to print in hexadecimal.
#[derive(Copy, Clone, Eq, Hash, PartialEq)]
pub(crate) struct DebugTypeSignature(pub(crate) u64);

impl Bucketable for DebugTypeSignature {
    fn index(&self) -> u64 {
        self.0
    }
}

impl fmt::Debug for DebugTypeSignature {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "DebugTypeSignature({:#x})", self.0)
    }
}

impl From<gimli::DebugTypeSignature> for DebugTypeSignature {
    fn from(signature: gimli::DebugTypeSignature) -> Self {
        Self(signature.0)
    }
}

/// Identifier for a DWARF object.
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq)]
pub(crate) enum DwarfObject {
    /// `DwoId` identifying compilation units.
    Compilation(DwoId),
    /// `DebugTypeSignature` identifying type units.
    Type(DebugTypeSignature),
}

impl Bucketable for DwarfObject {
    fn index(&self) -> u64 {
        match *self {
            DwarfObject::Compilation(dwo_id) => dwo_id.index(),
            DwarfObject::Type(type_signature) => type_signature.index(),
        }
    }
}

/// Returns the `DwoId` or `DebugTypeSignature` of a unit.
///
/// **DWARF 5:**
///
/// - `DwoId` is in the unit header of a skeleton unit (identifying the split compilation unit
/// that contains the debuginfo) or split compilation unit (identifying the skeleton unit that this
/// debuginfo corresponds to).
/// - `DebugTypeSignature` is in the unit header of a split type unit.
///
/// **Earlier DWARF versions with GNU extension:**
///
/// - `DW_AT_GNU_dwo_id` attribute of the DIE contains the `DwoId`.
#[tracing::instrument(level = "trace", skip(debug_abbrev, header))]
pub(crate) fn dwo_identifier_of_unit<R: gimli::Reader>(
    debug_abbrev: &gimli::DebugAbbrev<R>,
    header: &gimli::UnitHeader<R>,
) -> Result<Option<DwarfObject>> {
    match header.type_() {
        // Compilation units with DWARF 5
        UnitType::Skeleton(dwo_id) | UnitType::SplitCompilation(dwo_id) => {
            Ok(Some(DwarfObject::Compilation(dwo_id.into())))
        }
        // Compilation units with GNU Extension
        UnitType::Compilation => {
            let abbreviations =
                header.abbreviations(&debug_abbrev).map_err(Error::ParseUnitAbbreviations)?;
            let mut cursor = header.entries(&abbreviations);
            cursor.next_dfs()?;
            let root = cursor.current().ok_or(Error::NoDie)?;
            match root.tag() {
                gimli::DW_TAG_compile_unit | gimli::DW_TAG_type_unit => (),
                _ => return Err(Error::TopLevelDieNotUnit),
            }
            let mut attrs = root.attrs();
            while let Some(attr) = attrs.next().map_err(Error::ParseUnitAttribute)? {
                match (attr.name(), attr.value()) {
                    (gimli::constants::DW_AT_GNU_dwo_id, gimli::AttributeValue::DwoId(dwo_id)) => {
                        return Ok(Some(DwarfObject::Compilation(dwo_id.into())))
                    }
                    _ => (),
                }
            }

            Ok(None)
        }
        // Type units with DWARF 5
        UnitType::SplitType { type_signature, .. } => {
            Ok(Some(DwarfObject::Type(type_signature.into())))
        }
        // Type units with GNU extension
        UnitType::Type { type_signature, .. } => Ok(Some(DwarfObject::Type(type_signature.into()))),
        // Wrong compilation unit type.
        _ => Ok(None),
    }
}

/// Wrapper around `.debug_info.dwo` and `debug_types.dwo` unit iterators for uniform handling.
enum UnitHeaderIterator<R: gimli::Reader> {
    DebugInfo(gimli::read::DebugInfoUnitHeadersIter<R>),
    DebugTypes(gimli::read::DebugTypesUnitHeadersIter<R>),
}

impl<R: gimli::Reader> UnitHeaderIterator<R> {
    fn next(&mut self) -> gimli::read::Result<Option<UnitHeader<R>>> {
        match self {
            UnitHeaderIterator::DebugInfo(iter) => iter.next(),
            UnitHeaderIterator::DebugTypes(iter) => iter.next(),
        }
    }
}

/// Returns the parsed unit index from a `.debug_{cu,tu}_index` section.
pub(crate) fn maybe_load_index_section<'input, 'session: 'input, Endian, Index, R, Sess>(
    sess: &'session Sess,
    encoding: Encoding,
    endian: Endian,
    input: &object::File<'input>,
) -> Result<Option<UnitIndex<R>>>
where
    Endian: gimli::Endianity,
    Index: IndexSectionExt<'input, Endian, R>,
    R: gimli::Reader,
    Sess: Session<RelocationMap>,
{
    let index_name = Index::id().dwo_name().expect("index id w/out known value");
    if let Some(index_section) = input.section_by_name(index_name) {
        let index_data = index_section
            .compressed_data()
            .and_then(|d| d.decompress())
            .map_err(Error::DecompressData)?;
        let index_data_ref = sess.alloc_owned_cow(index_data);
        let unit_index = Index::new(index_data_ref, endian)
            .index()
            .map_err(|e| Error::ParseIndex(e, index_name.to_string()))?;

        if !encoding.is_compatible_dwarf_package_index_version(unit_index.version()) {
            return Err(Error::IncompatibleIndexVersion(
                index_name.to_string(),
                encoding.dwarf_package_index_version(),
                unit_index.version(),
            ));
        }

        Ok(Some(unit_index))
    } else {
        Ok(None)
    }
}

/// Returns a closure which takes an identifier and a `Option<Contribution>`, and returns an
/// adjusted contribution if the input file is a DWARF package (and the contribution was
/// present).
///
/// For example, consider the `.debug_str_offsets` section: DWARF packages have a single
/// `.debug_str_offsets` section which contains the string offsets of all of its compilation/type
/// units, the contributions of each unit into that section are tracked in its
/// `.debug_{cu,tu}_index` section.
///
/// When a DWARF package is the input, the contributions of the units which constituted that
/// package should not be lost when its `.debug_str_offsets` section is merged with the new
/// DWARF package currently being created.
///
/// Given a parsed index section, use the size of its contribution to `.debug_str_offsets` as the
/// size of its contribution in the new unit (without this, it would be the size of the entire
/// `.debug_str_offsets` section from the input, rather than the part that the compilation unit
/// originally contributed to that). For subsequent units from the input, the offset in the
/// contribution will need to be adjusted to based on the size of the previous units.
///
/// This function returns a "contribution adjustor" closure, which adjusts the contribution's
/// offset and size according to its contribution in the input's index and with an offset
/// accumulated over all calls to the closure.
pub(crate) fn create_contribution_adjustor<'input, R: 'input>(
    cu_index: Option<&'input UnitIndex<R>>,
    tu_index: Option<&'input UnitIndex<R>>,
    target_section_id: gimli::SectionId,
) -> impl FnMut(DwarfObject, Option<Contribution>) -> Result<Option<Contribution>> + 'input
where
    R: gimli::Reader,
{
    let mut cu_adjustment = 0;
    let mut tu_adjustment = 0;

    move |identifier: DwarfObject,
          contribution: Option<Contribution>|
          -> Result<Option<Contribution>> {
        let (adjustment, index) = match identifier {
            DwarfObject::Compilation(_) => (&mut cu_adjustment, &cu_index),
            DwarfObject::Type(_) => (&mut tu_adjustment, &tu_index),
        };
        match (index, contribution) {
            // dwp input with section
            (Some(index), Some(contribution)) => {
                let idx = identifier.index();
                let row_id = index.find(idx).ok_or(Error::UnitNotInIndex(idx))?;
                let section = index
                    .sections(row_id)
                    .map_err(|e| Error::RowNotInIndex(e, row_id))?
                    .find(|index_section| index_section.section == target_section_id)
                    .ok_or(Error::SectionNotInRow)?;
                let adjusted_offset: u64 = contribution.offset.0 + *adjustment;
                *adjustment += section.size as u64;

                Ok(Some(Contribution {
                    offset: ContributionOffset(adjusted_offset),
                    size: section.size as u64,
                }))
            }
            // dwp input without section
            (Some(_) | None, None) => Ok(contribution),
            // dwo input with section, but we aren't adjusting this particular index
            (None, Some(_)) => Ok(contribution),
        }
    }
}

/// Wrapper around `object::write::Object` that keeps track of the section indexes relevant to
/// DWARF packaging.
struct DwarfPackageObject<'file> {
    /// Object file being created.
    obj: WritableObject<'file>,

    /// Identifier for output `.debug_cu_index.dwo` section.
    debug_cu_index: Option<SectionId>,
    /// `.debug_tu_index.dwo`
    debug_tu_index: Option<SectionId>,
    /// `.debug_info.dwo`
    debug_info: Option<SectionId>,
    /// `.debug_abbrev.dwo`
    debug_abbrev: Option<SectionId>,
    /// `.debug_str.dwo`
    debug_str: Option<SectionId>,
    /// `.debug_types.dwo`
    debug_types: Option<SectionId>,
    /// `.debug_line.dwo`
    debug_line: Option<SectionId>,
    /// `.debug_loc.dwo`
    debug_loc: Option<SectionId>,
    /// `.debug_loclists.dwo`
    debug_loclists: Option<SectionId>,
    /// `.debug_rnglists.dwo`
    debug_rnglists: Option<SectionId>,
    /// `.debug_str_offsets.dwo`
    debug_str_offsets: Option<SectionId>,
    /// `.debug_macinfo.dwo`
    debug_macinfo: Option<SectionId>,
    /// `.debug_macro.dwo`
    debug_macro: Option<SectionId>,
}

/// Macro for generating helper functions which appending non-empty data to specific sections.
macro_rules! generate_append_for {
    ( $( $fn_name:ident => ($name:ident, $section_name:expr) ),+ ) => {
        $(
            fn $fn_name(&mut self, data: &[u8]) -> Option<Contribution> {
                if data.is_empty() {
                    return None;
                }

                let id = *self.$name.get_or_insert_with(|| self.obj.add_section(
                    Vec::new(),
                    Vec::from($section_name),
                    SectionKind::Debug,
                ));

                // FIXME: correct alignment
                let offset = self.obj.append_section_data(id, data, 1);
                debug!(?offset, ?data);
                Some(Contribution {
                    offset: ContributionOffset(offset),
                    size: data.len().try_into().expect("data size larger than u64"),
                })
            }
        )+
    };
}

impl<'file> DwarfPackageObject<'file> {
    /// Create a new `DwarfPackageObject` from an architecture and endianness.
    #[tracing::instrument(level = "trace")]
    pub(crate) fn new(
        architecture: object::Architecture,
        endianness: object::Endianness,
    ) -> DwarfPackageObject<'file> {
        let obj = WritableObject::new(BinaryFormat::Elf, architecture, endianness);
        Self {
            obj,
            debug_cu_index: Default::default(),
            debug_tu_index: Default::default(),
            debug_info: Default::default(),
            debug_abbrev: Default::default(),
            debug_str: Default::default(),
            debug_types: Default::default(),
            debug_line: Default::default(),
            debug_loc: Default::default(),
            debug_loclists: Default::default(),
            debug_rnglists: Default::default(),
            debug_str_offsets: Default::default(),
            debug_macinfo: Default::default(),
            debug_macro: Default::default(),
        }
    }

    generate_append_for! {
        append_to_debug_abbrev => (debug_abbrev, ".debug_abbrev.dwo"),
        append_to_debug_cu_index => (debug_cu_index, ".debug_cu_index"),
        append_to_debug_info => (debug_info, ".debug_info.dwo"),
        append_to_debug_line => (debug_line, ".debug_line.dwo"),
        append_to_debug_loc => (debug_loc, ".debug_loc.dwo"),
        append_to_debug_loclists => (debug_loclists, ".debug_loclists.dwo"),
        append_to_debug_macinfo => (debug_macinfo, ".debug_macinfo.dwo"),
        append_to_debug_macro => (debug_macro, ".debug_macro.dwo"),
        append_to_debug_rnglists => (debug_rnglists, ".debug_rnglists.dwo"),
        append_to_debug_str => (debug_str, ".debug_str.dwo"),
        append_to_debug_str_offsets => (debug_str_offsets, ".debug_str_offsets.dwo"),
        append_to_debug_tu_index => (debug_tu_index, ".debug_tu_index"),
        append_to_debug_types => (debug_types, ".debug_types.dwo")
    }

    /// Return the DWARF package object file.
    pub(crate) fn finish(self) -> WritableObject<'file> {
        self.obj
    }
}

/// In-progress DWARF package being produced.
pub(crate) struct InProgressDwarfPackage<'file> {
    /// Endianness of the DWARF package being created.
    endian: RunTimeEndian,

    /// Object file being created.
    obj: DwarfPackageObject<'file>,
    /// In-progress string table being accumulated.
    ///
    /// Used to write final `.debug_str.dwo` and `.debug_str_offsets.dwo`.
    string_table: PackageStringTable,

    /// Compilation unit index entries (offsets + sizes) being accumulated.
    cu_index_entries: Vec<IndexEntry>,
    /// Type unit index entries (offsets + sizes) being accumulated.
    tu_index_entries: Vec<IndexEntry>,

    /// `DebugTypeSignature`s of type units and `DwoId`s of compilation units that have already
    /// been added to the output package.
    ///
    /// Used when adding new TU index entries to de-duplicate type units (as required by the
    /// specification). Also used to check that all dwarf objects referenced by executables
    /// have been found.
    contained_units: HashSet<DwarfObject>,
}

impl<'file> InProgressDwarfPackage<'file> {
    /// Create an object file with empty sections that will be later populated from DWARF object
    /// files.
    #[tracing::instrument(level = "trace")]
    pub(crate) fn new(
        architecture: object::Architecture,
        endianness: object::Endianness,
    ) -> InProgressDwarfPackage<'file> {
        let endian = endianness.as_runtime_endian();
        Self {
            endian,
            obj: DwarfPackageObject::new(architecture, endianness),
            string_table: PackageStringTable::new(),
            cu_index_entries: Default::default(),
            tu_index_entries: Default::default(),
            contained_units: Default::default(),
        }
    }

    /// Returns the units contained within the DWARF package.
    pub(crate) fn contained_units(&self) -> &HashSet<DwarfObject> {
        &self.contained_units
    }

    /// Process an input DWARF object.
    ///
    /// Copies relevant debug sections, compilation/type units and strings from the `input` DWARF
    /// object into this DWARF package.
    #[tracing::instrument(level = "trace", skip(sess, input,))]
    pub(crate) fn add_input_object<'input, 'session: 'input>(
        &mut self,
        sess: &'session impl Session<RelocationMap>,
        input: &object::File<'input>,
        encoding: Encoding,
    ) -> Result<()> {
        // Load index sections (if they exist).
        let cu_index = maybe_load_index_section::<_, gimli::DebugCuIndex<_>, _, _>(
            sess,
            encoding,
            self.endian,
            input,
        )?;
        let tu_index = maybe_load_index_section::<_, gimli::DebugTuIndex<_>, _, _>(
            sess,
            encoding,
            self.endian,
            input,
        )?;

        let mut debug_abbrev = None;
        let mut debug_line = None;
        let mut debug_loc = None;
        let mut debug_loclists = None;
        let mut debug_macinfo = None;
        let mut debug_macro = None;
        let mut debug_rnglists = None;
        let mut debug_str_offsets = None;

        macro_rules! update {
            ($target:ident += $source:expr) => {
                if let Some(other) = $source {
                    let contribution = $target.get_or_insert(Contribution { size: 0, ..other });
                    contribution.size += other.size;
                }
                debug!(?$target);
            };
        }

        // Iterate over sections rather than using `section_by_name` because sections can be
        // repeated.
        for section in input.sections() {
            match section.name() {
                Ok(".debug_abbrev.dwo" | ".zdebug_abbrev.dwo") => {
                    let data = section.compressed_data()?.decompress()?;
                    update!(debug_abbrev += self.obj.append_to_debug_abbrev(&data));
                }
                Ok(".debug_line.dwo" | ".zdebug_line.dwo") => {
                    let data = section.compressed_data()?.decompress()?;
                    update!(debug_line += self.obj.append_to_debug_line(&data));
                }
                Ok(".debug_loc.dwo" | ".zdebug_loc.dwo") => {
                    let data = section.compressed_data()?.decompress()?;
                    update!(debug_loc += self.obj.append_to_debug_loc(&data));
                }
                Ok(".debug_loclists.dwo" | ".zdebug_loclists.dwo") => {
                    let data = section.compressed_data()?.decompress()?;
                    update!(debug_loclists += self.obj.append_to_debug_loclists(&data));
                }
                Ok(".debug_macinfo.dwo" | ".zdebug_macinfo.dwo") => {
                    let data = section.compressed_data()?.decompress()?;
                    update!(debug_macinfo += self.obj.append_to_debug_macinfo(&data));
                }
                Ok(".debug_macro.dwo" | ".zdebug_macro.dwo") => {
                    let data = section.compressed_data()?.decompress()?;
                    update!(debug_macro += self.obj.append_to_debug_macro(&data));
                }
                Ok(".debug_rnglists.dwo" | ".zdebug_rnglists.dwo") => {
                    let data = section.compressed_data()?.decompress()?;
                    update!(debug_rnglists += self.obj.append_to_debug_rnglists(&data));
                }
                Ok(".debug_str_offsets.dwo" | ".zdebug_str_offsets.dwo") => {
                    let debug_str_offsets_section = {
                        let data = section.compressed_data()?.decompress()?;
                        let data_ref = sess.alloc_owned_cow(data);
                        gimli::DebugStrOffsets::from(gimli::EndianSlice::new(data_ref, self.endian))
                    };

                    let debug_str_section =
                        if let Some(section) = input.section_by_name(".debug_str.dwo") {
                            let data = section.compressed_data()?.decompress()?;
                            let data_ref = sess.alloc_owned_cow(data);
                            gimli::DebugStr::new(data_ref, self.endian)
                        } else {
                            return Err(Error::MissingRequiredSection(".debug_str.dwo"));
                        };

                    let data = self.string_table.remap_str_offsets_section(
                        debug_str_section,
                        debug_str_offsets_section,
                        section.size(),
                        self.endian,
                        encoding,
                    )?;
                    update!(
                        debug_str_offsets += self.obj.append_to_debug_str_offsets(data.slice())
                    );
                }
                _ => (),
            }
        }

        // `.debug_abbrev.dwo`'s contribution will already have been processed, but getting the
        // `DwoId` of a GNU Extension compilation unit requires access to it.
        let debug_abbrev_section = if let Some(section) = input.section_by_name(".debug_abbrev.dwo")
        {
            let data = section.compressed_data()?.decompress()?;
            let data_ref = sess.alloc_owned_cow(data);
            gimli::DebugAbbrev::new(data_ref, self.endian)
        } else {
            return Err(Error::MissingRequiredSection(".debug_abbrev.dwo"));
        };

        // Create offset adjustor functions, see comment on `create_contribution_adjustor` for
        // explanation.
        let mut abbrev_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugAbbrev,
        );
        let mut line_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugLine,
        );
        let mut loc_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugLoc,
        );
        let mut loclists_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugLocLists,
        );
        let mut rnglists_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugRngLists,
        );
        let mut str_offsets_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugStrOffsets,
        );
        let mut macinfo_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugMacinfo,
        );
        let mut macro_adjustor = create_contribution_adjustor(
            cu_index.as_ref(),
            tu_index.as_ref(),
            gimli::SectionId::DebugMacro,
        );

        let mut seen_debug_info = false;
        let mut seen_debug_types = false;

        for section in input.sections() {
            let data;
            let mut iter = match section.name() {
                Ok(".debug_info.dwo" | ".zdebug_info.dwo")
                    // Report an error if a input DWARF package has multiple `.debug_info`
                    // sections.
                    if seen_debug_info && cu_index.is_some() =>
                {
                    return Err(Error::MultipleDebugInfoSection);
                }
                Ok(".debug_info.dwo" | ".zdebug_info.dwo") => {
                    data = section.compressed_data()?.decompress()?;
                    seen_debug_info = true;
                    UnitHeaderIterator::DebugInfo(
                        gimli::DebugInfo::new(&data, self.endian).units(),
                    )
                }
                Ok(".debug_types.dwo" | ".zdebug_types.dwo")
                    // Report an error if a input DWARF package has multiple `.debug_types`
                    // sections.
                    if seen_debug_types && tu_index.is_some() =>
                {
                    return Err(Error::MultipleDebugTypesSection);
                }
                Ok(".debug_types.dwo" | ".zdebug_types.dwo") => {
                    data = section.compressed_data()?.decompress()?;
                    seen_debug_types = true;
                    UnitHeaderIterator::DebugTypes(
                        gimli::DebugTypes::new(&data, self.endian).units(),
                    )
                }
                _ => continue,
            };

            while let Some(header) = iter.next().map_err(Error::ParseUnitHeader)? {
                let id = match dwo_identifier_of_unit(&debug_abbrev_section, &header)? {
                    // Report an error if the unit doesn't have a `DwoId` or `DebugTypeSignature`.
                    None => {
                        return Err(Error::NotSplitUnit);
                    }
                    // Report an error when a duplicate compilation unit is found.
                    Some(id @ DwarfObject::Compilation(dwo_id))
                        if self.contained_units.contains(&id) =>
                    {
                        return Err(Error::DuplicateUnit(dwo_id.0));
                    }
                    // Skip duplicate type units, these happen during proper operation of `thorin`.
                    Some(id @ DwarfObject::Type(type_sig))
                        if self.contained_units.contains(&id) =>
                    {
                        debug!(?type_sig, "skipping duplicate type unit, already seen");
                        continue;
                    }
                    Some(id) => id,
                };

                let size: u64 = header
                    .length_including_self()
                    .try_into()
                    .expect("unit header length larger than u64");
                let offset = match header.offset() {
                    UnitSectionOffset::DebugInfoOffset(offset) => offset.0,
                    UnitSectionOffset::DebugTypesOffset(offset) => offset.0,
                };

                let data = section
                    .compressed_data_range(
                        sess,
                        offset.try_into().expect("offset larger than u64"),
                        size,
                    )
                    .map_err(Error::DecompressData)?
                    .ok_or(Error::EmptyUnit(id.index()))?;

                let (debug_info, debug_types) = match (&iter, id) {
                    (UnitHeaderIterator::DebugTypes(_), DwarfObject::Type(_)) => {
                        (None, self.obj.append_to_debug_types(data))
                    }
                    (_, DwarfObject::Compilation(_) | DwarfObject::Type(_)) => {
                        (self.obj.append_to_debug_info(data), None)
                    }
                };

                let debug_abbrev = abbrev_adjustor(id, debug_abbrev)?;
                let debug_line = line_adjustor(id, debug_line)?;
                let debug_loc = loc_adjustor(id, debug_loc)?;
                let debug_loclists = loclists_adjustor(id, debug_loclists)?;
                let debug_rnglists = rnglists_adjustor(id, debug_rnglists)?;
                let debug_str_offsets = str_offsets_adjustor(id, debug_str_offsets)?;
                let debug_macinfo = macinfo_adjustor(id, debug_macinfo)?;
                let debug_macro = macro_adjustor(id, debug_macro)?;

                let entry = IndexEntry {
                    encoding,
                    id,
                    debug_info,
                    debug_types,
                    debug_abbrev,
                    debug_line,
                    debug_loc,
                    debug_loclists,
                    debug_rnglists,
                    debug_str_offsets,
                    debug_macinfo,
                    debug_macro,
                };
                debug!(?entry);

                match id {
                    DwarfObject::Compilation(_) => self.cu_index_entries.push(entry),
                    DwarfObject::Type(_) => self.tu_index_entries.push(entry),
                }
                self.contained_units.insert(id);
            }
        }

        if !seen_debug_info {
            // Report an error if no `.debug_info` section was found.
            return Err(Error::MissingRequiredSection(".debug_info.dwo"));
        }

        Ok(())
    }

    /// Return the DWARF package object being created, writing any final sections.
    pub(crate) fn finish(self) -> Result<WritableObject<'file>> {
        let Self { mut obj, string_table, cu_index_entries, tu_index_entries, .. } = self;

        // Write `.debug_str` to the object.
        let _ = obj.append_to_debug_str(&string_table.finish());

        // Write `.debug_{cu,tu}_index` sections to the object.
        debug!("writing cu index");
        let cu_index_data = write_index(self.endian, &cu_index_entries)?;
        let _ = obj.append_to_debug_cu_index(cu_index_data.slice());
        debug!("writing tu index");
        let tu_index_data = write_index(self.endian, &tu_index_entries)?;
        let _ = obj.append_to_debug_tu_index(tu_index_data.slice());

        Ok(obj.finish())
    }
}

impl<'file> fmt::Debug for InProgressDwarfPackage<'file> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "InProgressDwarfPackage")
    }
}
