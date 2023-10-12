// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#![no_std]
#![deny(
    missing_docs,
    clippy::indexing_slicing,
    clippy::unwrap_used,
    clippy::panic,
    clippy::expect_used
)]
// These features are needed to support no_std + alloc
#![feature(lang_items)]

//! Rust ffi wrapper of ldt_np_adv, can be called from C/C++ Clients

extern crate alloc;

use alloc::boxed::Box;
use core::slice;
use handle_map::get_enc_handle_map;
use ldt_np_adv::{
    build_np_adv_decrypter_from_key_seed, salt_padder, LdtAdvDecryptError, LdtEncrypterXtsAes128,
    LdtNpAdvDecrypterXtsAes128, LegacySalt,
};
use np_hkdf::NpKeySeedHkdf;

use crate::handle_map::get_dec_handle_map;

mod handle_map;

// Pull in the needed deps for std vs no_std
cfg_if::cfg_if! {
    // Test pulls in std which causes duplicate errors
    if #[cfg(any(feature = "std", test, feature = "boringssl", feature = "openssl"))] {
        extern crate std;
    } else {
        // Allow using Box in no_std
        mod no_std;
    }
}

// Fail early for invalid combination of feature flags, we need at least one crypto library specified
#[cfg(all(
    not(feature = "openssl"),
    not(feature = "crypto_provider_rustcrypto"),
    not(feature = "boringssl")
))]
compile_error!("Either the \"openssl\", \"boringssl\"or \"default\" features flag needs to be set in order to specify cryptographic library");

// Need to have one of the crypto provider impls
cfg_if::cfg_if! {
    if #[cfg(feature = "openssl")] {
        use crypto_provider_openssl::Openssl as CryptoProviderImpl;
    } else if #[cfg(feature = "boringssl")]{
        use crypto_provider_boringssl::Boringssl as CryptoProviderImpl;
    } else {
        use crypto_provider_rustcrypto::RustCrypto as CryptoProviderImpl;
    }
}

pub(crate) type LdtAdvDecrypter = LdtNpAdvDecrypterXtsAes128<CryptoProviderImpl>;
pub(crate) type LdtAdvEncrypter = LdtEncrypterXtsAes128<CryptoProviderImpl>;

const SUCCESS: i32 = 0;

#[repr(C)]
struct NpLdtKeySeed {
    bytes: [u8; 32],
}

#[repr(C)]
struct NpMetadataKeyHmac {
    bytes: [u8; 32],
}

#[repr(C)]
struct NpLdtSalt {
    bytes: [u8; 2],
}

#[repr(C)]
struct NpLdtEncryptHandle {
    handle: u64,
}

#[repr(C)]
struct NpLdtDecryptHandle {
    handle: u64,
}

#[no_mangle]
extern "C" fn NpLdtDecryptCreate(
    key_seed: NpLdtKeySeed,
    metadata_key_hmac: NpMetadataKeyHmac,
) -> NpLdtDecryptHandle {
    let cipher = build_np_adv_decrypter_from_key_seed(
        &NpKeySeedHkdf::new(&key_seed.bytes),
        metadata_key_hmac.bytes,
    );
    let handle = get_dec_handle_map().insert::<CryptoProviderImpl>(Box::new(cipher));
    NpLdtDecryptHandle { handle }
}

#[no_mangle]
extern "C" fn NpLdtEncryptCreate(key_seed: NpLdtKeySeed) -> NpLdtEncryptHandle {
    let cipher = LdtAdvEncrypter::new(
        &NpKeySeedHkdf::<CryptoProviderImpl>::new(&key_seed.bytes).legacy_ldt_key(),
    );
    let handle = get_enc_handle_map().insert::<CryptoProviderImpl>(Box::new(cipher));
    NpLdtEncryptHandle { handle }
}

#[no_mangle]
extern "C" fn NpLdtEncryptClose(handle: NpLdtEncryptHandle) -> i32 {
    map_to_error_code(|| {
        get_enc_handle_map()
            .remove(&handle.handle)
            .ok_or(CloseCipherError::InvalidHandle)
            .map(|_| 0)
    })
}

#[no_mangle]
extern "C" fn NpLdtDecryptClose(handle: NpLdtDecryptHandle) -> i32 {
    map_to_error_code(|| {
        get_dec_handle_map()
            .remove(&handle.handle)
            .ok_or(CloseCipherError::InvalidHandle)
            .map(|_| 0)
    })
}

#[no_mangle]
// continue to use LdtAdvDecrypter::encrypt() for now, but we should expose a higher level API
// and get rid of this.
#[allow(deprecated)]
extern "C" fn NpLdtEncrypt(
    handle: NpLdtEncryptHandle,
    buffer: *mut u8,
    buffer_len: usize,
    salt: NpLdtSalt,
) -> i32 {
    map_to_error_code(|| {
        let data = unsafe { slice::from_raw_parts_mut(buffer, buffer_len) };
        let padder = salt_padder::<16, CryptoProviderImpl>(LegacySalt::from(salt.bytes));
        get_enc_handle_map()
            .get(&handle.handle)
            .map(|cipher| {
                cipher.encrypt(data, &padder).map(|_| 0).map_err(|e| match e {
                    ldt::LdtError::InvalidLength(_) => EncryptError::InvalidLength,
                })
            })
            .unwrap_or(Err(EncryptError::InvalidHandle))
    })
}

#[no_mangle]
extern "C" fn NpLdtDecryptAndVerify(
    handle: NpLdtDecryptHandle,
    buffer: *mut u8,
    buffer_len: usize,
    salt: NpLdtSalt,
) -> i32 {
    map_to_error_code(|| {
        let data = unsafe { slice::from_raw_parts_mut(buffer, buffer_len) };
        let padder = salt_padder::<16, CryptoProviderImpl>(LegacySalt::from(salt.bytes));

        get_dec_handle_map()
            .get(&handle.handle)
            .map(|cipher| {
                cipher
                    .decrypt_and_verify(data, &padder)
                    .map_err(|e| match e {
                        LdtAdvDecryptError::InvalidLength(_) => DecryptError::InvalidLength,
                        LdtAdvDecryptError::MacMismatch => DecryptError::HmacDoesntMatch,
                    })
                    .map(|plaintext| {
                        data.copy_from_slice(plaintext.as_slice());
                        SUCCESS
                    })
            })
            .unwrap_or(Err(DecryptError::InvalidHandle))
    })
}

fn map_to_error_code<T, E: ErrorEnum<T>, F: Fn() -> Result<T, E>>(f: F) -> T {
    f().unwrap_or_else(|e| e.to_error_code())
}

trait ErrorEnum<C> {
    fn to_error_code(&self) -> C;
}

enum CloseCipherError {
    InvalidHandle,
}

impl ErrorEnum<i32> for CloseCipherError {
    fn to_error_code(&self) -> i32 {
        match self {
            Self::InvalidHandle => -3,
        }
    }
}

enum EncryptError {
    InvalidLength,
    InvalidHandle,
}

impl ErrorEnum<i32> for EncryptError {
    fn to_error_code(&self) -> i32 {
        match self {
            Self::InvalidLength => -1,
            Self::InvalidHandle => -3,
        }
    }
}

enum DecryptError {
    HmacDoesntMatch,
    InvalidLength,
    InvalidHandle,
}

impl ErrorEnum<i32> for DecryptError {
    fn to_error_code(&self) -> i32 {
        match self {
            Self::InvalidLength => -1,
            Self::HmacDoesntMatch => -2,
            Self::InvalidHandle => -3,
        }
    }
}
