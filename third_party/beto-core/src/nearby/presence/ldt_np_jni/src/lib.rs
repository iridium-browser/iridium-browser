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

//! JNI adapter for LDT.
//!
//! Helpful resources:
//! - <https://developer.ibm.com/articles/j-jni>
//! - <https://developer.android.com/training/articles/perf-jni>
//! - <https://www.iitk.ac.in/esc101/05Aug/tutorial/native1.1/index.html>
//! - <https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html>

#![no_std]
#![deny(missing_docs)]

// Allow using Box in no_std
extern crate alloc;

use alloc::boxed::Box;

use jni::objects::JByteArray;
use jni::{
    objects::JClass,
    sys::{jbyte, jbyteArray, jchar, jint, jlong},
    JNIEnv,
};

use ldt::XorPadder;
use ldt_np_adv::{LdtAdvDecryptError, LdtEncrypterXtsAes128, LdtNpAdvDecrypterXtsAes128};
use np_hkdf::NpKeySeedHkdf;

use crypto_provider_default::CryptoProviderImpl;

/// Length limits per LDT
const MIN_DATA_LEN: usize = crypto_provider::aes::BLOCK_SIZE;
const MAX_DATA_LEN: usize = crypto_provider::aes::BLOCK_SIZE * 2 - 1;

/// Error return value for creating handles
const CREATE_ERROR: jlong = 0;

// TODO: don't allow panics to cross FFI boundary
// TODO: JNI null checks? (only if jni crate isn't doing them already).

// TODO: split this into separate APIs for encrypt and decrypt
struct Ldt {
    ldt_enc: LdtEncrypterXtsAes128<CryptoProviderImpl>,
    ldt_dec: LdtNpAdvDecrypterXtsAes128<CryptoProviderImpl>,
}

/// Create an LDT cipher.
///
/// Returns 0 for error, or the pointer as a jlong/i64.
/// Safety: We know the key pointer is safe as it is coming directly from the JVM.
#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "system" fn Java_com_google_android_gms_nearby_presence_hazmat_LdtNpJni_createLdtCipher(
    env: JNIEnv,
    _class: JClass,
    key_seed: jbyteArray,
    metadata_key_hmac_tag: jbyteArray,
) -> jlong {
    env.get_array_length(unsafe { &JByteArray::from_raw(key_seed) })
        .map_err(|_| CREATE_ERROR)
        // check length
        .and_then(|len| if len as usize != 32 { Err(CREATE_ERROR) } else { Ok(len) })
        // extract u8 array
        .and_then(|len| {
            let mut jbyte_buf = [jbyte::default(); 32];
            env.get_byte_array_region(
                unsafe { &JByteArray::from_raw(key_seed) },
                0,
                &mut jbyte_buf[..],
            )
            .map_err(|_| CREATE_ERROR)
            .map(|_| (len, jbyte_array_to_u8_array(jbyte_buf)))
        })
        // initialize ldt -- we already know the key is the right length
        .and_then(|(_len, key_seed_buf)| {
            let hkdf_key_seed = NpKeySeedHkdf::new(&key_seed_buf);
            let ldt_enc = ldt_np_adv::LdtEncrypterXtsAes128::<CryptoProviderImpl>::new(
                &hkdf_key_seed.legacy_ldt_key(),
            );

            let mut tag_buff = [jbyte::default(); 32];
            let tag = env
                .get_byte_array_region(
                    unsafe { &JByteArray::from_raw(metadata_key_hmac_tag) },
                    0,
                    &mut tag_buff[..],
                )
                .map_err(|_| CREATE_ERROR)
                .map(|_| jbyte_array_to_u8_array(tag_buff))
                .unwrap();
            // TODO: Error handling

            let ldt_dec = ldt_np_adv::build_np_adv_decrypter_from_key_seed::<CryptoProviderImpl>(
                &hkdf_key_seed,
                tag,
            );
            box_to_handle(Ldt { ldt_enc, ldt_dec }).map_err(|_| CREATE_ERROR)
        })
        .unwrap_or_else(|e| e)
}

