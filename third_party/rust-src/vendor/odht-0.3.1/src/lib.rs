//! This crate implements a hash table that can be used as is in its binary, on-disk format.
//! The goal is to provide a high performance data structure that can be used without any significant up-front decoding.
//! The implementation makes no assumptions about alignment or endianess of the underlying data,
//! so a table encoded on one platform can be used on any other platform and
//! the binary data can be mapped into memory at arbitrary addresses.
//!
//!
//! ## Usage
//!
//! In order to use the hash table one needs to implement the `Config` trait.
//! This trait defines how the table is encoded and what hash function is used.
//! With a `Config` in place the `HashTableOwned` type can be used to build and serialize a hash table.
//! The `HashTable` type can then be used to create an almost zero-cost view of the serialized hash table.
//!
//! ```rust
//!
//! use odht::{HashTable, HashTableOwned, Config, FxHashFn};
//!
//! struct MyConfig;
//!
//! impl Config for MyConfig {
//!
//!     type Key = u64;
//!     type Value = u32;
//!
//!     type EncodedKey = [u8; 8];
//!     type EncodedValue = [u8; 4];
//!
//!     type H = FxHashFn;
//!
//!     #[inline] fn encode_key(k: &Self::Key) -> Self::EncodedKey { k.to_le_bytes() }
//!     #[inline] fn encode_value(v: &Self::Value) -> Self::EncodedValue { v.to_le_bytes() }
//!     #[inline] fn decode_key(k: &Self::EncodedKey) -> Self::Key { u64::from_le_bytes(*k) }
//!     #[inline] fn decode_value(v: &Self::EncodedValue) -> Self::Value { u32::from_le_bytes(*v)}
//! }
//!
//! fn main() {
//!     let mut builder = HashTableOwned::<MyConfig>::with_capacity(3, 95);
//!
//!     builder.insert(&1, &2);
//!     builder.insert(&3, &4);
//!     builder.insert(&5, &6);
//!
//!     let serialized = builder.raw_bytes().to_owned();
//!
//!     let table = HashTable::<MyConfig, &[u8]>::from_raw_bytes(
//!         &serialized[..]
//!     ).unwrap();
//!
//!     assert_eq!(table.get(&1), Some(2));
//!     assert_eq!(table.get(&3), Some(4));
//!     assert_eq!(table.get(&5), Some(6));
//! }
//! ```

#![cfg_attr(feature = "nightly", feature(core_intrinsics))]

#[cfg(test)]
extern crate quickcheck;

#[cfg(feature = "nightly")]
macro_rules! likely {
    ($x:expr) => {
        std::intrinsics::likely($x)
    };
}

#[cfg(not(feature = "nightly"))]
macro_rules! likely {
    ($x:expr) => {
        $x
    };
}

#[cfg(feature = "nightly")]
macro_rules! unlikely {
    ($x:expr) => {
        std::intrinsics::unlikely($x)
    };
}

#[cfg(not(feature = "nightly"))]
macro_rules! unlikely {
    ($x:expr) => {
        $x
    };
}

mod error;
mod fxhash;
mod memory_layout;
mod raw_table;
mod swisstable_group_query;
mod unhash;

use error::Error;
use memory_layout::Header;
use std::borrow::{Borrow, BorrowMut};
use swisstable_group_query::REFERENCE_GROUP_SIZE;

pub use crate::fxhash::FxHashFn;
pub use crate::unhash::UnHashFn;

use crate::raw_table::{ByteArray, RawIter, RawTable, RawTableMut};

/// This trait provides a complete "configuration" for a hash table, i.e. it
/// defines the key and value types, how these are encoded and what hash
/// function is being used.
///
/// Implementations of the `encode_key` and `encode_value` methods must encode
/// the given key/value into a fixed size array. The encoding must be
/// deterministic (i.e. no random padding bytes) and must be independent of
/// platform endianess. It is always highly recommended to mark these methods
/// as `#[inline]`.
pub trait Config {
    type Key;
    type Value;

