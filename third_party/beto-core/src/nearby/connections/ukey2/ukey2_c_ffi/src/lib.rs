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

use std::collections::HashMap;
use std::ptr::null_mut;

use lazy_static::lazy_static;
use rand::Rng;
use rand_chacha::rand_core::SeedableRng;
use rand_chacha::ChaCha20Rng;
use raw_parts::RawParts;
use spin::Mutex;

use ukey2_connections::{
    D2DConnectionContextV1, D2DHandshakeContext, HandshakeImplementation,
    InitiatorD2DHandshakeContext, ServerD2DHandshakeContext,
};

cfg_if::cfg_if! {
    if #[cfg(feature = "rustcrypto")] {
        use crypto_provider_rustcrypto::RustCrypto as CryptoProvider;
    } else {
        use crypto_provider_openssl::Openssl as CryptoProvider;
    }
}
#[repr(C)]
pub struct RustFFIByteArray {
    ptr: *mut u8,
    len: usize,
    cap: usize,
}

#[repr(C)]
pub struct CFFIByteArray {
    ptr: *mut u8,
    len: usize,
}

type D2DBox = Box<dyn D2DHandshakeContext>;
type ConnectionBox = Box<D2DConnectionContextV1>;

lazy_static! {
    static ref HANDLE_MAPPING: Mutex<HashMap<u64, D2DBox>> = Mutex::new(HashMap::new());
    static ref CONNECTION_HANDLE_MAPPING: Mutex<HashMap<u64, ConnectionBox>> =
        Mutex::new(HashMap::new());
    static ref RNG: Mutex<ChaCha20Rng> = Mutex::new(ChaCha20Rng::from_entropy());
}

fn generate_handle() -> u64 {
    RNG.lock().gen()
}

fn insert_gen_handle(item: D2DBox) -> u64 {
    let handle = generate_handle();
    HANDLE_MAPPING.lock().insert(handle, item);
    handle
}

fn insert_conn_gen_handle(item: ConnectionBox) -> u64 {
    let handle = generate_handle();
    CONNECTION_HANDLE_MAPPING.lock().insert(handle, item);
    handle
}

// Utilities
/// This function deallocates FFIByteArray instances allocated from Rust only.
/// NOTE: Any FFIByteArray instances deallocated by this function will no longer be in a guaranteed
/// usable state.
///
/// # Safety
/// The array must have been allocated by a Rust function with the Rust allocator, e.g.
/// [get_next_handshake_message].
#[no_mangle]
pub unsafe extern "C" fn rust_dealloc_ffi_byte_array(arr: RustFFIByteArray) {
    if !arr.ptr.is_null() {
        let raw_parts = RawParts {
            ptr: arr.ptr,
            length: arr.len,
            capacity: arr.cap,
        };
        let _ = RawParts::into_vec(raw_parts);
    }
}

// Common functions
#[no_mangle]
pub extern "C" fn is_handshake_complete(handle: u64) -> bool {
    HANDLE_MAPPING
        .lock()
        .get(&handle)
        .map_or(false, |ctx| ctx.is_handshake_complete())
}

#[no_mangle]
pub extern "C" fn get_next_handshake_message(handle: u64) -> RustFFIByteArray {
    // TODO: error handling
    let opt_msg = HANDLE_MAPPING
        .lock()
        .get(&handle)
        .and_then(|c| c.get_next_handshake_message());
    if let Some(msg) = opt_msg {
        let RawParts {
            ptr,
            length,
            capacity,
        } = RawParts::from_vec(msg);
        RustFFIByteArray {
            ptr,
            len: length,
            cap: capacity,
        }
    } else {
        RustFFIByteArray {
            ptr: null_mut(),
            len: usize::MAX,
            cap: usize::MAX,
        }
    }
}

