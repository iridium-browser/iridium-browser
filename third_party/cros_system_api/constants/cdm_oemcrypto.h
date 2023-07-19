// Copyright 2021 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_CONSTANTS_CDM_OEMCRYPTO_H_
#define SYSTEM_API_CONSTANTS_CDM_OEMCRYPTO_H_

// Constants relating to buffer sizes and layout used by cdm-oemcrypto daemon
// to create buffers that are later processed by Chrome for video decoding.
namespace cdm_oemcrypto {
// Total header size at the front of the buffer before the video data.
constexpr size_t kSecureBufferHeaderSize = 1024;

// Marker bytes at the beginning of the header and corresponding size.
constexpr char kSecureBufferTag[] = "OECARCSB";
constexpr size_t kSecureBufferTagLen = sizeof(kSecureBufferTag) - 1;

// Offset from beginning of header to the protobuf. There is a uint32_t
// after the tag to indicate the size of the protobuf.
constexpr size_t kSecureBufferProtoOffset =
    kSecureBufferTagLen + sizeof(uint32_t);

}  // namespace cdm_oemcrypto
#endif  // SYSTEM_API_CONSTANTS_CDM_OEMCRYPTO_H_
