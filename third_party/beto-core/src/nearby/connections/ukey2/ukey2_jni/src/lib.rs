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

use jni::objects::{JByteArray, JClass};
use jni::sys::{jboolean, jbyteArray, jint, jlong, JNI_TRUE};
use jni::JNIEnv;
use lazy_static::lazy_static;
use lock_adapter::NoPoisonMutex;
use rand::Rng;
use rand_chacha::rand_core::SeedableRng;
use rand_chacha::ChaCha20Rng;

#[cfg(not(feature = "std"))]
use lock_adapter::spin::Mutex;
#[cfg(feature = "std")]
use lock_adapter::std::Mutex;

use ukey2_connections::{
    D2DConnectionContextV1, D2DHandshakeContext, DecodeError, DeserializeError, HandleMessageError,
    HandshakeError, HandshakeImplementation, InitiatorD2DHandshakeContext,
    ServerD2DHandshakeContext,
};

cfg_if::cfg_if! {
    if #[cfg(feature = "rustcrypto")] {
        use crypto_provider_rustcrypto::RustCrypto as CryptoProvider;
    } else {
        use crypto_provider_openssl::Openssl as CryptoProvider;
    }
}
// Handle management

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

pub(crate) fn insert_handshake_handle(item: D2DBox) -> u64 {
    let handle = generate_handle();
    HANDLE_MAPPING.lock().insert(handle, item);
    handle
}

pub(crate) fn insert_conn_handle(item: ConnectionBox) -> u64 {
    let handle = generate_handle();
    CONNECTION_HANDLE_MAPPING.lock().insert(handle, item);
    handle
}

#[derive(Debug)]
enum JniError {
    BadHandle,
    DecodeError(DecodeError),
    HandleMessageError(HandleMessageError),
    HandshakeError(HandshakeError),
}

