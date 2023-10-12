use crate::HashFn;
use std::convert::TryInto;

/// Implements the hash function from the rustc-hash crate.
#[derive(Eq, PartialEq)]
pub struct FxHashFn;

impl HashFn for FxHashFn {
    // This function is marked as #[inline] because that allows LLVM to know the
    // actual size of `bytes` and thus eliminate all unneeded branches below.
    #[inline]
    fn hash(mut bytes: &[u8]) -> u32 {
        let mut hash_value = 0;

        while bytes.len() >= 8 {
            hash_value = add_to_hash(hash_value, read_u64(bytes));
            bytes = &bytes[8..];
        }

        if bytes.len() >= 4 {
            hash_value = add_to_hash(
                hash_value,
                u32::from_le_bytes(bytes[..4].try_into().unwrap()) as u64,
            );
            bytes = &bytes[4..];
        }

        if bytes.len() >= 2 {
            hash_value = add_to_hash(
                hash_value,
                u16::from_le_bytes(bytes[..2].try_into().unwrap()) as u64,
            );
            bytes = &bytes[2..];
        }

        if bytes.len() >= 1 {
            hash_value = add_to_hash(hash_value, bytes[0] as u64);
        }

        return hash_value as u32;

        #[inline]
        fn add_to_hash(current_hash: u64, value: u64) -> u64 {
            use std::ops::BitXor;
            current_hash
                .rotate_left(5)
                .bitxor(value)
                // This constant is part of FxHash's definition:
                // https://github.com/rust-lang/rustc-hash/blob/5e09ea0a1/src/lib.rs#L67
                .wrapping_mul(0x517cc1b727220a95)
        }

        #[inline]
        fn read_u64(bytes: &[u8]) -> u64 {
            u64::from_le_bytes(bytes[..8].try_into().unwrap())
        }
    }
}
