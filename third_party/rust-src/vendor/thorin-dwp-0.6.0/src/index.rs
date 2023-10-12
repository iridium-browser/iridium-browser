use std::fmt;

use gimli::{
    write::{EndianVec, Writer},
    Encoding,
};
use tracing::{debug, trace};

use crate::{
    error::{Error, Result},
    ext::PackageFormatExt,
    package::DwarfObject,
};

/// Helper trait for types that can be used in creating the `.debug_{cu,tu}_index` hash table.
pub(crate) trait Bucketable {
    fn index(&self) -> u64;
}

/// Returns a hash table computed for `elements`. Used in the `.debug_{cu,tu}_index` sections.
#[tracing::instrument(level = "trace", skip_all)]
fn bucket<B: Bucketable + fmt::Debug>(elements: &[B]) -> Vec<u32> {
    let unit_count: u32 = elements.len().try_into().expect("unit count larger than u32");
    let num_buckets = if elements.len() < 2 { 2 } else { (3 * unit_count / 2).next_power_of_two() };
    let mask: u64 = num_buckets as u64 - 1;
    trace!(?mask);

    let mut buckets = vec![0u32; num_buckets as usize];
    trace!(?buckets);
    for (elem, i) in elements.iter().zip(0u32..) {
        trace!(?i, ?elem);
        let s = elem.index();
        let mut h = s & mask;
        let hp = ((s >> 32) & mask) | 1;
        trace!(?s, ?h, ?hp);

        while buckets[h as usize] > 0 {
            assert!(elements[(buckets[h as usize] - 1) as usize].index() != elem.index());
            h = (h + hp) & mask;
            trace!(?h);
        }

        buckets[h as usize] = i + 1;
        trace!(?buckets);
    }

    buckets
}

/// New-type'd offset into a section of a compilation/type unit's contribution.
#[derive(Copy, Clone, Eq, Hash, PartialEq)]
pub(crate) struct ContributionOffset(pub(crate) u64);

impl fmt::Debug for ContributionOffset {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "ContributionOffset({:#x})", self.0)
    }
}

/// Type alias for the size of a compilation/type unit's contribution.
type ContributionSize = u64;

/// Contribution to a section - offset and size.
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq)]
pub(crate) struct Contribution {
    /// Offset of this contribution into its containing section.
    pub(crate) offset: ContributionOffset,
    /// Size of this contribution in its containing section.
    pub(crate) size: ContributionSize,
}

/// Populated columns in the `.debug_cu_index` or `.debug_tu_index` section.
#[derive(Copy, Clone, Debug, Default)]
pub(crate) struct IndexColumns {
    pub(crate) debug_info: bool,
    pub(crate) debug_types: bool,
    pub(crate) debug_abbrev: bool,
    pub(crate) debug_line: bool,
    pub(crate) debug_loc: bool,
    pub(crate) debug_loclists: bool,
    pub(crate) debug_rnglists: bool,
    pub(crate) debug_str_offsets: bool,
    pub(crate) debug_macinfo: bool,
    pub(crate) debug_macro: bool,
}

impl IndexColumns {
    /// Return the number of columns required for all index entries.
    fn number_of_columns(&self) -> u32 {
        self.debug_info as u32
            + self.debug_types as u32
            + self.debug_abbrev as u32
            + self.debug_line as u32
            + self.debug_loc as u32
            + self.debug_loclists as u32
            + self.debug_rnglists as u32
            + self.debug_str_offsets as u32
            + self.debug_macinfo as u32
            + self.debug_macro as u32
    }

    /// Update the columns to include the columns required for an index entry.
    fn add_entry(&mut self, entry: &IndexEntry) {
        self.debug_info |= entry.debug_info.is_some();
        self.debug_types |= entry.debug_types.is_some();
        self.debug_abbrev |= entry.debug_abbrev.is_some();
        self.debug_line |= entry.debug_line.is_some();
        self.debug_loc |= entry.debug_loc.is_some();
        self.debug_loclists |= entry.debug_loclists.is_some();
        self.debug_rnglists |= entry.debug_rnglists.is_some();
        self.debug_str_offsets |= entry.debug_str_offsets.is_some();
        self.debug_macinfo |= entry.debug_macinfo.is_some();
        self.debug_macro |= entry.debug_macro.is_some();
    }

    /// Write the header row for the columns.
    ///
    /// There is only a single header row in any index section, its contents depend on the output
    /// format and the columns from contributions so the complete index entries are required to
    /// know what header to write.
    fn write_header<Endian: gimli::Endianity>(
        &self,
        out: &mut EndianVec<Endian>,
        encoding: Encoding,
    ) -> Result<()> {
        if encoding.is_gnu_extension_dwarf_package_format() {
            if self.debug_info {
                out.write_u32(gimli::DW_SECT_V2_INFO.0)?;
            }
            if self.debug_types {
                out.write_u32(gimli::DW_SECT_V2_TYPES.0)?;
            }
            if self.debug_abbrev {
                out.write_u32(gimli::DW_SECT_V2_ABBREV.0)?;
            }
            if self.debug_line {
                out.write_u32(gimli::DW_SECT_V2_LINE.0)?;
            }
            if self.debug_loc {
                out.write_u32(gimli::DW_SECT_V2_LOC.0)?;
            }
            if self.debug_str_offsets {
                out.write_u32(gimli::DW_SECT_V2_STR_OFFSETS.0)?;
            }
            if self.debug_macinfo {
                out.write_u32(gimli::DW_SECT_V2_MACINFO.0)?;
            }
            if self.debug_macro {
                out.write_u32(gimli::DW_SECT_V2_MACRO.0)?;
            }
        } else {
            if self.debug_info {
                out.write_u32(gimli::DW_SECT_INFO.0)?;
            }
            if self.debug_abbrev {
                out.write_u32(gimli::DW_SECT_ABBREV.0)?;
            }
            if self.debug_line {
                out.write_u32(gimli::DW_SECT_LINE.0)?;
            }
            if self.debug_loclists {
                out.write_u32(gimli::DW_SECT_LOCLISTS.0)?;
            }
            if self.debug_rnglists {
                out.write_u32(gimli::DW_SECT_RNGLISTS.0)?;
            }
            if self.debug_str_offsets {
                out.write_u32(gimli::DW_SECT_STR_OFFSETS.0)?;
            }
            if self.debug_macro {
                out.write_u32(gimli::DW_SECT_MACRO.0)?;
            }
        }

        Ok(())
    }
}