// D2DHandshakeContext
#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DHandshakeContext_is_1handshake_1complete(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
) -> jboolean {
    let mut is_complete = false;
    if let Some(ctx) = HANDLE_MAPPING.lock().get(&(context_handle as u64)) {
        is_complete = ctx.is_handshake_complete();
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
    }
    is_complete as jboolean
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DHandshakeContext_create_1context(
    _: JNIEnv,
    _: JClass,
    is_client: jboolean,
) -> jlong {
    if is_client == JNI_TRUE {
        let client_obj = Box::new(InitiatorD2DHandshakeContext::<CryptoProvider>::new(
            HandshakeImplementation::PublicKeyInProtobuf,
        ));
        insert_handshake_handle(client_obj) as jlong
    } else {
        let server_obj = Box::new(ServerD2DHandshakeContext::<CryptoProvider>::new(
            HandshakeImplementation::PublicKeyInProtobuf,
        ));
        insert_handshake_handle(server_obj) as jlong
    }
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DHandshakeContext_get_1next_1handshake_1message(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
) -> jbyteArray {
    let empty_arr = env.new_byte_array(0).unwrap();
    let next_message = if let Some(ctx) = HANDLE_MAPPING.lock().get(&(context_handle as u64)) {
        ctx.get_next_handshake_message()
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        None
    };
    // TODO error handling
    if let Some(message) = next_message {
        env.byte_array_from_slice(message.as_slice()).unwrap()
    } else {
        empty_arr
    }
    .into_raw()
}

#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
/// Safety: We know the message pointer is safe as it is coming directly from the JVM.
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DHandshakeContext_parse_1handshake_1message(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
    message: jbyteArray,
) {
    let rust_buffer = env.convert_byte_array(unsafe { JByteArray::from_raw(message) }).unwrap();
    let result = if let Some(ctx) = HANDLE_MAPPING.lock().get_mut(&(context_handle as u64)) {
        ctx.handle_handshake_message(rust_buffer.as_slice()).map_err(JniError::HandleMessageError)
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        Err(JniError::BadHandle)
    };
    if let Err(e) = result {
        if !env.exception_check().unwrap() {
            env.throw_new(
                "com/google/security/cryptauth/lib/securegcm/HandshakeException",
                match e {
                    JniError::BadHandle => "Bad handle",
                    JniError::DecodeError(_) => "Unable to decode message",
                    JniError::HandleMessageError(_) => "Unable to handle message",
                    JniError::HandshakeError(_) => "Handshake incomplete",
                },
            )
            .expect("failed to find error class");
        }
    }
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DHandshakeContext_get_1verification_1string(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
    length: jint,
) -> jbyteArray {
    let empty_array = env.new_byte_array(0).unwrap();
    let result = if let Some(ctx) = HANDLE_MAPPING.lock().get_mut(&(context_handle as u64)) {
        ctx.to_completed_handshake()
            .map_err(|_| JniError::HandshakeError(HandshakeError::HandshakeNotComplete))
            .map(|h| h.auth_string::<CryptoProvider>().derive_vec(length as usize).unwrap())
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        Err(JniError::BadHandle)
    };
    if let Err(e) = result {
        if !env.exception_check().unwrap() {
            env.throw_new(
                "com/google/security/cryptauth/lib/securegcm/HandshakeException",
                match e {
                    JniError::BadHandle => "Bad handle",
                    JniError::DecodeError(_) => "Unable to decode message",
                    JniError::HandleMessageError(_) => "Unable to handle message",
                    JniError::HandshakeError(_) => "Handshake incomplete",
                },
            )
            .expect("failed to find error class");
        }
        empty_array
    } else {
        let ret_vec = result.unwrap();
        env.byte_array_from_slice(&ret_vec).unwrap()
    }
    .into_raw()
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DHandshakeContext_to_1connection_1context(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
) -> jlong {
    let conn_context = if let Some(ctx) = HANDLE_MAPPING.lock().get_mut(&(context_handle as u64)) {
        ctx.to_connection_context().map_err(JniError::HandshakeError)
    } else {
        Err(JniError::BadHandle)
    };
    if let Err(error) = conn_context {
        env.throw_new(
            "com/google/security/cryptauth/lib/securegcm/HandshakeException",
            match error {
                JniError::BadHandle => "Bad context handle",
                JniError::HandshakeError(_) => "Handshake not complete",
                JniError::DecodeError(_) | JniError::HandleMessageError(_) => "Unknown exception",
            },
        )
        .expect("failed to find error class");
        return -1;
    } else {
        HANDLE_MAPPING.lock().remove(&(context_handle as u64));
    }
    insert_conn_handle(Box::new(conn_context.unwrap())) as jlong
}

// D2DConnectionContextV1
#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
/// Safety: We know the payload and associated_data pointers are safe as they are coming directly
/// from the JVM.
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DConnectionContextV1_encode_1message_1to_1peer(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
    payload: jbyteArray,
    associated_data: jbyteArray,
) -> jbyteArray {
    // We create the empty array here so we don't run into issues requesting a new byte array from
    // the JNI env while an exception is being thrown.
    let empty_array = env.new_byte_array(0).unwrap();
    let result = if let Some(ctx) =
        CONNECTION_HANDLE_MAPPING.lock().get_mut(&(context_handle as u64))
    {
        Ok(ctx.encode_message_to_peer::<CryptoProvider, _>(
            env.convert_byte_array(unsafe { JByteArray::from_raw(payload) }).unwrap().as_slice(),
            if associated_data.is_null() {
                None
            } else {
                Some(
                    env.convert_byte_array(unsafe { JByteArray::from_raw(associated_data) })
                        .unwrap(),
                )
            },
        ))
    } else {
        Err(JniError::BadHandle)
    };
    if let Ok(ret_vec) = result {
        env.byte_array_from_slice(ret_vec.as_slice()).expect("unable to create jByteArray")
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        empty_array
    }
    .into_raw()
}

#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
/// Safety: We know the message and associated_data pointers are safe as they are coming directly
/// from the JVM.
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DConnectionContextV1_decode_1message_1from_1peer(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
    message: jbyteArray,
    associated_data: jbyteArray,
) -> jbyteArray {
    let empty_array = env.new_byte_array(0).unwrap();
    let result = if let Some(ctx) =
        CONNECTION_HANDLE_MAPPING.lock().get_mut(&(context_handle as u64))
    {
        ctx.decode_message_from_peer::<CryptoProvider, _>(
            env.convert_byte_array(unsafe { JByteArray::from_raw(message) }).unwrap().as_slice(),
            if associated_data.is_null() {
                None
            } else {
                Some(
                    env.convert_byte_array(unsafe { JByteArray::from_raw(associated_data) })
                        .unwrap(),
                )
            },
        )
        .map_err(JniError::DecodeError)
    } else {
        Err(JniError::BadHandle)
    };
    if let Ok(message) = result {
        env.byte_array_from_slice(message.as_slice()).expect("unable to create jByteArray")
    } else {
        env.throw_new(
            "com/google/security/cryptauth/lib/securegcm/CryptoException",
            match result.unwrap_err() {
                JniError::BadHandle => "Bad context handle",
                JniError::DecodeError(e) => match e {
                    DecodeError::BadData => "Bad data",
                    DecodeError::BadSequenceNumber => "Bad sequence number",
                },
                // None of these should ever occur in this case.
                JniError::HandleMessageError(_) | JniError::HandshakeError(_) => "Unknown error",
            },
        )
        .expect("failed to find exception class");
        empty_array
    }
    .into_raw()
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DConnectionContextV1_get_1sequence_1number_1for_1encoding(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
) -> jint {
    if let Some(ctx) = CONNECTION_HANDLE_MAPPING.lock().get(&(context_handle as u64)) {
        ctx.get_sequence_number_for_encoding() as jint
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        -1 as jint
    }
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DConnectionContextV1_get_1sequence_1number_1for_1decoding(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
) -> jint {
    if let Some(ctx) = CONNECTION_HANDLE_MAPPING.lock().get(&(context_handle as u64)) {
        ctx.get_sequence_number_for_decoding() as jint
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        -1 as jint
    }
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DConnectionContextV1_save_1session(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
) -> jbyteArray {
    let empty_array = env.new_byte_array(0).unwrap();
    if let Some(ctx) = CONNECTION_HANDLE_MAPPING.lock().get(&(context_handle as u64)) {
        env.byte_array_from_slice(ctx.save_session().as_slice()).expect("unable to save session")
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        empty_array
    }
    .into_raw()
}

#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
/// Safety: We know the session_info pointer is safe because it is coming directly from the JVM.
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DConnectionContextV1_from_1saved_1session(
    mut env: JNIEnv,
    _: JClass,
    session_info: jbyteArray,
) -> jlong {
    let session_info_rust = env
        .convert_byte_array(unsafe { JByteArray::from_raw(session_info) })
        .expect("bad session_info data");
    let ctx =
        D2DConnectionContextV1::from_saved_session::<CryptoProvider>(session_info_rust.as_slice());
    if ctx.is_err() {
        env.throw_new(
            "com/google/security/cryptauth/lib/securegcm/SessionRestoreException",
            match ctx.err().unwrap() {
                DeserializeError::BadDataLength => "DeserializeError: bad session_info length",
                DeserializeError::BadProtocolVersion => "DeserializeError: bad protocol version",
                DeserializeError::BadData => "DeserializeError: bad data",
            },
        )
        .expect("failed to find exception class");
        return -1;
    }
    let final_ctx = ctx.ok().unwrap();
    let conn_context_final = Box::new(final_ctx);
    insert_conn_handle(conn_context_final) as jlong
}

#[no_mangle]
pub extern "system" fn Java_com_google_security_cryptauth_lib_securegcm_D2DConnectionContextV1_get_1session_1unique(
    mut env: JNIEnv,
    _: JClass,
    context_handle: jlong,
) -> jbyteArray {
    let empty_array = env.new_byte_array(0).unwrap();
    if let Some(ctx) = CONNECTION_HANDLE_MAPPING.lock().get(&(context_handle as u64)) {
        env.byte_array_from_slice(ctx.get_session_unique::<CryptoProvider>().as_slice())
            .expect("unable to get unique session id")
    } else {
        env.throw_new("com/google/security/cryptauth/lib/securegcm/BadHandleException", "")
            .expect("failed to find error class");
        empty_array
    }
    .into_raw()
}