    // The EncodedKey and EncodedValue types must always be a fixed size array of bytes,
    // e.g. [u8; 4].
    type EncodedKey: ByteArray;
    type EncodedValue: ByteArray;

    type H: HashFn;

    /// Implementations of the `encode_key` and `encode_value` methods must encode
    /// the given key/value into a fixed size array. See above for requirements.
    fn encode_key(k: &Self::Key) -> Self::EncodedKey;

    /// Implementations of the `encode_key` and `encode_value` methods must encode
    /// the given key/value into a fixed size array. See above for requirements.
    fn encode_value(v: &Self::Value) -> Self::EncodedValue;

    fn decode_key(k: &Self::EncodedKey) -> Self::Key;
    fn decode_value(v: &Self::EncodedValue) -> Self::Value;
}

/// This trait represents hash functions as used by HashTable and
/// HashTableOwned.
pub trait HashFn: Eq {
    fn hash(bytes: &[u8]) -> u32;
}

/// A [HashTableOwned] keeps the underlying data on the heap and
/// can resize itself on demand.
#[derive(Clone)]
pub struct HashTableOwned<C: Config> {
    allocation: memory_layout::Allocation<C, Box<[u8]>>,
}

impl<C: Config> Default for HashTableOwned<C> {
    fn default() -> Self {
        HashTableOwned::with_capacity(12, 87)
    }
}

impl<C: Config> HashTableOwned<C> {
    /// Creates a new [HashTableOwned] that can hold at least `max_item_count`
    /// items while maintaining the specified load factor.
    pub fn with_capacity(max_item_count: usize, max_load_factor_percent: u8) -> HashTableOwned<C> {
        assert!(max_load_factor_percent <= 100);
        assert!(max_load_factor_percent > 0);

        Self::with_capacity_internal(
            max_item_count,
            Factor::from_percent(max_load_factor_percent),
        )
    }

    fn with_capacity_internal(max_item_count: usize, max_load_factor: Factor) -> HashTableOwned<C> {
        let slots_needed = slots_needed(max_item_count, max_load_factor);
        assert!(slots_needed > 0);

        let allocation = memory_layout::allocate(slots_needed, 0, max_load_factor);

        HashTableOwned { allocation }
    }

    /// Retrieves the value for the given key. Returns `None` if no entry is found.
    #[inline]
    pub fn get(&self, key: &C::Key) -> Option<C::Value> {
        let encoded_key = C::encode_key(key);
        if let Some(encoded_value) = self.as_raw().find(&encoded_key) {
            Some(C::decode_value(encoded_value))
        } else {
            None
        }
    }

    #[inline]
    pub fn contains_key(&self, key: &C::Key) -> bool {
        let encoded_key = C::encode_key(key);
        self.as_raw().find(&encoded_key).is_some()
    }

    /// Inserts the given key-value pair into the table.
    /// Grows the table if necessary.
    #[inline]
    pub fn insert(&mut self, key: &C::Key, value: &C::Value) -> Option<C::Value> {
        let (item_count, max_item_count) = {
            let header = self.allocation.header();
            let max_item_count = max_item_count_for(header.slot_count(), header.max_load_factor());
            (header.item_count(), max_item_count)
        };

        if unlikely!(item_count == max_item_count) {
            self.grow();
        }

        debug_assert!(
            item_count
                < max_item_count_for(
                    self.allocation.header().slot_count(),
                    self.allocation.header().max_load_factor()
                )
        );

        let encoded_key = C::encode_key(key);
        let encoded_value = C::encode_value(value);

        with_raw_mut(&mut self.allocation, |header, mut raw_table| {
            if let Some(old_value) = raw_table.insert(encoded_key, encoded_value) {
                Some(C::decode_value(&old_value))
            } else {
                header.set_item_count(item_count + 1);
                None
            }
        })
    }

