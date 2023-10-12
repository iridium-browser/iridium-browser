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

#include "ukey2_bindings.h"
#include "ukey2_ffi.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

CFFIByteArray nullByteArray() {
    return {
        .handle = nullptr,
        .len = 0,
    };
}

// Implementation of functions
Ukey2Handshake Ukey2Handshake::ForInitiator() {
    return Ukey2Handshake(initiator_new());
}

Ukey2Handshake Ukey2Handshake::ForResponder() {
    return Ukey2Handshake(responder_new());
}

bool Ukey2Handshake::IsHandshakeComplete() {
    return is_handshake_complete(handle_);
}

std::string Ukey2Handshake::GetNextHandshakeMessage() {
    RustFFIByteArray array = get_next_handshake_message(handle_);
    std::string ret = std::string((const char*) array.handle, array.len);
    rust_dealloc_ffi_byte_array(array);
    return ret;
}

ParseResult Ukey2Handshake::ParseHandshakeMessage(std::string message) {
    CFFIByteArray messageRaw{
        .handle = (uint8_t*)message.c_str(),
        .len = message.length(),
    };
    CMessageParseResult result = parse_handshake_message(handle_, messageRaw);
    std::string alert;
    if (!result.success) {
        std::cout << "parse failed" << std::endl;
        RustFFIByteArray array = result.alert_to_send;
        if (array.handle != nullptr) {
            alert = std::string((const char*) array.handle, array.len);
            rust_dealloc_ffi_byte_array(array);
        }
    }
    return ParseResult {
        .success = result.success,
        .alert_to_send = alert,
    };
}

std::string Ukey2Handshake::GetVerificationString(size_t output_length) {
    RustFFIByteArray array = get_verification_string(handle_, output_length);
    std::string ret = std::string((const char*) array.handle, array.len);
    rust_dealloc_ffi_byte_array(array);
    return ret;
}

D2DConnectionContextV1 Ukey2Handshake::ToConnectionContext() {
    assert(IsHandshakeComplete());
    return D2DConnectionContextV1(to_connection_context(handle_));
}

std::string D2DConnectionContextV1::DecodeMessageFromPeer(std::string message, std::string associated_data) {
    CFFIByteArray messageRaw{
        .handle = (uint8_t*)message.c_str(),
        .len = message.length(),
    };
    CFFIByteArray associatedDataRaw{
        .handle = (uint8_t*)associated_data.c_str(),
        .len = associated_data.length(),
    };
    RustFFIByteArray array =
        decode_message_from_peer(handle_, messageRaw, associatedDataRaw);
    if (array.handle == nullptr) {
        return "";
    }
    std::string ret = std::string((const char*) array.handle, array.len);
    rust_dealloc_ffi_byte_array(array);
    return ret;
}

std::string D2DConnectionContextV1::EncodeMessageToPeer(std::string message, std::string associated_data) {
    CFFIByteArray messageRaw{
        .handle = (uint8_t*)message.c_str(),
        .len = message.length(),
    };
    CFFIByteArray associatedDataRaw{
        .handle = (uint8_t*)associated_data.c_str(),
        .len = associated_data.length(),
    };
    RustFFIByteArray array =
        encode_message_to_peer(handle_, messageRaw, associatedDataRaw);
    std::string ret = std::string((const char*) array.handle, array.len);
    rust_dealloc_ffi_byte_array(array);
    return ret;
}

std::string D2DConnectionContextV1::GetSessionUnique() {
    RustFFIByteArray array = get_session_unique(handle_);
    std::string ret = std::string((const char*) array.handle, array.len);
    rust_dealloc_ffi_byte_array(array);
    return ret;
}

int D2DConnectionContextV1::GetSequenceNumberForEncoding() {
    return get_sequence_number_for_encoding(handle_);
}

int D2DConnectionContextV1::GetSequenceNumberForDecoding() {
    return get_sequence_number_for_decoding(handle_);
}

std::string D2DConnectionContextV1::SaveSession() {
    RustFFIByteArray array = save_session(handle_);
    std::string ret = std::string((const char*) array.handle, array.len);
    rust_dealloc_ffi_byte_array(array);
    return ret;
}

D2DRestoreConnectionContextV1Result D2DConnectionContextV1::FromSavedSession(std::string data) {
    CFFIByteArray arr{
        .handle = (uint8_t*)data.c_str(),
        .len = data.length(),
    };
    auto result = from_saved_session(arr);
    return {
        D2DConnectionContextV1(result.handle),
        result.status,
    };
}
