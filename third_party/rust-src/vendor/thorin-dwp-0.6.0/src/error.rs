use std::error::Error as StdError;
use std::fmt;

pub(crate) type Result<T> = std::result::Result<T, Error>;

/// Helper trait for converting an error to a `&dyn std::error::Error`.
pub trait AsDynError<'a> {
    fn as_dyn_error(&self) -> &(dyn StdError + 'a);
}

impl<'a, T: StdError + 'a> AsDynError<'a> for T {
    #[inline]
    fn as_dyn_error(&self) -> &(dyn StdError + 'a) {
        self
    }
}

/// Diagnostics (and contexts) emitted during DWARF packaging.
#[derive(Debug)]
#[non_exhaustive]
pub enum Error {
    /// Failure to read input file.
    ///
    /// This error occurs in the `Session::read_input` function provided by the user of `thorin`.
    ReadInput(std::io::Error),
    /// Failed to parse kind of input file.
    ///
    /// Input file kind is necessary to determine how to parse the rest of the input, and to
    /// validate that the input file is of a type that `thorin` can process.
    ParseFileKind(object::Error),
    /// Failed to parse object file.
    ParseObjectFile(object::Error),
    /// Failed to parse archive file.
    ParseArchiveFile(object::Error),
    /// Failed to parse archive member.
    ParseArchiveMember(object::Error),
    /// Invalid kind of input.
    ///
    /// Only archive and elf files are supported input files.
    InvalidInputKind,
    /// Failed to decompress data.
    ///
    /// `thorin` uses `object` for decompression, so `object` probably didn't have support for the
    /// type of compression used.
    DecompressData(object::Error),
    /// Section without a name.
    NamelessSection(object::Error, usize),
    /// Relocation has invalid symbol for a section.
    RelocationWithInvalidSymbol(String, usize),
    /// Multiple relocations for a section.
    MultipleRelocations(String, usize),
    /// Unsupported relocations for a section.
    UnsupportedRelocation(String, usize),
    /// Input object that has a `DwoId` (or `DebugTypeSignature`) does not have a
    /// `DW_AT_GNU_dwo_name` or `DW_AT_dwo_name` attribute.
    MissingDwoName(u64),
    /// Input object has no compilation units.
    NoCompilationUnits,
    /// No top-level debugging information entry in unit.
    NoDie,
    /// Top-level debugging information entry is not a compilation/type unit.
    TopLevelDieNotUnit,
    /// Section required of input DWARF objects was missing.
    MissingRequiredSection(&'static str),
    /// Failed to parse unit abbreviations.
    ParseUnitAbbreviations(gimli::read::Error),
    /// Failed to parse unit attribute.
    ParseUnitAttribute(gimli::read::Error),
    /// Failed to parse unit header.
    ParseUnitHeader(gimli::read::Error),
    /// Failed to parse unit.
    ParseUnit(gimli::read::Error),
    /// Input DWARF package has a different index version than the version being output.
    IncompatibleIndexVersion(String, u16, u16),
    /// Failed to read string offset from `.debug_str_offsets` at index.
    OffsetAtIndex(gimli::read::Error, u64),
    /// Failed to read string from `.debug_str` at offset.
    StrAtOffset(gimli::read::Error, usize),
    /// Failed to parse index section.
    ///
    /// If an input file is a DWARF package, its index section needs to be read to ensure that the
    /// contributions within it are preserved.
    ParseIndex(gimli::read::Error, String),
    /// Compilation unit in DWARF package is not its index.
    UnitNotInIndex(u64),
    /// Row for a compilation unit is not in the index.
    RowNotInIndex(gimli::read::Error, u32),
    /// Section not found in unit's row in index, i.e. a DWARF package contains a section but its
    /// index doesn't record contributions to it.
    SectionNotInRow,
    /// Compilation unit in input DWARF object has no content.
    EmptyUnit(u64),
    /// Found multiple `.debug_info.dwo` sections.
    MultipleDebugInfoSection,
    /// Found multiple `.debug_types.dwo` sections in a DWARF package file.
    MultipleDebugTypesSection,
    /// Found a regular compilation unit in a DWARF object.
    NotSplitUnit,
    /// Found duplicate split compilation unit.
    DuplicateUnit(u64),
    /// Unit referenced by an executable was not found.
    MissingReferencedUnit(u64),
    /// No output object was created from inputs
    NoOutputObjectCreated,
    /// Input objects have different encodings.
    MixedInputEncodings,

    /// Catch-all for `std::io::Error`.
    Io(std::io::Error),
    /// Catch-all for `object::Error`.
    ObjectRead(object::Error),
    /// Catch-all for `object::write::Error`.
    ObjectWrite(object::write::Error),
    /// Catch-all for `gimli::read::Error`.
    GimliRead(gimli::read::Error),
    /// Catch-all for `gimli::write::Error`.
    GimliWrite(gimli::write::Error),
}

impl StdError for Error {
    fn source(&self) -> Option<&(dyn StdError + 'static)> {
        match self {
            Error::ReadInput(source) => Some(source.as_dyn_error()),
            Error::ParseFileKind(source) => Some(source.as_dyn_error()),
            Error::ParseObjectFile(source) => Some(source.as_dyn_error()),
            Error::ParseArchiveFile(source) => Some(source.as_dyn_error()),
            Error::ParseArchiveMember(source) => Some(source.as_dyn_error()),
            Error::InvalidInputKind => None,
            Error::DecompressData(source) => Some(source.as_dyn_error()),
            Error::NamelessSection(source, _) => Some(source.as_dyn_error()),
            Error::RelocationWithInvalidSymbol(_, _) => None,
            Error::MultipleRelocations(_, _) => None,
            Error::UnsupportedRelocation(_, _) => None,
            Error::MissingDwoName(_) => None,
            Error::NoCompilationUnits => None,
            Error::NoDie => None,
            Error::TopLevelDieNotUnit => None,
            Error::MissingRequiredSection(_) => None,
            Error::ParseUnitAbbreviations(source) => Some(source.as_dyn_error()),
            Error::ParseUnitAttribute(source) => Some(source.as_dyn_error()),
            Error::ParseUnitHeader(source) => Some(source.as_dyn_error()),
            Error::ParseUnit(source) => Some(source.as_dyn_error()),
            Error::IncompatibleIndexVersion(_, _, _) => None,
            Error::OffsetAtIndex(source, _) => Some(source.as_dyn_error()),
            Error::StrAtOffset(source, _) => Some(source.as_dyn_error()),
            Error::ParseIndex(source, _) => Some(source.as_dyn_error()),
            Error::UnitNotInIndex(_) => None,
            Error::RowNotInIndex(source, _) => Some(source.as_dyn_error()),
            Error::SectionNotInRow => None,
            Error::EmptyUnit(_) => None,
            Error::MultipleDebugInfoSection => None,
            Error::MultipleDebugTypesSection => None,
            Error::NotSplitUnit => None,
            Error::DuplicateUnit(_) => None,
            Error::MissingReferencedUnit(_) => None,
            Error::NoOutputObjectCreated => None,
            Error::MixedInputEncodings => None,
            Error::Io(transparent) => StdError::source(transparent.as_dyn_error()),
            Error::ObjectRead(transparent) => StdError::source(transparent.as_dyn_error()),
            Error::ObjectWrite(transparent) => StdError::source(transparent.as_dyn_error()),
            Error::GimliRead(transparent) => StdError::source(transparent.as_dyn_error()),
            Error::GimliWrite(transparent) => StdError::source(transparent.as_dyn_error()),
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::ReadInput(_) => write!(f, "Failed to read input file"),
            Error::ParseFileKind(_) => write!(f, "Failed to parse input file kind"),
            Error::ParseObjectFile(_) => write!(f, "Failed to parse input object file"),
            Error::ParseArchiveFile(_) => write!(f, "Failed to parse input archive file"),
            Error::ParseArchiveMember(_) => write!(f, "Failed to parse archive member"),
            Error::InvalidInputKind => write!(f, "Input is not an archive or elf object"),
            Error::DecompressData(_) => write!(f, "Failed to decompress compressed section"),
            Error::NamelessSection(_, offset) => {
                write!(f, "Section without name at offset 0x{:08x}", offset)
            }
            Error::RelocationWithInvalidSymbol(section, offset) => write!(
                f,
                "Relocation with invalid symbol for section `{}` at offset 0x{:08x}",
                section, offset
            ),
            Error::MultipleRelocations(section, offset) => write!(
                f,
                "Multiple relocations for section `{}` at offset 0x{:08x}",
                section, offset
            ),
            Error::UnsupportedRelocation(section, offset) => write!(
                f,
                "Unsupported relocation for section {} at offset 0x{:08x}",
                section, offset
            ),
            Error::MissingDwoName(id) => {
                write!(f, "Missing path attribute to DWARF object (0x{:08x})", id)
            }
            Error::NoCompilationUnits => {
                write!(f, "Input object has no compilation units")
            }
            Error::NoDie => {
                write!(f, "No top-level debugging information entry in compilation/type unit")
            }
            Error::TopLevelDieNotUnit => {
                write!(f, "Top-level debugging information entry is not a compilation/type unit")
            }
            Error::MissingRequiredSection(section) => {
                write!(f, "Input object missing required section `{}`", section)
            }
            Error::ParseUnitAbbreviations(_) => write!(f, "Failed to parse unit abbreviations"),
            Error::ParseUnitAttribute(_) => write!(f, "Failed to parse unit attribute"),
            Error::ParseUnitHeader(_) => write!(f, "Failed to parse unit header"),
            Error::ParseUnit(_) => write!(f, "Failed to parse unit"),
            Error::IncompatibleIndexVersion(section, format, actual) => {
                write!(
                    f,
                    "Incompatible `{}` index version: found version {}, expected version {}",
                    section, actual, format
                )
            }
            Error::OffsetAtIndex(_, index) => {
                write!(f, "Read offset at index {} of `.debug_str_offsets.dwo` section", index)
            }
            Error::StrAtOffset(_, offset) => {
                write!(f, "Read string at offset 0x{:08x} of `.debug_str.dwo` section", offset)
            }
            Error::ParseIndex(_, section) => {
                write!(f, "Failed to parse `{}` index section", section)
            }
            Error::UnitNotInIndex(unit) => {
                write!(f, "Unit 0x{0:08x} from input package is not in its index", unit)
            }
            Error::RowNotInIndex(_, row) => {
                write!(f, "Row {0} found in index's hash table not present in index", row)
            }
            Error::SectionNotInRow => write!(f, "Section not found in unit's row in index"),
            Error::EmptyUnit(unit) => {
                write!(f, "Unit 0x{:08x} in input DWARF object with no data", unit)
            }
            Error::MultipleDebugInfoSection => {
                write!(f, "Multiple `.debug_info.dwo` sections")
            }
            Error::MultipleDebugTypesSection => {
                write!(f, "Multiple `.debug_types.dwo` sections in a package")
            }
            Error::NotSplitUnit => {
                write!(f, "Regular compilation unit in object (missing dwo identifier)")
            }
            Error::DuplicateUnit(unit) => {
                write!(f, "Duplicate split compilation unit (0x{:08x})", unit)
            }
            Error::MissingReferencedUnit(unit) => {
                write!(f, "Unit 0x{:08x} referenced by executable was not found", unit)
            }
            Error::NoOutputObjectCreated => write!(f, "No output object was created from inputs"),
            Error::MixedInputEncodings => write!(f, "Input objects haved mixed encodings"),
            Error::Io(e) => fmt::Display::fmt(e, f),
            Error::ObjectRead(e) => fmt::Display::fmt(e, f),
            Error::ObjectWrite(e) => fmt::Display::fmt(e, f),
            Error::GimliRead(e) => fmt::Display::fmt(e, f),
            Error::GimliWrite(e) => fmt::Display::fmt(e, f),
        }
    }
}

impl From<std::io::Error> for Error {
    fn from(source: std::io::Error) -> Self {
        Error::Io(source)
    }
}

impl From<object::Error> for Error {
    fn from(source: object::Error) -> Self {
        Error::ObjectRead(source)
    }
}

impl From<object::write::Error> for Error {
    fn from(source: object::write::Error) -> Self {
        Error::ObjectWrite(source)
    }
}

impl From<gimli::read::Error> for Error {
    fn from(source: gimli::read::Error) -> Self {
        Error::GimliRead(source)
    }
}

impl From<gimli::write::Error> for Error {
    fn from(source: gimli::write::Error) -> Self {
        Error::GimliWrite(source)
    }
}