/// # Safety
/// We treat msg as data, so we should never have an issue trying to execute it.
#[no_mangle]
pub unsafe extern "C" fn parse_handshake_message(
    handle: u64,
    arr: CFFIByteArray,
) -> RustFFIByteArray {
    let msg = std::slice::from_raw_parts(arr.ptr, arr.len);
    // TODO error handling
    let result = HANDLE_MAPPING
        .lock()
        .get_mut(&handle)
        .unwrap()
        .handle_handshake_message(msg);
    if let Err(error) = result {
        log::error!("{:?}", error);
    }
    RustFFIByteArray {
        ptr: null_mut(),
        len: usize::MAX,
        cap: usize::MAX,
    }
}

#[no_mangle]
pub extern "C" fn get_verification_string(handle: u64, length: usize) -> RustFFIByteArray {
    HANDLE_MAPPING
        .lock()
        .get(&handle)
        .map(|h| {
            let auth_vec = h
                .to_completed_handshake()
                .unwrap()
                .auth_string::<CryptoProvider>()
                .derive_vec(length)
                .unwrap();
            let RawParts {
                ptr,
                length,
                capacity,
            } = RawParts::from_vec(auth_vec);
            RustFFIByteArray {
                ptr,
                len: length,
                cap: capacity,
            }
        })
        .unwrap()
}

#[no_mangle]
pub extern "C" fn to_connection_context(handle: u64) -> u64 {
    // TODO: error handling
    let ctx = HANDLE_MAPPING
        .lock()
        .remove(&handle)
        .map(move |mut ctx| {
            let result = Box::new(ctx.to_connection_context().unwrap());
            drop(ctx);
            result
        })
        .unwrap();
    insert_conn_gen_handle(ctx)
}

// Responder-specific functions
#[no_mangle]
pub extern "C" fn responder_new() -> u64 {
    let ctx = Box::new(ServerD2DHandshakeContext::<CryptoProvider>::new(
        HandshakeImplementation::PublicKeyInProtobuf,
    ));
    insert_gen_handle(ctx)
}

// Initiator-specific functions

/// # Safety
/// We treat next_protocol as data, not as executable memory.
#[no_mangle]
pub extern "C" fn initiator_new() -> u64 {
    let ctx = Box::new(InitiatorD2DHandshakeContext::<CryptoProvider>::new(
        HandshakeImplementation::PublicKeyInProtobuf,
    ));
    insert_gen_handle(ctx)
}

// Connection Context

/// # Safety
/// We treat msg and associated_data as data, not as executable memory.
/// associated_data and msg are slices so Rust won't try to do anything weird with allocation.
#[no_mangle]
pub unsafe extern "C" fn encode_message_to_peer(
    handle: u64,
    msg: CFFIByteArray,
    associated_data: CFFIByteArray,
) -> RustFFIByteArray {
    if msg.len == 0 {
        return RustFFIByteArray {
            ptr: null_mut(),
            len: usize::MAX,
            cap: usize::MAX,
        };
    }
    let msg = std::slice::from_raw_parts(msg.ptr, msg.len);
    let associated_data = if !associated_data.ptr.is_null() {
        Some(std::slice::from_raw_parts(
            associated_data.ptr,
            associated_data.len,
        ))
    } else {
        None
    };
    let ret = CONNECTION_HANDLE_MAPPING
        .lock()
        .get_mut(&handle)
        .map(|c| c.encode_message_to_peer::<CryptoProvider, _>(msg, associated_data));
    if let Some(msg) = ret {
        let RawParts {
            ptr,
            length,
            capacity,
        } = RawParts::from_vec(msg);
        RustFFIByteArray {
            ptr,
            len: length,
            cap: capacity,
        }
    } else {
        log::error!("Was unable to find handle!");
        RustFFIByteArray {
            ptr: null_mut(),
            len: usize::MAX,
            cap: usize::MAX,
        }
    }
}

