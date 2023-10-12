//! A string table implementation with a tree-like encoding.
//!
//! Each entry in the table represents a string and is encoded as a list of
//! components where each component can either be
//!
//! 1. a string _value_ that contains actual UTF-8 string content,
//! 2. a string _ID_ that contains a reference to another entry, or
//! 3. a terminator tag which marks the end of a component list.
//!
//! The string _content_ of an entry is defined as the concatenation of the
//! content of its components. The content of a string value is its actual
//! UTF-8 bytes. The content of a string ID is the contents of the entry
//! it references.
//!
//! The byte-level encoding of component lists uses the structure of UTF-8 in
//! order to save space:
//!
//! - A valid UTF-8 codepoint never starts with the byte `0xFE`. We make use
//!   of this fact by letting all string ID components start with this `0xFE`
//!   prefix. Thus when we parse the contents of a value we know to stop if
//!   we encounter this byte.
//!
//! - A valid UTF-8 string cannot contain the `0xFF` byte. Thus we can safely
//!   use `0xFF` as our component list terminator.
//!
//! The sample composite string ["abc", ID(42), "def", TERMINATOR] would thus be
//! encoded as:
//!
//! ```ignore
//!     ['a', 'b' , 'c', 254, 42, 0, 0, 0, 'd', 'e', 'f', 255]
//!                      ^^^^^^^^^^^^^^^^                 ^^^
//!                 string ID with 0xFE prefix      terminator (0xFF)
//! ```
//!
//! As you can see string IDs are encoded in little endian format.
//!
//! ----------------------------------------------------------------------------
//!
//! Each string in the table is referred to via a `StringId`. `StringId`s may
//! be generated in two ways:
//!
//!   1. Calling `StringTableBuilder::alloc()` which returns the `StringId` for
//!      the allocated string.
//!   2. Calling `StringId::new_virtual()` to create a "virtual" `StringId` that
//!      later can be mapped to an actual string via
//!      `StringTableBuilder::map_virtual_to_concrete_string()`.
//!
//! String IDs allow you to deduplicate strings by allocating a string
//! once and then referring to it by id over and over. This is a useful trick
//! for strings which are recorded many times and it can significantly reduce
//! the size of profile trace files.
//!
//! `StringId`s are partitioned according to type:
//!
//! > [0 .. MAX_VIRTUAL_STRING_ID, METADATA_STRING_ID, .. ]
//!
//! From `0` to `MAX_VIRTUAL_STRING_ID` are the allowed values for virtual strings.
//! After `MAX_VIRTUAL_STRING_ID`, there is one string id (`METADATA_STRING_ID`)
//! which is used internally by `measureme` to record additional metadata about
//! the profiling session. After `METADATA_STRING_ID` are all other `StringId`
//! values.

use crate::file_header::{
    write_file_header, FILE_MAGIC_STRINGTABLE_DATA, FILE_MAGIC_STRINGTABLE_INDEX,
};
use crate::serialization::Addr;
use crate::serialization::SerializationSink;
use std::{error::Error, sync::Arc};

/// A `StringId` is used to identify a string in the `StringTable`. It is
/// either a regular `StringId`, meaning that it contains the absolute address
/// of a string within the string table data. Or it is "virtual", which means
/// that the address it points to is resolved via the string table index data,
/// that maps virtual `StringId`s to addresses.
#[derive(Clone, Copy, Eq, PartialEq, Debug, Hash)]
#[repr(C)]
pub struct StringId(u32);

impl StringId {
    pub const INVALID: StringId = StringId(INVALID_STRING_ID);

    #[inline]
    pub fn new(id: u32) -> StringId {
        StringId(id)
    }

    #[inline]
    pub fn new_virtual(id: u32) -> StringId {
        assert!(id <= MAX_USER_VIRTUAL_STRING_ID);
        StringId(id)
    }

    #[inline]
    pub fn is_virtual(self) -> bool {
        self.0 <= METADATA_STRING_ID
    }

    #[inline]
    pub fn as_u32(self) -> u32 {
        self.0
    }

    #[inline]
    pub fn from_addr(addr: Addr) -> StringId {
        let id = addr.0.checked_add(FIRST_REGULAR_STRING_ID).unwrap();
        StringId::new(id)
    }