    #[inline]
    pub fn iter(&self) -> Iter<'_, C> {
        let (entry_metadata, entry_data) = self.allocation.data_slices();
        Iter(RawIter::new(entry_metadata, entry_data))
    }

    pub fn from_iterator<I: IntoIterator<Item = (C::Key, C::Value)>>(
        it: I,
        max_load_factor_percent: u8,
    ) -> Self {
        let it = it.into_iter();

        let known_size = match it.size_hint() {
            (min, Some(max)) => {
                if min == max {
                    Some(max)
                } else {
                    None
                }
            }
            _ => None,
        };

        if let Some(known_size) = known_size {
            let mut table = HashTableOwned::with_capacity(known_size, max_load_factor_percent);

            let initial_slot_count = table.allocation.header().slot_count();

            for (k, v) in it {
                table.insert(&k, &v);
            }

            // duplicates
            assert!(table.len() <= known_size);
            assert_eq!(table.allocation.header().slot_count(), initial_slot_count);

            table
        } else {
            let items: Vec<_> = it.collect();
            Self::from_iterator(items, max_load_factor_percent)
        }
    }

    /// Constructs a [HashTableOwned] from its raw byte representation.
    /// The provided data must have the exact right number of bytes.
    ///
    /// This method has linear time complexity as it needs to make its own
    /// copy of the given data.
    ///
    /// The method will verify the header of the given data and return an
    /// error if the verification fails.
    pub fn from_raw_bytes(data: &[u8]) -> Result<HashTableOwned<C>, Box<dyn std::error::Error>> {
        let data = data.to_owned().into_boxed_slice();
        let allocation = memory_layout::Allocation::from_raw_bytes(data)?;

        Ok(HashTableOwned { allocation })
    }

    #[inline]
    pub unsafe fn from_raw_bytes_unchecked(data: &[u8]) -> HashTableOwned<C> {
        let data = data.to_owned().into_boxed_slice();
        let allocation = memory_layout::Allocation::from_raw_bytes_unchecked(data);

        HashTableOwned { allocation }
    }

    /// Returns the number of items stored in the hash table.
    #[inline]
    pub fn len(&self) -> usize {
        self.allocation.header().item_count()
    }

    #[inline]
    pub fn raw_bytes(&self) -> &[u8] {
        self.allocation.raw_bytes()
    }

    #[inline]
    fn as_raw(&self) -> RawTable<'_, C::EncodedKey, C::EncodedValue, C::H> {
        let (entry_metadata, entry_data) = self.allocation.data_slices();
        RawTable::new(entry_metadata, entry_data)
    }

    #[inline(never)]
    #[cold]
    fn grow(&mut self) {
        let initial_slot_count = self.allocation.header().slot_count();
        let initial_item_count = self.allocation.header().item_count();
        let initial_max_load_factor = self.allocation.header().max_load_factor();

        let mut new_table =
            Self::with_capacity_internal(initial_item_count * 2, initial_max_load_factor);

        // Copy the entries over with the internal `insert_entry()` method,
        // which allows us to do insertions without hashing everything again.
        {
            with_raw_mut(&mut new_table.allocation, |header, mut raw_table| {
                for (_, entry_data) in self.as_raw().iter() {
                    raw_table.insert(entry_data.key, entry_data.value);
                }

                header.set_item_count(initial_item_count);
            });
        }

        *self = new_table;

        assert!(
            self.allocation.header().slot_count() >= 2 * initial_slot_count,
            "Allocation did not grow properly. Slot count is {} but was expected to be \
             at least {}",
            self.allocation.header().slot_count(),
            2 * initial_slot_count
        );
        assert_eq!(self.allocation.header().item_count(), initial_item_count);
        assert_eq!(
            self.allocation.header().max_load_factor(),
            initial_max_load_factor
        );
    }
}

impl<C: Config> std::fmt::Debug for HashTableOwned<C> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let header = self.allocation.header();

        writeln!(
            f,
            "(item_count={}, max_item_count={}, max_load_factor={}%)",
            header.item_count(),
            max_item_count_for(header.slot_count(), header.max_load_factor()),
            header.max_load_factor().to_percent(),
        )?;

        writeln!(f, "{:?}", self.as_raw())
    }
}