/// # Safety
/// We treat msg as data, not as executable memory.
#[no_mangle]
pub unsafe extern "C" fn decode_message_from_peer(
    handle: u64,
    msg: CFFIByteArray,
    associated_data: CFFIByteArray,
) -> RustFFIByteArray {
    if msg.len == 0 {
        return RustFFIByteArray {
            ptr: null_mut(),
            len: usize::MAX,
            cap: usize::MAX,
        };
    }
    let msg = std::slice::from_raw_parts(msg.ptr, msg.len);
    let associated_data = if !associated_data.ptr.is_null() {
        Some(std::slice::from_raw_parts(
            associated_data.ptr,
            associated_data.len,
        ))
    } else {
        None
    };
    let ret: Result<Vec<u8>, ukey2_connections::DecodeError> = CONNECTION_HANDLE_MAPPING
        .lock()
        .get_mut(&handle)
        .unwrap()
        .decode_message_from_peer::<CryptoProvider, _>(msg, associated_data);
    if let Ok(decoded) = ret {
        let RawParts {
            ptr,
            length,
            capacity,
        } = RawParts::from_vec(decoded);
        RustFFIByteArray {
            ptr,
            len: length,
            cap: capacity,
        }
    } else {
        RustFFIByteArray {
            ptr: null_mut(),
            len: usize::MAX,
            cap: usize::MAX,
        }
    }
}

#[no_mangle]
pub extern "C" fn get_session_unique(handle: u64) -> RustFFIByteArray {
    let session_unique_bytes = CONNECTION_HANDLE_MAPPING
        .lock()
        .get(&handle)
        .unwrap()
        .get_session_unique::<CryptoProvider>();
    let RawParts {
        ptr,
        length,
        capacity,
    } = RawParts::from_vec(session_unique_bytes);
    RustFFIByteArray {
        ptr,
        len: length,
        cap: capacity,
    }
}

#[no_mangle]
pub extern "C" fn get_sequence_number_for_encoding(handle: u64) -> i32 {
    CONNECTION_HANDLE_MAPPING
        .lock()
        .get(&handle)
        .unwrap()
        .get_sequence_number_for_encoding()
}

#[no_mangle]
pub extern "C" fn get_sequence_number_for_decoding(handle: u64) -> i32 {
    CONNECTION_HANDLE_MAPPING
        .lock()
        .get(&handle)
        .unwrap()
        .get_sequence_number_for_decoding()
}

#[no_mangle]
pub extern "C" fn save_session(handle: u64) -> RustFFIByteArray {
    let key = CONNECTION_HANDLE_MAPPING
        .lock()
        .get(&handle)
        .unwrap()
        .save_session();
    let RawParts {
        ptr,
        length,
        capacity,
    } = RawParts::from_vec(key);
    RustFFIByteArray {
        ptr,
        len: length,
        cap: capacity,
    }
}

#[repr(i32)]
#[derive(Debug)]
pub enum Status {
    Good,
    Error,
}

#[repr(C)]
pub struct CD2DRestoreConnectionContextV1Result {
    handle: u64,
    status: Status,
}

/// # Safety
/// We error out if the length is incorrect (too large or too small) for restoring a session.
#[no_mangle]
pub unsafe extern "C" fn from_saved_session(
    arr: CFFIByteArray,
) -> CD2DRestoreConnectionContextV1Result {
    let saved_session = std::slice::from_raw_parts(arr.ptr, arr.len);
    let ctx = D2DConnectionContextV1::from_saved_session::<CryptoProvider>(saved_session);
    if let Ok(conn_ctx) = ctx {
        let final_ctx = Box::new(conn_ctx);
        CD2DRestoreConnectionContextV1Result {
            handle: insert_conn_gen_handle(final_ctx),
            status: Status::Good,
        }
    } else {
        log::error!(
            "failed to restore session with error {:?}",
            ctx.unwrap_err()
        );
        CD2DRestoreConnectionContextV1Result {
            handle: u64::MAX,
            status: Status::Error,
        }
    }
}
