// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef UKEY2_H
#define UKEY2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef uint64_t Ukey2HandshakeContextHandle;
typedef uint64_t Ukey2ConnectionContextHandle;
typedef uint8_t Aes256Key[32];

typedef struct {
  uint8_t* handle;
  size_t len;
  size_t cap;
} RustFFIByteArray;

typedef struct {
  uint8_t* handle;
  size_t len;
} CFFIByteArray;

typedef struct {
  bool success;
  RustFFIByteArray alert_to_send;
} CMessageParseResult;

typedef enum {
  STATUS_GOOD = 0,
  STATUS_ERR = 1,
} CD2DRestoreConnectionContextV1Status;

typedef struct {
  Ukey2ConnectionContextHandle handle;
  CD2DRestoreConnectionContextV1Status status;
} CD2DRestoreConnectionContextV1Result;

// Create a new ResponderD2DHandshakeContext
Ukey2HandshakeContextHandle responder_new();

// Create a new InitiatorD2DHandshakeContext
Ukey2HandshakeContextHandle initiator_new();

// Utilities
void rust_dealloc_ffi_byte_array(RustFFIByteArray array);

// Common handshake methods
bool is_handshake_complete(Ukey2HandshakeContextHandle handle);
RustFFIByteArray get_next_handshake_message(Ukey2HandshakeContextHandle handle);
CMessageParseResult parse_handshake_message(Ukey2HandshakeContextHandle handle, CFFIByteArray message);
Ukey2ConnectionContextHandle to_connection_context(Ukey2HandshakeContextHandle handle);
RustFFIByteArray get_verification_string(Ukey2HandshakeContextHandle handle, size_t output_length);

// D2DConnectionContextV1 methods
RustFFIByteArray encode_message_to_peer(Ukey2ConnectionContextHandle handle, CFFIByteArray message, CFFIByteArray associated_data);
RustFFIByteArray decode_message_from_peer(Ukey2ConnectionContextHandle handle, CFFIByteArray message, CFFIByteArray associated_data);
RustFFIByteArray get_session_unique(Ukey2ConnectionContextHandle handle);
int get_sequence_number_for_encoding(Ukey2ConnectionContextHandle handle);
int get_sequence_number_for_decoding(Ukey2ConnectionContextHandle handle);
RustFFIByteArray save_session(Ukey2ConnectionContextHandle handle);
CD2DRestoreConnectionContextV1Result from_saved_session(CFFIByteArray data);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