/// The [HashTable] type provides a cheap way to construct a non-resizable view
/// of a persisted hash table. If the underlying data storage `D` implements
/// `BorrowMut<[u8]>` then the table can be modified in place.
#[derive(Clone, Copy)]
pub struct HashTable<C: Config, D: Borrow<[u8]>> {
    allocation: memory_layout::Allocation<C, D>,
}

impl<C: Config, D: Borrow<[u8]>> HashTable<C, D> {
    /// Constructs a [HashTable] from its raw byte representation.
    /// The provided data must have the exact right number of bytes.
    ///
    /// This method has constant time complexity and will only verify the header
    /// data of the hash table. It will not copy any data.
    pub fn from_raw_bytes(data: D) -> Result<HashTable<C, D>, Box<dyn std::error::Error>> {
        let allocation = memory_layout::Allocation::from_raw_bytes(data)?;
        Ok(HashTable { allocation })
    }

    /// Constructs a [HashTable] from its raw byte representation without doing
    /// any verification of the underlying data. It is the user's responsibility
    /// to make sure that the underlying data is actually a valid hash table.
    ///
    /// The [HashTable::from_raw_bytes] method provides a safe alternative to this
    /// method.
    #[inline]
    pub unsafe fn from_raw_bytes_unchecked(data: D) -> HashTable<C, D> {
        HashTable {
            allocation: memory_layout::Allocation::from_raw_bytes_unchecked(data),
        }
    }

    #[inline]
    pub fn get(&self, key: &C::Key) -> Option<C::Value> {
        let encoded_key = C::encode_key(key);
        self.as_raw().find(&encoded_key).map(C::decode_value)
    }

    #[inline]
    pub fn contains_key(&self, key: &C::Key) -> bool {
        let encoded_key = C::encode_key(key);
        self.as_raw().find(&encoded_key).is_some()
    }

    #[inline]
    pub fn iter(&self) -> Iter<'_, C> {
        let (entry_metadata, entry_data) = self.allocation.data_slices();
        Iter(RawIter::new(entry_metadata, entry_data))
    }

    /// Returns the number of items stored in the hash table.
    #[inline]
    pub fn len(&self) -> usize {
        self.allocation.header().item_count()
    }

    #[inline]
    pub fn raw_bytes(&self) -> &[u8] {
        self.allocation.raw_bytes()
    }

    #[inline]
    fn as_raw(&self) -> RawTable<'_, C::EncodedKey, C::EncodedValue, C::H> {
        let (entry_metadata, entry_data) = self.allocation.data_slices();
        RawTable::new(entry_metadata, entry_data)
    }
}

impl<C: Config, D: Borrow<[u8]> + BorrowMut<[u8]>> HashTable<C, D> {
    pub fn init_in_place(
        mut data: D,
        max_item_count: usize,
        max_load_factor_percent: u8,
    ) -> Result<HashTable<C, D>, Box<dyn std::error::Error>> {
        let max_load_factor = Factor::from_percent(max_load_factor_percent);
        let byte_count = bytes_needed_internal::<C>(max_item_count, max_load_factor);
        if data.borrow_mut().len() != byte_count {
            return Err(Error(format!(
                "byte slice to initialize has wrong length ({} instead of {})",
                data.borrow_mut().len(),
                byte_count
            )))?;
        }

        let slot_count = slots_needed(max_item_count, max_load_factor);
        let allocation = memory_layout::init_in_place::<C, _>(data, slot_count, 0, max_load_factor);
        Ok(HashTable { allocation })
    }

