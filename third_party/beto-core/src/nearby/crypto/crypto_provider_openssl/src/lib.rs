#![forbid(unsafe_code)]
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
#![deny(missing_docs, clippy::indexing_slicing, clippy::panic)]

//! Crate which provides impls for CryptoProvider backed by openssl

use cfg_if::cfg_if;
pub use openssl;
use openssl::hash::MessageDigest;
use openssl::md::MdRef;
use openssl::rand::rand_bytes;

use crypto_provider::CryptoRng;

/// Contains the openssl backed AES implementations for CryptoProvider
mod aes;
/// Contains the openssl backed ed25519 impl for key generation, verification, and signing
mod ed25519;

cfg_if! {
    if #[cfg(feature = "boringssl")] {
        /// Contains the boringssl backed hkdf impl for CryptoProvider
        mod hkdf_boringssl;
        use hkdf_boringssl as hkdf;
        /// Contains the boringssl backed hmac impl for CryptoProvider
        mod hmac_boringssl;
        use hmac_boringssl as hmac;
    } else {
        /// Contains the openssl backed hkdf impl for CryptoProvider
        mod hkdf_openssl;
        use hkdf_openssl as hkdf;
        /// Contains the openssl backed hmac impl for CryptoProvider
        mod hmac_openssl;
        use hmac_openssl as hmac;
    }
}
/// Contains the openssl backed NIST-P256 impl for CryptoProvider
mod p256;
/// Contains the openssl backed SHA2 impl for CryptoProvider
mod sha2;
/// Contains the openssl backed X25519 impl for CryptoProvider
mod x25519;

/// The the openssl backed struct which implements CryptoProvider
#[derive(Default, Clone, Debug, PartialEq, Eq)]
pub struct Openssl;

/// The trait which defines a has used in openssl crypto operations
pub trait OpenSslHash {
    /// gets the message digest algorithm to be used by the hmac implementation
    fn get_digest() -> MessageDigest;
    /// gets a reference to a message digest algorithm to be used by the hkdf implementation
    fn get_md() -> &'static MdRef;
}

impl crypto_provider::CryptoProvider for Openssl {
    type HkdfSha256 = hkdf::Hkdf<sha2::OpenSslSha256>;
    type HmacSha256 = hmac::Hmac<sha2::OpenSslSha256>;
    type HkdfSha512 = hkdf::Hkdf<sha2::OpenSslSha512>;
    type HmacSha512 = hmac::Hmac<sha2::OpenSslSha512>;
    type AesCbcPkcs7Padded = aes::OpenSslAesCbcPkcs7;
    type X25519 = x25519::X25519Ecdh;
    type P256 = p256::P256Ecdh;
    type Sha256 = sha2::OpenSslSha256;
    type Sha512 = sha2::OpenSslSha512;
    type Aes128 = aes::Aes128;
    type Aes256 = aes::Aes256;
    type AesCtr128 = aes::OpenSslAesCtr128;
    type AesCtr256 = aes::OpenSslAesCtr256;
    type Ed25519 = ed25519::Ed25519;
    type Aes128GcmSiv = crypto_provider_stubs::Aes128Stubs;
    type Aes256GcmSiv = crypto_provider_stubs::Aes256Stubs;
    type CryptoRng = OpenSslRng;

    fn constant_time_eq(a: &[u8], b: &[u8]) -> bool {
        a.len() == b.len() && openssl::memcmp::eq(a, b)
    }
}

/// OpenSSL implemented random number generator
pub struct OpenSslRng;

impl CryptoRng for OpenSslRng {
    fn new() -> Self {
        OpenSslRng {}
    }

    fn next_u64(&mut self) -> u64 {
        let mut buf = [0; 8];
        rand_bytes(&mut buf).unwrap();
        u64::from_be_bytes(buf)
    }

    fn fill(&mut self, dest: &mut [u8]) {
        rand_bytes(dest).expect("Error in generating random bytes")
    }
}

#[cfg(test)]
mod tests {
    use core::marker::PhantomData;

    use crypto_provider_test::sha2::*;
    use crypto_provider_test::*;

    use crate::Openssl;

    #[apply(sha2_test_cases)]
    fn sha2_tests(testcase: CryptoProviderTestCase<Openssl>) {
        testcase(PhantomData);
    }

    #[apply(constant_time_eq_test_cases)]
    fn constant_time_eq_tests(testcase: CryptoProviderTestCase<Openssl>) {
        testcase(PhantomData);
    }
}
