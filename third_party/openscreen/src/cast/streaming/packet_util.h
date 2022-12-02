// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STREAMING_PACKET_UTIL_H_
#define CAST_STREAMING_PACKET_UTIL_H_

#include <utility>

#include "absl/types/span.h"
#include "cast/streaming/ssrc.h"
#include "util/big_endian.h"

namespace openscreen {
namespace cast {

// Reads a field from the start of the given span and advances the span to point
// just after the field.
template <typename Integer>
inline Integer ConsumeField(absl::Span<const uint8_t>* in) {
  const Integer result = ReadBigEndian<Integer>(in->data());
  in->remove_prefix(sizeof(Integer));
  return result;
}

// Writes a field at the start of the given span and advances the span to point
// just after the field.
template <typename Integer>
inline void AppendField(Integer value, absl::Span<uint8_t>* out) {
  WriteBigEndian<Integer>(value, out->data());
  out->remove_prefix(sizeof(Integer));
}

// Returns a bitmask for a field having the given number of bits. For example,
// FieldBitmask<uint8_t>(5) returns 0b00011111.
template <typename Integer>
constexpr Integer FieldBitmask(unsigned field_size_in_bits) {
  return (Integer{1} << field_size_in_bits) - 1;
}

// Reserves |num_bytes| from the beginning of the given span, returning the
// reserved space.
inline absl::Span<uint8_t> ReserveSpace(int num_bytes,
                                        absl::Span<uint8_t>* out) {
  const absl::Span<uint8_t> reserved = out->subspan(0, num_bytes);
  out->remove_prefix(num_bytes);
  return reserved;
}

// Performs a quick-scan of the packet data for the purposes of routing it to an
// appropriate parser. Identifies whether the packet is a RTP packet, RTCP
// packet, or unknown; and provides the originator's SSRC. This only performs a
// very quick scan of the packet data, and does not guarantee that a full parse
// will later succeed.
enum class ApparentPacketType { UNKNOWN, RTP, RTCP };
std::pair<ApparentPacketType, Ssrc> InspectPacketForRouting(
    absl::Span<const uint8_t> packet);

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STREAMING_PACKET_UTIL_H_