    /// Inserts the given key-value pair into the table.
    /// Unlike [HashTableOwned::insert] this method cannot grow the underlying table
    /// if there is not enough space for the new item. Instead the call will panic.
    #[inline]
    pub fn insert(&mut self, key: &C::Key, value: &C::Value) -> Option<C::Value> {
        let item_count = self.allocation.header().item_count();
        let max_load_factor = self.allocation.header().max_load_factor();
        let slot_count = self.allocation.header().slot_count();
        // FIXME: This is actually a bit to conservative because it does not account for
        //        cases where an entry is overwritten and thus the item count does not
        //        change.
        assert!(item_count < max_item_count_for(slot_count, max_load_factor));

        let encoded_key = C::encode_key(key);
        let encoded_value = C::encode_value(value);

        with_raw_mut(&mut self.allocation, |header, mut raw_table| {
            if let Some(old_value) = raw_table.insert(encoded_key, encoded_value) {
                Some(C::decode_value(&old_value))
            } else {
                header.set_item_count(item_count + 1);
                None
            }
        })
    }
}

/// Computes the exact number of bytes needed for storing a HashTable with the
/// given max item count and load factor. The result can be used for allocating
/// storage to be passed into [HashTable::init_in_place].
pub fn bytes_needed<C: Config>(max_item_count: usize, max_load_factor_percent: u8) -> usize {
    let max_load_factor = Factor::from_percent(max_load_factor_percent);
    bytes_needed_internal::<C>(max_item_count, max_load_factor)
}

fn bytes_needed_internal<C: Config>(max_item_count: usize, max_load_factor: Factor) -> usize {
    let slot_count = slots_needed(max_item_count, max_load_factor);
    memory_layout::bytes_needed::<C>(slot_count)
}

pub struct Iter<'a, C: Config>(RawIter<'a, C::EncodedKey, C::EncodedValue>);

impl<'a, C: Config> Iterator for Iter<'a, C> {
    type Item = (C::Key, C::Value);

    fn next(&mut self) -> Option<Self::Item> {
        self.0.next().map(|(_, entry)| {
            let key = C::decode_key(&entry.key);
            let value = C::decode_value(&entry.value);

            (key, value)
        })
    }
}

// We use integer math here as not to run into any issues with
// platform-specific floating point math implementation.
fn slots_needed(item_count: usize, max_load_factor: Factor) -> usize {
    // Note: we round up here
    let slots_needed = max_load_factor.apply_inverse(item_count);
    std::cmp::max(
        slots_needed.checked_next_power_of_two().unwrap(),
        REFERENCE_GROUP_SIZE,
    )
}

fn max_item_count_for(slot_count: usize, max_load_factor: Factor) -> usize {
    // Note: we round down here
    max_load_factor.apply(slot_count)
}

