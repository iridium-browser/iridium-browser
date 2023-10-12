use crate::HashFn;
use std::convert::TryInto;

/// A hash function that simply takes the last 4 bytes of a given key as the
/// hash value.
#[derive(Eq, PartialEq)]
pub struct UnHashFn;

impl HashFn for UnHashFn {
    #[inline]
    fn hash(bytes: &[u8]) -> u32 {
        let len = bytes.len();
        u32::from_le_bytes(bytes[len - 4..].try_into().unwrap())
    }
}