/// Entry into the `.debug_cu_index` or `.debug_tu_index` section.
///
/// Contributions from `.debug_loclists.dwo` and `.debug_rnglists.dwo` from type units aren't
/// defined in the DWARF 5 specification but are tested by `llvm-dwp`'s test suite.
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq)]
pub(crate) struct IndexEntry {
    pub(crate) encoding: Encoding,
    pub(crate) id: DwarfObject,
    pub(crate) debug_info: Option<Contribution>,
    pub(crate) debug_types: Option<Contribution>,
    pub(crate) debug_abbrev: Option<Contribution>,
    pub(crate) debug_line: Option<Contribution>,
    pub(crate) debug_loc: Option<Contribution>,
    pub(crate) debug_loclists: Option<Contribution>,
    pub(crate) debug_rnglists: Option<Contribution>,
    pub(crate) debug_str_offsets: Option<Contribution>,
    pub(crate) debug_macinfo: Option<Contribution>,
    pub(crate) debug_macro: Option<Contribution>,
}

impl IndexEntry {
    /// Visit each contribution in an entry, calling `proj` to write a specific field.
    #[tracing::instrument(level = "trace", skip(out, proj))]
    fn write_contribution<Endian, Proj>(
        &self,
        out: &mut EndianVec<Endian>,
        proj: Proj,
        columns: &IndexColumns,
    ) -> Result<()>
    where
        Endian: gimli::Endianity,
        Proj: Copy + Fn(Contribution) -> u32,
    {
        let proj = |contrib: Option<Contribution>| contrib.map(proj).unwrap_or(0);
        if columns.debug_info {
            out.write_u32(proj(self.debug_info))?;
        }
        if columns.debug_types {
            out.write_u32(proj(self.debug_types))?;
        }
        if columns.debug_abbrev {
            out.write_u32(proj(self.debug_abbrev))?;
        }
        if columns.debug_line {
            out.write_u32(proj(self.debug_line))?;
        }
        if columns.debug_loc {
            out.write_u32(proj(self.debug_loc))?;
        }
        if columns.debug_loclists {
            out.write_u32(proj(self.debug_loclists))?;
        }
        if columns.debug_rnglists {
            out.write_u32(proj(self.debug_rnglists))?;
        }
        if columns.debug_str_offsets {
            out.write_u32(proj(self.debug_str_offsets))?;
        }
        if columns.debug_macinfo {
            out.write_u32(proj(self.debug_macinfo))?;
        }
        if columns.debug_macro {
            out.write_u32(proj(self.debug_macro))?;
        }

        Ok(())
    }
}

impl Bucketable for IndexEntry {
    fn index(&self) -> u64 {
        self.id.index()
    }
}

/// Write a `.debug_{cu,tu}_index` section given `IndexEntry`s.
#[tracing::instrument(level = "trace")]
pub(crate) fn write_index<'output, Endian: gimli::Endianity>(
    endianness: Endian,
    entries: &[IndexEntry],
) -> Result<EndianVec<Endian>> {
    let mut out = EndianVec::new(endianness);

    if entries.len() == 0 {
        return Ok(out);
    }

    let buckets = bucket(entries);
    debug!(?buckets);

    let encoding = entries[0].encoding;
    if !entries.iter().all(|e| e.encoding == encoding) {
        return Err(Error::MixedInputEncodings);
    }
    debug!(?encoding);

    let mut columns = IndexColumns::default();
    for entry in entries {
        columns.add_entry(entry);
    }
    let num_columns = columns.number_of_columns();
    debug!(?entries, ?columns, ?num_columns);

    // Write header..
    if encoding.is_gnu_extension_dwarf_package_format() {
        // GNU Extension
        out.write_u32(2)?;
    } else {
        // DWARF 5
        out.write_u16(5)?;
        // Reserved padding
        out.write_u16(0)?;
    }

    // Columns (e.g. info, abbrev, loc, etc.)
    out.write_u32(num_columns)?;
    // Number of units
    out.write_u32(entries.len().try_into().expect("number of units larger than u32"))?;
    // Number of buckets
    out.write_u32(buckets.len().try_into().expect("number of buckets larger than u32"))?;

    // Write signatures..
    for i in &buckets {
        if *i > 0 {
            out.write_u64(entries[(*i - 1) as usize].index())?;
        } else {
            out.write_u64(0)?;
        }
    }

    // Write indices..
    for i in &buckets {
        out.write_u32(*i)?;
    }

    // Write column headers..
    columns.write_header(&mut out, encoding)?;

    // Write offsets..
    let write_offset = |contrib: Contribution| {
        contrib.offset.0.try_into().expect("contribution offset larger than u32")
    };
    for entry in entries {
        entry.write_contribution(&mut out, write_offset, &columns)?;
    }

    // Write sizes..
    let write_size =
        |contrib: Contribution| contrib.size.try_into().expect("contribution size larger than u32");
    for entry in entries {
        entry.write_contribution(&mut out, write_size, &columns)?;
    }

    Ok(out)
}