#[inline]
fn with_raw_mut<C, M, F, R>(allocation: &mut memory_layout::Allocation<C, M>, f: F) -> R
where
    C: Config,
    M: BorrowMut<[u8]>,
    F: FnOnce(&mut Header, RawTableMut<'_, C::EncodedKey, C::EncodedValue, C::H>) -> R,
{
    allocation.with_mut_parts(|header, entry_metadata, entry_data| {
        f(header, RawTableMut::new(entry_metadata, entry_data))
    })
}

/// This type is used for computing max item counts for a given load factor
/// efficiently. We use integer math here so that things are the same on
/// all platforms and with all compiler settings.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct Factor(pub u16);

impl Factor {
    const BASE: usize = u16::MAX as usize;

    #[inline]
    fn from_percent(percent: u8) -> Factor {
        let percent = percent as usize;
        Factor(((percent * Self::BASE) / 100) as u16)
    }

    fn to_percent(self) -> usize {
        (self.0 as usize * 100) / Self::BASE
    }

    // Note: we round down here
    #[inline]
    fn apply(self, x: usize) -> usize {
        // Let's make sure there's no overflow during the
        // calculation below by doing everything with 128 bits.
        let x = x as u128;
        let factor = self.0 as u128;
        ((x * factor) >> 16) as usize
    }

    // Note: we round up here
    #[inline]
    fn apply_inverse(self, x: usize) -> usize {
        // Let's make sure there's no overflow during the
        // calculation below by doing everything with 128 bits.
        let x = x as u128;
        let factor = self.0 as u128;
        let base = Self::BASE as u128;
        ((base * x + factor - 1) / factor) as usize
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::convert::TryInto;

    enum TestConfig {}

    impl Config for TestConfig {
        type EncodedKey = [u8; 4];
        type EncodedValue = [u8; 4];

        type Key = u32;
        type Value = u32;

        type H = FxHashFn;

        fn encode_key(k: &Self::Key) -> Self::EncodedKey {
            k.to_le_bytes()
        }

        fn encode_value(v: &Self::Value) -> Self::EncodedValue {
            v.to_le_bytes()
        }

        fn decode_key(k: &Self::EncodedKey) -> Self::Key {
            u32::from_le_bytes(k[..].try_into().unwrap())
        }

        fn decode_value(v: &Self::EncodedValue) -> Self::Value {
            u32::from_le_bytes(v[..].try_into().unwrap())
        }
    }

    fn make_test_items(count: usize) -> Vec<(u32, u32)> {
        if count == 0 {
            return vec![];
        }

        let mut items = vec![];

        if count > 1 {
            let steps = (count - 1) as u32;
            let step = u32::MAX / steps;

            for i in 0..steps {
                let x = i * step;
                items.push((x, u32::MAX - x));
            }
        }

        items.push((u32::MAX, 0));

        items.sort();
        items.dedup();
        assert_eq!(items.len(), count);

        items
    }

    #[test]
    fn from_iterator() {
        for count in 0..33 {
            let items = make_test_items(count);
            let table = HashTableOwned::<TestConfig>::from_iterator(items.clone(), 95);
            assert_eq!(table.len(), items.len());

            let mut actual_items: Vec<_> = table.iter().collect();
            actual_items.sort();

            assert_eq!(items, actual_items);
        }
    }

    #[test]
    fn init_in_place() {
        for count in 0..33 {
            let items = make_test_items(count);
            let byte_count = bytes_needed::<TestConfig>(items.len(), 87);
            let data = vec![0u8; byte_count];

            let mut table =
                HashTable::<TestConfig, _>::init_in_place(data, items.len(), 87).unwrap();

            for (i, (k, v)) in items.iter().enumerate() {
                assert_eq!(table.len(), i);
                assert_eq!(table.insert(k, v), None);
                assert_eq!(table.len(), i + 1);

                // Make sure we still can find all items previously inserted.
                for (k, v) in items.iter().take(i) {
                    assert_eq!(table.get(k), Some(*v));
                }
            }

            let mut actual_items: Vec<_> = table.iter().collect();
            actual_items.sort();

            assert_eq!(items, actual_items);
        }
    }

    #[test]
    fn hash_table_at_different_alignments() {
        let items = make_test_items(33);

        let mut serialized = {
            let table: HashTableOwned<TestConfig> =
                HashTableOwned::from_iterator(items.clone(), 95);

            assert_eq!(table.len(), items.len());

            table.raw_bytes().to_owned()
        };

        for alignment_shift in 0..4 {
            let data = &serialized[alignment_shift..];

            let table = HashTable::<TestConfig, _>::from_raw_bytes(data).unwrap();

            assert_eq!(table.len(), items.len());

            for (key, value) in items.iter() {
                assert_eq!(table.get(key), Some(*value));
            }

            serialized.insert(0, 0xFFu8);
        }
    }

    #[test]
    fn load_factor_and_item_count() {
        assert_eq!(
            slots_needed(0, Factor::from_percent(100)),
            REFERENCE_GROUP_SIZE
        );
        assert_eq!(slots_needed(6, Factor::from_percent(60)), 16);
        assert_eq!(slots_needed(5, Factor::from_percent(50)), 16);
        assert_eq!(slots_needed(5, Factor::from_percent(49)), 16);
        assert_eq!(slots_needed(1000, Factor::from_percent(100)), 1024);

        // Factor cannot never be a full 100% because of the rounding involved.
        assert_eq!(max_item_count_for(10, Factor::from_percent(100)), 9);
        assert_eq!(max_item_count_for(10, Factor::from_percent(50)), 4);
        assert_eq!(max_item_count_for(11, Factor::from_percent(50)), 5);
        assert_eq!(max_item_count_for(12, Factor::from_percent(50)), 5);
    }

    #[test]
    fn grow() {
        let items = make_test_items(100);
        let mut table = HashTableOwned::<TestConfig>::with_capacity(10, 87);

        for (key, value) in items.iter() {
            assert_eq!(table.insert(key, value), None);
        }
    }

    #[test]
    fn factor_from_percent() {
        assert_eq!(Factor::from_percent(100), Factor(u16::MAX));
        assert_eq!(Factor::from_percent(0), Factor(0));
        assert_eq!(Factor::from_percent(50), Factor(u16::MAX / 2));
    }

    #[test]
    fn factor_apply() {
        assert_eq!(Factor::from_percent(100).apply(12345), 12344);
        assert_eq!(Factor::from_percent(0).apply(12345), 0);
        assert_eq!(Factor::from_percent(50).apply(66), 32);

        // Make sure we can handle large numbers without overflow
        assert_basically_equal(Factor::from_percent(100).apply(usize::MAX), usize::MAX);
    }

    #[test]
    fn factor_apply_inverse() {
        assert_eq!(Factor::from_percent(100).apply_inverse(12345), 12345);
        assert_eq!(Factor::from_percent(10).apply_inverse(100), 1001);
        assert_eq!(Factor::from_percent(50).apply_inverse(33), 67);

        // // Make sure we can handle large numbers without overflow
        assert_basically_equal(
            Factor::from_percent(100).apply_inverse(usize::MAX),
            usize::MAX,
        );
    }

    fn assert_basically_equal(x: usize, y: usize) {
        let larger_number = std::cmp::max(x, y) as f64;
        let abs_difference = (x as f64 - y as f64).abs();
        let difference_in_percent = (abs_difference / larger_number) * 100.0;

        const MAX_ALLOWED_DIFFERENCE_IN_PERCENT: f64 = 0.01;

        assert!(
            difference_in_percent < MAX_ALLOWED_DIFFERENCE_IN_PERCENT,
            "{} and {} differ by {:.4} percent but the maximally allowed difference \
            is {:.2} percent. Large differences might be caused by integer overflow.",
            x,
            y,
            difference_in_percent,
            MAX_ALLOWED_DIFFERENCE_IN_PERCENT
        );
    }

    mod quickchecks {
        use super::*;
        use crate::raw_table::ByteArray;
        use quickcheck::{Arbitrary, Gen};
        use rustc_hash::FxHashMap;

        #[derive(Copy, Clone, Hash, Eq, PartialEq, Debug)]
        struct Bytes<const BYTE_COUNT: usize>([u8; BYTE_COUNT]);

        impl<const L: usize> Arbitrary for Bytes<L> {
            fn arbitrary(gen: &mut Gen) -> Self {
                let mut xs = [0; L];
                for x in xs.iter_mut() {
                    *x = u8::arbitrary(gen);
                }
                Bytes(xs)
            }
        }

        impl<const L: usize> Default for Bytes<L> {
            fn default() -> Self {
                Bytes([0; L])
            }
        }

        impl<const L: usize> ByteArray for Bytes<L> {
            #[inline(always)]
            fn zeroed() -> Self {
                Bytes([0u8; L])
            }

            #[inline(always)]
            fn as_slice(&self) -> &[u8] {
                &self.0[..]
            }

            #[inline(always)]
            fn equals(&self, other: &Self) -> bool {
                self.as_slice() == other.as_slice()
            }
        }

        macro_rules! mk_quick_tests {
            ($name: ident, $key_len:expr, $value_len:expr) => {
                mod $name {
                    use super::*;
                    use quickcheck::quickcheck;

                    struct Cfg;

                    type Key = Bytes<$key_len>;
                    type Value = Bytes<$value_len>;

                    impl Config for Cfg {
                        type EncodedKey = Key;
                        type EncodedValue = Value;

                        type Key = Key;
                        type Value = Value;

                        type H = FxHashFn;

                        fn encode_key(k: &Self::Key) -> Self::EncodedKey {
                            *k
                        }

                        fn encode_value(v: &Self::Value) -> Self::EncodedValue {
                            *v
                        }

                        fn decode_key(k: &Self::EncodedKey) -> Self::Key {
                            *k
                        }

                        fn decode_value(v: &Self::EncodedValue) -> Self::Value {
                            *v
                        }
                    }

                    fn from_std_hashmap(m: &FxHashMap<Key, Value>) -> HashTableOwned<Cfg> {
                        HashTableOwned::<Cfg>::from_iterator(m.iter().map(|(x, y)| (*x, *y)), 87)
                    }

                    quickcheck! {
                        fn len(xs: FxHashMap<Key, Value>) -> bool {
                            let table = from_std_hashmap(&xs);

                            xs.len() == table.len()
                        }
                    }

                    quickcheck! {
                        fn lookup(xs: FxHashMap<Key, Value>) -> bool {
                            let table = from_std_hashmap(&xs);
                            xs.iter().all(|(k, v)| table.get(k) == Some(*v))
                        }
                    }

                    quickcheck! {
                        fn insert_with_duplicates(xs: Vec<(Key, Value)>) -> bool {
                            let mut reference = FxHashMap::default();
                            let mut table = HashTableOwned::<Cfg>::default();

                            for (k, v) in xs {
                                let expected = reference.insert(k, v);
                                let actual = table.insert(&k, &v);

                                if expected != actual {
                                    return false;
                                }
                            }

                            true
                        }
                    }

                    quickcheck! {
                        fn bytes_deterministic(xs: FxHashMap<Key, Value>) -> bool {
                            // NOTE: We only guarantee this given the exact same
                            //       insertion order.
                            let table0 = from_std_hashmap(&xs);
                            let table1 = from_std_hashmap(&xs);

                            table0.raw_bytes() == table1.raw_bytes()
                        }
                    }

                    quickcheck! {
                        fn from_iterator_vs_manual_insertion(xs: Vec<(Key, Value)>) -> bool {
                            let mut table0 = HashTableOwned::<Cfg>::with_capacity(xs.len(), 87);

                            for (k, v) in xs.iter() {
                                table0.insert(k, v);
                            }

                            let table1 = HashTableOwned::<Cfg>::from_iterator(xs.into_iter(), 87);

                            // Requiring bit for bit equality might be a bit too much in this case,
                            // as long as it works ...
                            table0.raw_bytes() == table1.raw_bytes()
                        }
                    }
                }
            };
        }

        // Test zero sized key and values
        mk_quick_tests!(k0_v0, 0, 0);
        mk_quick_tests!(k1_v0, 1, 0);
        mk_quick_tests!(k2_v0, 2, 0);
        mk_quick_tests!(k3_v0, 3, 0);
        mk_quick_tests!(k4_v0, 4, 0);
        mk_quick_tests!(k8_v0, 8, 0);
        mk_quick_tests!(k15_v0, 15, 0);
        mk_quick_tests!(k16_v0, 16, 0);
        mk_quick_tests!(k17_v0, 17, 0);
        mk_quick_tests!(k63_v0, 63, 0);
        mk_quick_tests!(k64_v0, 64, 0);

        // Test a few different key sizes
        mk_quick_tests!(k2_v4, 2, 4);
        mk_quick_tests!(k4_v4, 4, 4);
        mk_quick_tests!(k8_v4, 8, 4);
        mk_quick_tests!(k17_v4, 17, 4);
        mk_quick_tests!(k20_v4, 20, 4);
        mk_quick_tests!(k64_v4, 64, 4);

        // Test a few different value sizes
        mk_quick_tests!(k16_v1, 16, 1);
        mk_quick_tests!(k16_v2, 16, 2);
        mk_quick_tests!(k16_v3, 16, 3);
        mk_quick_tests!(k16_v4, 16, 4);
        mk_quick_tests!(k16_v8, 16, 8);
        mk_quick_tests!(k16_v16, 16, 16);
        mk_quick_tests!(k16_v17, 16, 17);
    }
}