    #[inline]
    pub fn to_addr(self) -> Addr {
        Addr(self.0.checked_sub(FIRST_REGULAR_STRING_ID).unwrap())
    }
}

// See module-level documentation for more information on the encoding.
pub const TERMINATOR: u8 = 0xFF;
pub const STRING_REF_TAG: u8 = 0xFE;
pub const STRING_REF_ENCODED_SIZE: usize = 5;

/// The maximum id value a virtual string may be.
const MAX_USER_VIRTUAL_STRING_ID: u32 = 100_000_000;

/// The id of the profile metadata string entry.
pub const METADATA_STRING_ID: u32 = MAX_USER_VIRTUAL_STRING_ID + 1;

/// Some random string ID that we make sure cannot be generated or assigned to.
const INVALID_STRING_ID: u32 = METADATA_STRING_ID + 1;

pub const FIRST_REGULAR_STRING_ID: u32 = INVALID_STRING_ID + 1;

/// Write-only version of the string table
pub struct StringTableBuilder {
    data_sink: Arc<SerializationSink>,
    index_sink: Arc<SerializationSink>,
}

/// Anything that implements `SerializableString` can be written to a
/// `StringTable`.
pub trait SerializableString {
    fn serialized_size(&self) -> usize;
    fn serialize(&self, bytes: &mut [u8]);
}

// A single string is encoded as `[UTF-8 bytes][TERMINATOR]`
impl SerializableString for str {
    #[inline]
    fn serialized_size(&self) -> usize {
        self.len() + // actual bytes
        1 // terminator
    }

    #[inline]
    fn serialize(&self, bytes: &mut [u8]) {
        let last_byte_index = bytes.len() - 1;
        bytes[0..last_byte_index].copy_from_slice(self.as_bytes());
        bytes[last_byte_index] = TERMINATOR;
    }
}

/// A single component of a string. Used for building composite table entries.
pub enum StringComponent<'s> {
    Value(&'s str),
    Ref(StringId),
}

impl<'s> StringComponent<'s> {
    #[inline]
    fn serialized_size(&self) -> usize {
        match *self {
            StringComponent::Value(s) => s.len(),
            StringComponent::Ref(_) => STRING_REF_ENCODED_SIZE,
        }
    }

    #[inline]
    fn serialize<'b>(&self, bytes: &'b mut [u8]) -> &'b mut [u8] {
        match *self {
            StringComponent::Value(s) => {
                bytes[..s.len()].copy_from_slice(s.as_bytes());
                &mut bytes[s.len()..]
            }
            StringComponent::Ref(string_id) => {
                // The code below assumes we use a 5-byte encoding for string
                // refs, where the first byte is STRING_REF_TAG and the
                // following 4 bytes are a little-endian u32 string ID value.
                assert!(STRING_REF_ENCODED_SIZE == 5);

                bytes[0] = STRING_REF_TAG;
                bytes[1..5].copy_from_slice(&string_id.0.to_le_bytes());
                &mut bytes[5..]
            }
        }
    }
}

impl<'a> SerializableString for [StringComponent<'a>] {
    #[inline]
    fn serialized_size(&self) -> usize {
        self.iter().map(|c| c.serialized_size()).sum::<usize>() + // size of components
        1 // terminator
    }

    #[inline]
    fn serialize(&self, mut bytes: &mut [u8]) {
        assert!(bytes.len() == self.serialized_size());
        for component in self.iter() {
            bytes = component.serialize(bytes);
        }

        // Assert that we used the exact number of bytes we anticipated.
        assert!(bytes.len() == 1);
        bytes[0] = TERMINATOR;
    }
}

macro_rules! impl_serializable_string_for_fixed_size {
    ($n:expr) => {
        impl<'a> SerializableString for [StringComponent<'a>; $n] {
            #[inline(always)]
            fn serialized_size(&self) -> usize {
                (&self[..]).serialized_size()
            }

            #[inline(always)]
            fn serialize(&self, bytes: &mut [u8]) {
                (&self[..]).serialize(bytes);
            }
        }
    };
}