/// Close an LDT cipher.
#[no_mangle]
pub extern "system" fn Java_com_google_android_gms_nearby_presence_hazmat_LdtNpJni_closeLdtCipher(
    _env: JNIEnv,
    _class: JClass,
    ldt_handle: jlong,
) -> jint {
    // create the box, let it be dropped
    let _ = boxed_from_handle::<Ldt>(ldt_handle);
    // success -- are there any meaningful error condtions we can even detect?
    0
}

/// Encrypt a buffer in place.
/// Safety: We know the data jArray pointer is safe because it is coming directly from the JVM.
#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "system" fn Java_com_google_android_gms_nearby_presence_hazmat_LdtNpJni_encrypt(
    env: JNIEnv,
    _class: JClass,
    ldt_handle: jlong,
    salt: jchar,
    data: jbyteArray,
) -> jint {
    jbyte_cipher_data_as_u8_array(&env, data)
        .and_then(|(len, mut data_u8)| {
            with_handle::<Ldt, _, _>(ldt_handle, |ldt| {
                ldt.ldt_enc.encrypt(&mut data_u8[..len], &expand_np_salt_to_padder(salt)).map_err(
                    |err| match err {
                        ldt::LdtError::InvalidLength(_) => CipherOpError::DataLen,
                    },
                )?;
                env.set_byte_array_region(
                    unsafe { &JByteArray::from_raw(data) },
                    0,
                    &u8_slice_to_jbyte_array(data_u8)[..len],
                )
                .map_err(|_| CipherOpError::JniOp)
                .map(|_| 0) // success
            })
        })
        .unwrap_or_else(|e| e.to_jni_error_code())
}

/// Decrypt a buffer in place.
/// Safety: We know the data pointer is safe because it is coming directly from the JVM.
#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "system" fn Java_com_google_android_gms_nearby_presence_hazmat_LdtNpJni_decrypt_1and_1verify(
    env: JNIEnv,
    _class: JClass,
    ldt_handle: jlong,
    salt: jchar,
    data: jbyteArray,
) -> jint {
    jbyte_cipher_data_as_u8_array(&env, data)
        .and_then(|(len, mut data_u8)| {
            with_handle::<Ldt, _, _>(ldt_handle, |ldt| {
                let result = ldt
                    .ldt_dec
                    .decrypt_and_verify(&data_u8[..len], &expand_np_salt_to_padder(salt))
                    .map_err(|err| match err {
                        LdtAdvDecryptError::InvalidLength(_) => CipherOpError::DataLen,
                        LdtAdvDecryptError::MacMismatch => CipherOpError::MacMisMatch,
                    })?;
                data_u8[..result.len()].copy_from_slice(result.as_slice());
                env.set_byte_array_region(
                    unsafe { &JByteArray::from_raw(data) },
                    0,
                    &u8_slice_to_jbyte_array(data_u8)[..len],
                )
                .map_err(|_| CipherOpError::JniOp)
                .map(|_| 0) // success
            })
        })
        .unwrap_or_else(|e| e.to_jni_error_code())
}

/// Reconstruct a `Box<T>` from `handle`, and invoke `f` with the resulting `&T`.
///
/// The `Box<T>` is leaked after invoking `block` rather than dropped so that the handle can be used
/// again.
///
/// Returns the result of evaluating `f`.
fn with_handle<T, U, F: FnMut(&T) -> U>(handle: jlong, mut f: F) -> U {
    let boxed = boxed_from_handle(handle);
    let ret = f(&boxed);

    // don't consume the box -- need to keep the handle alive
    Box::leak(boxed);

    ret
}

/// Reconstruct a `Box<T>` from `handle`.
///
/// `handle` must be an aligned, non-null `jlong` representation of a pointer produced from
/// `Box::into_raw` that has not yet been deallocated.
fn boxed_from_handle<T>(handle: jlong) -> Box<T> {
    // on 32-bit systems, truncate i64 to low 32 bits (which should be the only bits that were set
    // when the jlong handle was created).
    let handle_usize = handle as usize;
    // convert pointer-sized integer to pointer
    unsafe { Box::from_raw(handle_usize as *mut _) }
}

