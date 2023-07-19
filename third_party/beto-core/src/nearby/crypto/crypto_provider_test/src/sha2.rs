// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

extern crate alloc;
extern crate std;
pub use crate::prelude::*;
use crate::CryptoProvider;
use alloc::vec::Vec;
use core::{marker::PhantomData, str::FromStr};
use crypto_provider::sha2::{Sha256, Sha512};
use hex::FromHex;
pub use hex_literal::hex;
use rstest_reuse::template;

/// Test vectors from SHA256ShortMsg.rsp in
/// <https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing#shavs>
pub fn sha256_cavp_short_vector_test<C: CryptoProvider>(_marker: PhantomData<C>) {
    sha256_cavp_vector_test::<C>(include_str!("testdata/SHA256ShortMsg.rsp"));
}

/// Test vectors from SHA256LongMsg.rsp in
/// <https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing#shavs>
pub fn sha256_cavp_long_vector_test<C: CryptoProvider>(_marker: PhantomData<C>) {
    sha256_cavp_vector_test::<C>(include_str!("testdata/SHA256LongMsg.rsp"));
}

/// Test vectors from SHA512ShortMsg.rsp in
/// <https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing#shavs>
pub fn sha512_cavp_short_vector_test<C: CryptoProvider>(_marker: PhantomData<C>) {
    sha512_cavp_vector_test::<C>(include_str!("testdata/SHA512ShortMsg.rsp"));
}

/// Test vectors from SHA512LongMsg.rsp in
/// <https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing#shavs>
pub fn sha512_cavp_long_vector_test<C: CryptoProvider>(_marker: PhantomData<C>) {
    sha512_cavp_vector_test::<C>(include_str!("testdata/SHA512LongMsg.rsp"));
}

/// Test vectors an rsp file in
/// <https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing#shavs>
fn sha256_cavp_vector_test<C: CryptoProvider>(cavp_file_contents: &str) {
    sha_cavp_vector_test(<C::Sha256 as Sha256>::sha256, cavp_file_contents)
}

/// Test vectors an rsp file in
/// <https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing#shavs>
fn sha512_cavp_vector_test<C: CryptoProvider>(cavp_file_contents: &str) {
    sha_cavp_vector_test(<C::Sha512 as Sha512>::sha512, cavp_file_contents)
}

fn sha_cavp_vector_test<const N: usize>(
    hash_func: impl Fn(&[u8]) -> [u8; N],
    cavp_file_contents: &str,
) {
    let test_cases = cavp_file_contents.split("\n\n").filter_map(|chunk| {
        let mut len: Option<usize> = None;
        let mut msg: Option<Vec<u8>> = None;
        let mut md: Option<Vec<u8>> = None;
        for line in chunk.split('\n') {
            if line.starts_with('#') || line.is_empty() || line == std::format!("[L = {N}]") {
                continue;
            } else if let Some(len_str) = line.strip_prefix("Len = ") {
                len = Some(FromStr::from_str(len_str).unwrap());
            } else if let Some(hex_str) = line.strip_prefix("Msg = ") {
                msg = Some(Vec::<u8>::from_hex(hex_str).unwrap());
            } else if let Some(hex_str) = line.strip_prefix("MD = ") {
                md = Some(Vec::<u8>::from_hex(hex_str).unwrap());
            } else {
                panic!("Unexpected line in test file: `{}`", line);
            }
        }
        if let (Some(len), Some(msg), Some(md)) = (len, msg, md) {
            Some((len, msg, md))
        } else {
            None
        }
    });
    for (len, mut msg, md) in test_cases {
        if len == 0 {
            // Truncate len = 0, since the test file has "Msg = 00" in there that should be
            // ignored.
            msg.truncate(0);
        }
        assert_eq!(msg.len(), len / 8);
        let md_arr: [u8; N] = md.try_into().unwrap();
        assert_eq!(hash_func(&msg), md_arr);
    }
}

/// Generates the test cases to validate the SHA2 implementation.
/// For example, to test `MyCryptoProvider`:
///
/// ```
/// use crypto_provider::sha2::testing::*;
///
/// mod tests {
///     #[apply(sha2_test_cases)]
///     fn sha2_tests(testcase: CryptoProviderTestCase<MyCryptoProvider>) {
///         testcase(PhantomData::<MyCryptoProvider>);
///     }
/// }
/// ```
#[template]
#[export]
#[rstest]
#[case::sha256_cavp_short_vector(sha256_cavp_short_vector_test)]
#[case::sha256_cavp_long_vector(sha256_cavp_long_vector_test)]
fn sha2_test_cases<C: CryptoProvider>(#[case] testcase: CryptoProviderTestCase<C>) {}
