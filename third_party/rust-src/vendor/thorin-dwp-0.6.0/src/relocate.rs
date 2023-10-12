//! Apply relocations to addresses and offsets during parsing, instead of requiring the data
//! to be fully relocated prior to parsing. Necessary to load object files that reference dwarf
//! objects (not just executables). Implementation derived from Gimli's `dwarfdump` example.

use std::borrow::Cow;

use gimli;
use hashbrown::HashMap;
use object::{Object, ObjectSection, ObjectSymbol, RelocationKind, RelocationTarget};

use crate::{Error, Result};

pub(crate) type RelocationMap = HashMap<usize, object::Relocation>;

#[derive(Debug, Clone)]
pub(crate) struct Relocate<'a, R: gimli::Reader<Offset = usize>> {
    pub(crate) relocations: &'a RelocationMap,
    pub(crate) section: R,
    pub(crate) reader: R,
}

impl<'a, R: gimli::Reader<Offset = usize>> Relocate<'a, R> {
    fn relocate(&self, offset: usize, value: u64) -> u64 {
        if let Some(relocation) = self.relocations.get(&offset) {
            if matches!(relocation.kind(), RelocationKind::Absolute) {
                if relocation.has_implicit_addend() {
                    // Use the explicit addend too, because it may have the symbol value.
                    return value.wrapping_add(relocation.addend() as u64);
                } else {
                    return relocation.addend() as u64;
                }
            }
        };

        value
    }
}

impl<'a, R: gimli::Reader<Offset = usize>> gimli::Reader for Relocate<'a, R> {
    type Endian = R::Endian;
    type Offset = R::Offset;

    fn read_address(&mut self, address_size: u8) -> gimli::Result<u64> {
        let offset = self.reader.offset_from(&self.section);
        let value = self.reader.read_address(address_size)?;
        Ok(self.relocate(offset, value))
    }

    fn read_length(&mut self, format: gimli::Format) -> gimli::Result<usize> {
        let offset = self.reader.offset_from(&self.section);
        let value = self.reader.read_length(format)?;
        <usize as gimli::ReaderOffset>::from_u64(self.relocate(offset, value as u64))
    }

    fn read_offset(&mut self, format: gimli::Format) -> gimli::Result<usize> {
        let offset = self.reader.offset_from(&self.section);
        let value = self.reader.read_offset(format)?;
        <usize as gimli::ReaderOffset>::from_u64(self.relocate(offset, value as u64))
    }

    fn read_sized_offset(&mut self, size: u8) -> gimli::Result<usize> {
        let offset = self.reader.offset_from(&self.section);
        let value = self.reader.read_sized_offset(size)?;
        <usize as gimli::ReaderOffset>::from_u64(self.relocate(offset, value as u64))
    }

    #[inline]
    fn split(&mut self, len: Self::Offset) -> gimli::Result<Self> {
        let mut other = self.clone();
        other.reader.truncate(len)?;
        self.reader.skip(len)?;
        Ok(other)
    }

    // All remaining methods simply delegate to `self.reader`.

    #[inline]
    fn endian(&self) -> Self::Endian {
        self.reader.endian()
    }

    #[inline]
    fn len(&self) -> Self::Offset {
        self.reader.len()
    }

    #[inline]
    fn empty(&mut self) {
        self.reader.empty()
    }

    #[inline]
    fn truncate(&mut self, len: Self::Offset) -> gimli::Result<()> {
        self.reader.truncate(len)
    }

    #[inline]
    fn offset_from(&self, base: &Self) -> Self::Offset {
        self.reader.offset_from(&base.reader)
    }

    #[inline]
    fn offset_id(&self) -> gimli::ReaderOffsetId {
        self.reader.offset_id()
    }

    #[inline]
    fn lookup_offset_id(&self, id: gimli::ReaderOffsetId) -> Option<Self::Offset> {
        self.reader.lookup_offset_id(id)
    }

    #[inline]
    fn find(&self, byte: u8) -> gimli::Result<Self::Offset> {
        self.reader.find(byte)
    }

    #[inline]
    fn skip(&mut self, len: Self::Offset) -> gimli::Result<()> {
        self.reader.skip(len)
    }

    #[inline]
    fn to_slice(&self) -> gimli::Result<Cow<'_, [u8]>> {
        self.reader.to_slice()
    }

    #[inline]
    fn to_string(&self) -> gimli::Result<Cow<'_, str>> {
        self.reader.to_string()
    }

    #[inline]
    fn to_string_lossy(&self) -> gimli::Result<Cow<'_, str>> {
        self.reader.to_string_lossy()
    }

    #[inline]
    fn read_slice(&mut self, buf: &mut [u8]) -> gimli::Result<()> {
        self.reader.read_slice(buf)
    }
}

pub(crate) fn add_relocations(
    relocations: &mut RelocationMap,
    file: &object::File<'_>,
    section: &object::Section<'_, '_>,
) -> Result<()> {
    for (offset64, mut relocation) in section.relocations() {
        let offset = offset64 as usize;
        if offset as u64 != offset64 {
            continue;
        }

        let offset = offset as usize;
        if matches!(relocation.kind(), RelocationKind::Absolute) {
            if let RelocationTarget::Symbol(symbol_idx) = relocation.target() {
                match file.symbol_by_index(symbol_idx) {
                    Ok(symbol) => {
                        let addend = symbol.address().wrapping_add(relocation.addend() as u64);
                        relocation.set_addend(addend as i64);
                    }
                    Err(_) => {
                        return Err(Error::RelocationWithInvalidSymbol(
                            section
                                .name()
                                .map_err(|e| Error::NamelessSection(e, offset))?
                                .to_string(),
                            offset,
                        ));
                    }
                }
            }

            if relocations.insert(offset, relocation).is_some() {
                return Err(Error::MultipleRelocations(
                    section.name().map_err(|e| Error::NamelessSection(e, offset))?.to_string(),
                    offset,
                ));
            }
        } else {
            return Err(Error::UnsupportedRelocation(
                section.name().map_err(|e| Error::NamelessSection(e, offset))?.to_string(),
                offset,
            ));
        }
    }

    Ok(())
}