impl_serializable_string_for_fixed_size!(0);
impl_serializable_string_for_fixed_size!(1);
impl_serializable_string_for_fixed_size!(2);
impl_serializable_string_for_fixed_size!(3);
impl_serializable_string_for_fixed_size!(4);
impl_serializable_string_for_fixed_size!(5);
impl_serializable_string_for_fixed_size!(6);
impl_serializable_string_for_fixed_size!(7);
impl_serializable_string_for_fixed_size!(8);
impl_serializable_string_for_fixed_size!(9);
impl_serializable_string_for_fixed_size!(10);
impl_serializable_string_for_fixed_size!(11);
impl_serializable_string_for_fixed_size!(12);
impl_serializable_string_for_fixed_size!(13);
impl_serializable_string_for_fixed_size!(14);
impl_serializable_string_for_fixed_size!(15);
impl_serializable_string_for_fixed_size!(16);

fn serialize_index_entry(sink: &SerializationSink, id: StringId, addr: Addr) {
    sink.write_atomic(8, |bytes| {
        bytes[0..4].copy_from_slice(&id.0.to_le_bytes());
        bytes[4..8].copy_from_slice(&addr.0.to_le_bytes());
    });
}

impl StringTableBuilder {
    pub fn new(
        data_sink: Arc<SerializationSink>,
        index_sink: Arc<SerializationSink>,
    ) -> Result<StringTableBuilder, Box<dyn Error + Send + Sync>> {
        // The first thing in every stream we generate must be the stream header.
        write_file_header(&mut data_sink.as_std_write(), FILE_MAGIC_STRINGTABLE_DATA)?;
        write_file_header(&mut index_sink.as_std_write(), FILE_MAGIC_STRINGTABLE_INDEX)?;

        Ok(StringTableBuilder {
            data_sink,
            index_sink,
        })
    }

    /// Creates a mapping so that `virtual_id` will resolve to the contents of
    /// `concrete_id` when reading the string table.
    pub fn map_virtual_to_concrete_string(&self, virtual_id: StringId, concrete_id: StringId) {
        // This assertion does not use `is_virtual` on purpose because that
        // would also allow to overwrite `METADATA_STRING_ID`.
        assert!(virtual_id.0 <= MAX_USER_VIRTUAL_STRING_ID);
        serialize_index_entry(&*self.index_sink, virtual_id, concrete_id.to_addr());
    }

    pub fn bulk_map_virtual_to_single_concrete_string<I>(
        &self,
        virtual_ids: I,
        concrete_id: StringId,
    ) where
        I: Iterator<Item = StringId> + ExactSizeIterator,
    {
        // TODO: Index data encoding could have a special bulk mode that assigns
        //       multiple StringIds to the same addr, so we don't have to repeat
        //       the `concrete_id` over and over.

        type MappingEntry = [u32; 2];
        assert!(std::mem::size_of::<MappingEntry>() == 8);

        let to_addr_le = concrete_id.to_addr().0.to_le();

        let serialized: Vec<MappingEntry> = virtual_ids
            .map(|from| {
                let id = from.0;
                assert!(id <= MAX_USER_VIRTUAL_STRING_ID);
                [id.to_le(), to_addr_le]
            })
            .collect();

        let num_bytes = serialized.len() * std::mem::size_of::<MappingEntry>();
        let byte_ptr = serialized.as_ptr() as *const u8;

        let bytes = unsafe { std::slice::from_raw_parts(byte_ptr, num_bytes) };

        self.index_sink.write_bytes_atomic(bytes);
    }

    pub fn alloc_metadata<STR: SerializableString + ?Sized>(&self, s: &STR) {
        let concrete_id = self.alloc(s);
        let virtual_id = StringId(METADATA_STRING_ID);
        assert!(virtual_id.is_virtual());
        serialize_index_entry(&*self.index_sink, virtual_id, concrete_id.to_addr());
    }

    pub fn alloc<STR: SerializableString + ?Sized>(&self, s: &STR) -> StringId {
        let size_in_bytes = s.serialized_size();
        let addr = self.data_sink.write_atomic(size_in_bytes, |mem| {
            s.serialize(mem);
        });

        StringId::from_addr(addr)
    }
}