/// Constructs a `Box<T>`, leaks a pointer to it, and converts the pointer to `jlong`.
///
/// If the pointer can't fit, `Err` is returned.
fn box_to_handle<T>(thing: T) -> Result<jlong, ()> {
    // Box::new heap allocates space for the thing
    // Box::into_raw intentionally leaks into an aligned, non-null pointer
    let pointer = Box::into_raw(Box::new(thing));
    // As a best practice, cast from pointer to usize because usize is always pointer sized, so the
    // cast is easy to reason about.
    // https://doc.rust-lang.org/reference/expressions/operator-expr.html#pointer-to-address-cast
    let ptr_usize = pointer as usize;
    // Fallible conversion into a u64 -- eventually 128 bit pointer types will fail here.
    // Assuming it fits, integer cast should be either no conversion or zero extension.
    ptr_usize
        .try_into()
        .map_err(|_| {
            // resuscitate the Box so that its drop can run, otherwise we would leak on error
            unsafe {
                let _ = Box::from_raw(pointer);
            }
        })
        // Now that we know the pointer fits in 64 bits, can cast u64 to i64/jlong.
        .map(|ptr_64: u64| ptr_64 as jlong)
}

/// Extract data suitable for Ldt128 cipher ops from a JNI jbyteArray.
///
/// Returns `(data len in buffer, buffer)`, or `Err` if any JNI ops fail.
fn jbyte_cipher_data_as_u8_array(
    env: &JNIEnv,
    cipher_data: jbyteArray,
) -> Result<(usize, [u8; MAX_DATA_LEN]), CipherOpError> {
    let data_len = env
        .get_array_length(unsafe { &JByteArray::from_raw(cipher_data) })
        .map_err(|_| CipherOpError::JniOp)? as usize;
    if !(MIN_DATA_LEN..=MAX_DATA_LEN).contains(&data_len) {
        return Err(CipherOpError::DataLen);
    }

    let mut buf = [jbyte::default(); MAX_DATA_LEN];
    env.get_byte_array_region(
        unsafe { &JByteArray::from_raw(cipher_data) },
        0,
        &mut buf[0..data_len],
    )
    .map_err(|_| CipherOpError::JniOp)?;

    Ok((data_len, jbyte_array_to_u8_array(buf)))
}

/// Convert a jbyte array to a u8 array
fn jbyte_array_to_u8_array<const N: usize>(src: [jbyte; N]) -> [u8; N] {
    let mut dest = [0_u8; N];
    for i in 0..N {
        // numeric cast doesn't alter bits, which is what we want
        // https://doc.rust-lang.org/reference/expressions/operator-expr.html#semantics
        dest[i] = src[i] as u8;
    }
    dest
}

fn u8_slice_to_jbyte_array<const N: usize>(src: [u8; N]) -> [jbyte; N] {
    let mut dest = [0_i8; N];
    for i in 0..N {
        // numeric cast doesn't alter bits, which is what we want
        // https://doc.rust-lang.org/reference/expressions/operator-expr.html#semantics
        dest[i] = src[i] as jbyte;
    }
    dest
}

/// Expand the NP salt to the size needed to be an LDT XorPadder.
///
/// Returns a XorPadder containing the HKDF of the salt.
fn expand_np_salt_to_padder(np_salt: jchar) -> XorPadder<{ crypto_provider::aes::BLOCK_SIZE }> {
    let salt_bytes = np_salt.to_be_bytes();
    ldt_np_adv::salt_padder::<16, CryptoProviderImpl>(salt_bytes.into())
}

#[derive(Debug)]
enum CipherOpError {
    /// The mac did not match the provided tag
    MacMisMatch,
    /// Data is the wrong length
    DataLen,
    /// JNI op failed
    JniOp,
}

impl CipherOpError {
    /// Returns an error code suitable for returning from Ldt encrypt/decrypt JNI calls.
    fn to_jni_error_code(&self) -> jint {
        match self {
            CipherOpError::DataLen => -1,
            CipherOpError::MacMisMatch => -2,
            CipherOpError::JniOp => -3,
        }
    }
}
