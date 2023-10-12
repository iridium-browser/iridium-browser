#![feature(test)]

extern crate test;

use odht::{Config, FxHashFn, HashTable, HashTableOwned};
use rustc_hash::FxHashMap;

#[repr(C)]
#[derive(Copy, Clone, Hash, Eq, PartialEq)]
struct TestKey(u64, u64);

struct FxConfig;

impl Config for FxConfig {
    type Key = TestKey;
    type Value = u32;

    type EncodedKey = [u8; 16];
    type EncodedValue = [u8; 4];

    type H = FxHashFn;

    #[inline]
    fn encode_key(k: &Self::Key) -> Self::EncodedKey {
        let mut result = [0u8; 16];

        result[0..8].copy_from_slice(&k.0.to_le_bytes());
        result[8..16].copy_from_slice(&k.1.to_le_bytes());

        result
    }

    #[inline]
    fn encode_value(v: &Self::Value) -> Self::EncodedValue {
        v.to_le_bytes()
    }

    #[inline]
    fn decode_key(_k: &Self::EncodedKey) -> Self::Key {
        panic!()
    }

    #[inline]
    fn decode_value(v: &Self::EncodedValue) -> Self::Value {
        u32::from_le_bytes(*v)
    }
}

fn index_contained(i: usize) -> bool {
    i % 10 != 3
}

// Call functions being benchmark through an #[inline(never)] wrapper
// so that we get a clear inlining barrier to look at in profilers.
#[inline(never)]
fn get_value<F: Fn(&TestKey) -> Option<u32>>(f: &F, key: &TestKey) -> Option<u32> {
    f(key)
}

#[inline(never)]
fn insert_value<F: FnMut(TestKey, u32) -> Option<u32>>(
    f: &mut F,
    key: TestKey,
    value: u32,
) -> Option<u32> {
    f(key, value)
}

fn generate_hash_table(
    test_data: &[(TestKey, u32)],
    load_factor_percent: u8,
) -> HashTableOwned<FxConfig> {
    let values: Vec<_> = test_data
        .iter()
        .enumerate()
        .filter(|&(i, _)| index_contained(i))
        .map(|(_, x)| x)
        .collect();

    let mut table = HashTableOwned::with_capacity(values.len(), load_factor_percent);

    for (key, value) in values {
        table.insert(key, value);
    }

    table
}

fn generate_std_hash_table(test_data: &[(TestKey, u32)]) -> FxHashMap<TestKey, u32> {
    let mut table = FxHashMap::default();

    let values: Vec<_> = test_data
        .iter()
        .enumerate()
        .filter(|&(i, _)| index_contained(i))
        .map(|(_, x)| *x)
        .collect();

    for (key, value) in values {
        table.insert(key, value);
    }

    table
}

fn generate_test_data(num_values: usize) -> Vec<(TestKey, u32)> {
    use rand::prelude::*;

    (0..num_values)
        .map(|_| (TestKey(random(), random()), random()))
        .collect()
}

const LOOKUP_ITERATIONS: usize = 10;
const INSERT_ITERATIONS: usize = 10;

fn bench_odht_fx_lookup(b: &mut test::Bencher, num_values: usize, load_factor_percent: u8) {
    let test_data = crate::generate_test_data(num_values);
    let table = crate::generate_hash_table(&test_data, load_factor_percent);

    let mut serialized = table.raw_bytes().to_owned();

    // Shift the data so we mess up alignment. We want to test the table under
    // realistic conditions where we cannot expect any specific alignment.
    serialized.insert(0, 0xFFu8);

    let table = HashTable::<FxConfig, _>::from_raw_bytes(&serialized[1..]).unwrap();

    let get = |key: &TestKey| table.get(key);

    b.iter(|| {
        for _ in 0..LOOKUP_ITERATIONS {
            for (index, &(key, value)) in test_data.iter().enumerate() {
                if index_contained(index) {
                    assert!(get_value(&get, &key) == Some(value));
                } else {
                    assert!(get_value(&get, &key).is_none());
                }
            }
        }
    })
}

fn bench_odht_fx_insert(b: &mut test::Bencher, num_values: usize, load_factor_percent: u8) {
    let test_data = crate::generate_test_data(num_values);

    b.iter(|| {
        for _ in 0..INSERT_ITERATIONS {
            let mut table = HashTableOwned::<FxConfig>::with_capacity(10, load_factor_percent);

            let mut insert = |key: TestKey, value: u32| table.insert(&key, &value);

            for (key, value) in test_data.iter() {
                assert!(insert_value(&mut insert, *key, *value).is_none());
            }
        }
    })
}

fn bench_std_fx_lookup(b: &mut test::Bencher, num_values: usize) {
    let test_data = crate::generate_test_data(num_values);
    let table = crate::generate_std_hash_table(&test_data);

    let get = |key: &TestKey| table.get(key).cloned();

    b.iter(|| {
        for _ in 0..LOOKUP_ITERATIONS {
            for (index, &(key, value)) in test_data.iter().enumerate() {
                if index_contained(index) {
                    assert!(get_value(&get, &key) == Some(value));
                } else {
                    assert!(get_value(&get, &key).is_none());
                }
            }
        }
    })
}

fn bench_std_fx_insert(b: &mut test::Bencher, num_values: usize) {
    let test_data = crate::generate_test_data(num_values);

    b.iter(|| {
        for _ in 0..INSERT_ITERATIONS {
            let mut table = FxHashMap::default();

            let mut insert = |key: TestKey, value: u32| -> Option<u32> { table.insert(key, value) };

            for (key, value) in test_data.iter() {
                assert!(insert_value(&mut insert, *key, *value).is_none());
            }
        }
    })
}

macro_rules! bench {
    ($name:ident, $num_values:expr) => {
        mod $name {
            #[bench]
            fn lookup_odht_fx_load_87(b: &mut test::Bencher) {
                crate::bench_odht_fx_lookup(b, $num_values, 87);
            }

            #[bench]
            fn insert_odht_fx_load_87(b: &mut test::Bencher) {
                crate::bench_odht_fx_insert(b, $num_values, 87);
            }

            #[bench]
            fn lookup_std_fx(b: &mut test::Bencher) {
                crate::bench_std_fx_lookup(b, $num_values);
            }

            #[bench]
            fn insert_std_fx(b: &mut test::Bencher) {
                crate::bench_std_fx_insert(b, $num_values);
            }
        }
    };
}

// These numbers are chosen so that we get an actual load factor of ~87%
// taking into account that slot counts are always rounded up to the next
// power of two.
bench!(____n13, 13);
bench!(____n55, 55);
bench!(___n444, 444);
bench!(__n3550, 3550);
bench!(_n57000, 57000);
