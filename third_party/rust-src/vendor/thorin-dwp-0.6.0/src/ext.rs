use gimli::{Encoding, EndianSlice, RunTimeEndian, UnitIndex};
use object::{Endianness, ObjectSection};

use crate::{relocate::RelocationMap, Session};

/// Helper trait to translate between `object`'s `Endianness` and `gimli`'s `RunTimeEndian`.
pub(crate) trait EndianityExt {
    fn as_runtime_endian(&self) -> RunTimeEndian;
}

impl EndianityExt for Endianness {
    fn as_runtime_endian(&self) -> RunTimeEndian {
        match *self {
            Endianness::Little => RunTimeEndian::Little,
            Endianness::Big => RunTimeEndian::Big,
        }
    }
}

/// Helper trait to add `compressed_data_range` function to `ObjectSection` types.
pub(crate) trait CompressedDataRangeExt<'input, 'session: 'input>:
    ObjectSection<'input>
{
    /// Return the decompressed contents of the section data in the given range.
    ///
    /// Decompression happens only if the data is compressed.
    fn compressed_data_range(
        &self,
        sess: &'session impl Session<RelocationMap>,
        address: u64,
        size: u64,
    ) -> object::Result<Option<&'input [u8]>>;
}

impl<'input, 'session: 'input, S> CompressedDataRangeExt<'input, 'session> for S
where
    S: ObjectSection<'input>,
{
    fn compressed_data_range(
        &self,
        sess: &'session impl Session<RelocationMap>,
        address: u64,
        size: u64,
    ) -> object::Result<Option<&'input [u8]>> {
        let data = self.compressed_data()?.decompress()?;

        /// Originally from `object::read::util`, used in `ObjectSection::data_range`, but not
        /// public.
        fn data_range(
            data: &[u8],
            data_address: u64,
            range_address: u64,
            size: u64,
        ) -> Option<&[u8]> {
            let offset = range_address.checked_sub(data_address)?;
            data.get(offset.try_into().ok()?..)?.get(..size.try_into().ok()?)
        }

        let data_ref = sess.alloc_owned_cow(data);
        Ok(data_range(data_ref, self.address(), address, size))
    }
}

/// Helper trait that abstracts over `gimli::DebugCuIndex` and `gimli::DebugTuIndex`.
pub(crate) trait IndexSectionExt<'input, Endian: gimli::Endianity, R: gimli::Reader>:
    gimli::Section<R>
{
    fn new(section: &'input [u8], endian: Endian) -> Self;

    fn index(self) -> gimli::read::Result<UnitIndex<R>>;
}

impl<'input, Endian: gimli::Endianity> IndexSectionExt<'input, Endian, EndianSlice<'input, Endian>>
    for gimli::DebugCuIndex<EndianSlice<'input, Endian>>
{
    fn new(section: &'input [u8], endian: Endian) -> Self {
        Self::new(section, endian)
    }

    fn index(self) -> gimli::read::Result<UnitIndex<EndianSlice<'input, Endian>>> {
        Self::index(self)
    }
}

impl<'input, Endian: gimli::Endianity> IndexSectionExt<'input, Endian, EndianSlice<'input, Endian>>
    for gimli::DebugTuIndex<EndianSlice<'input, Endian>>
{
    fn new(section: &'input [u8], endian: Endian) -> Self {
        Self::new(section, endian)
    }

    fn index(self) -> gimli::read::Result<UnitIndex<EndianSlice<'input, Endian>>> {
        Self::index(self)
    }
}

/// Helper trait to add DWARF package specific functions to the `Encoding` type.
pub(crate) trait PackageFormatExt {
    /// Returns `true` if this `Encoding` would produce to a DWARF 5-standardized package file.
    ///
    /// See Sec 7.3.5 and Appendix F of the [DWARF specification].
    ///
    /// [DWARF specification]: https://dwarfstd.org/doc/DWARF5.pdf
    fn is_std_dwarf_package_format(&self) -> bool;

    /// Returns `true` if this `Encoding` would produce a GNU Extension DWARF package file
    /// (preceded standardized version from DWARF 5).
    ///
    /// See [specification](https://gcc.gnu.org/wiki/DebugFissionDWP).
    fn is_gnu_extension_dwarf_package_format(&self) -> bool;

    /// Returns index version of DWARF package for this `Encoding`.
    fn dwarf_package_index_version(&self) -> u16;

    /// Returns `true` if the dwarf package index version provided is compatible with this
    /// `Encoding`.
    fn is_compatible_dwarf_package_index_version(&self, index_version: u16) -> bool;
}

impl PackageFormatExt for Encoding {
    fn is_gnu_extension_dwarf_package_format(&self) -> bool {
        !self.is_std_dwarf_package_format()
    }

    fn is_std_dwarf_package_format(&self) -> bool {
        self.version >= 5
    }

    fn dwarf_package_index_version(&self) -> u16 {
        if self.is_gnu_extension_dwarf_package_format() {
            2
        } else {
            5
        }
    }

    fn is_compatible_dwarf_package_index_version(&self, index_version: u16) -> bool {
        if self.is_gnu_extension_dwarf_package_format() {
            index_version == 2
        } else {
            index_version >= 5
        }
    }
}
