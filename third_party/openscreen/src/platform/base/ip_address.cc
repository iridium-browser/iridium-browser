// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/ip_address.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <utility>

namespace openscreen {

IPAddress::IPAddress(Version version, const uint8_t* b) : version_(version) {
  if (version_ == Version::kV4) {
    bytes_ = {{b[0], b[1], b[2], b[3]}};
  } else {
    bytes_ = {{b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9],
               b[10], b[11], b[12], b[13], b[14], b[15]}};
  }
}

bool IPAddress::operator==(const IPAddress& o) const {
  if (version_ != o.version_)
    return false;

  if (version_ == Version::kV4) {
    return bytes_[0] == o.bytes_[0] && bytes_[1] == o.bytes_[1] &&
           bytes_[2] == o.bytes_[2] && bytes_[3] == o.bytes_[3];
  }
  return bytes_ == o.bytes_;
}

bool IPAddress::operator!=(const IPAddress& o) const {
  return !(*this == o);
}

IPAddress::operator bool() const {
  if (version_ == Version::kV4)
    return bytes_[0] | bytes_[1] | bytes_[2] | bytes_[3];

  for (const auto& byte : bytes_)
    if (byte)
      return true;

  return false;
}

void IPAddress::CopyToV4(uint8_t x[4]) const {
  assert(version_ == Version::kV4);
  std::memcpy(x, bytes_.data(), 4);
}

void IPAddress::CopyToV6(uint8_t x[16]) const {
  assert(version_ == Version::kV6);
  std::memcpy(x, bytes_.data(), 16);
}

namespace {

ErrorOr<IPAddress> ParseV4(const std::string& s) {
  int octets[4];
  int chars_scanned;
  // Note: sscanf()'s parsing for %d allows leading whitespace; so the invalid
  // presence of whitespace must be explicitly checked too.
  if (std::any_of(s.begin(), s.end(), [](char c) { return std::isspace(c); }) ||
      sscanf(s.c_str(), "%3d.%3d.%3d.%3d%n", &octets[0], &octets[1], &octets[2],
             &octets[3], &chars_scanned) != 4 ||
      chars_scanned != static_cast<int>(s.size()) ||
      std::any_of(std::begin(octets), std::end(octets),
                  [](int octet) { return octet < 0 || octet > 255; })) {
    return Error::Code::kInvalidIPV4Address;
  }
  return IPAddress(octets[0], octets[1], octets[2], octets[3]);
}

// Returns the zero-expansion of a double-colon in |s| if |s| is a
// well-formatted IPv6 address. If |s| is ill-formatted, returns *any* string
// that is ill-formatted.
std::string ExpandIPv6DoubleColon(const std::string& s) {
  constexpr char kDoubleColon[] = "::";
  const size_t double_colon_position = s.find(kDoubleColon);
  if (double_colon_position == std::string::npos) {
    return s;  // Nothing to expand.
  }
  if (double_colon_position != s.rfind(kDoubleColon)) {
    return {};  // More than one occurrence of double colons is illegal.
  }

  std::ostringstream expanded;
  const int num_single_colons = std::count(s.begin(), s.end(), ':') - 2;
  int num_zero_groups_to_insert = 8 - num_single_colons;
  if (double_colon_position != 0) {
    // abcd:0123:4567::f000:1
    // ^^^^^^^^^^^^^^^
    expanded << s.substr(0, double_colon_position + 1);
    --num_zero_groups_to_insert;
  }
  if (double_colon_position != (s.size() - 2)) {
    --num_zero_groups_to_insert;
  }
  while (--num_zero_groups_to_insert > 0) {
    expanded << "0:";
  }
  expanded << '0';
  if (double_colon_position != (s.size() - 2)) {
    // abcd:0123:4567::f000:1
    //                ^^^^^^^
    expanded << s.substr(double_colon_position + 1);
  }
  return expanded.str();
}

ErrorOr<IPAddress> ParseV6(const std::string& s) {
  const std::string scan_input = ExpandIPv6DoubleColon(s);
  uint16_t hextets[8];
  int chars_scanned;
  // Note: sscanf()'s parsing for %x allows leading whitespace; so the invalid
  // presence of whitespace must be explicitly checked too.
  if (std::any_of(s.begin(), s.end(), [](char c) { return std::isspace(c); }) ||
      sscanf(scan_input.c_str(),
             "%4" SCNx16 ":%4" SCNx16 ":%4" SCNx16 ":%4" SCNx16 ":%4" SCNx16
             ":%4" SCNx16 ":%4" SCNx16 ":%4" SCNx16 "%n",
             &hextets[0], &hextets[1], &hextets[2], &hextets[3], &hextets[4],
             &hextets[5], &hextets[6], &hextets[7], &chars_scanned) != 8 ||
      chars_scanned != static_cast<int>(scan_input.size())) {
    return Error::Code::kInvalidIPV6Address;
  }
  return IPAddress(hextets);
}

}  // namespace

// static
ErrorOr<IPAddress> IPAddress::Parse(const std::string& s) {
  ErrorOr<IPAddress> v4 = ParseV4(s);

  return v4 ? std::move(v4) : ParseV6(s);
}

// static
const IPEndpoint IPEndpoint::kAnyV4() {
  return IPEndpoint{};
}

// static
const IPEndpoint IPEndpoint::kAnyV6() {
  return IPEndpoint{IPAddress::kAnyV6(), 0};
}

IPEndpoint::operator bool() const {
  return address || port;
}

// static
ErrorOr<IPEndpoint> IPEndpoint::Parse(const std::string& s) {
  // Look for the colon that separates the IP address from the port number. Note
  // that this check also guards against the case where |s| is the empty string.
  const auto colon_pos = s.rfind(':');
  if (colon_pos == std::string::npos) {
    return Error(Error::Code::kParseError, "missing colon separator");
  }
  // The colon cannot be the first nor the last character in |s| because that
  // would mean there is no address part or port part.
  if (colon_pos == 0) {
    return Error(Error::Code::kParseError, "missing address before colon");
  }
  if (colon_pos == (s.size() - 1)) {
    return Error(Error::Code::kParseError, "missing port after colon");
  }

  ErrorOr<IPAddress> address(Error::Code::kParseError);
  if (s[0] == '[' && s[colon_pos - 1] == ']') {
    // [abcd:beef:1:1::2600]:8080
    // ^^^^^^^^^^^^^^^^^^^^^
    address = ParseV6(s.substr(1, colon_pos - 2));
  } else {
    // 127.0.0.1:22
    // ^^^^^^^^^
    address = ParseV4(s.substr(0, colon_pos));
  }
  if (address.is_error()) {
    return Error(Error::Code::kParseError, "invalid address part");
  }

  const char* const port_part = s.c_str() + colon_pos + 1;
  int port, chars_scanned;
  // Note: sscanf()'s parsing for %d allows leading whitespace. Thus, if the
  // first char is not whitespace, a successful sscanf() parse here can only
  // mean numerical chars contributed to the parsed integer.
  if (std::isspace(port_part[0]) ||
      sscanf(port_part, "%d%n", &port, &chars_scanned) != 1 ||
      port_part[chars_scanned] != '\0' || port < 0 ||
      port > std::numeric_limits<uint16_t>::max()) {
    return Error(Error::Code::kParseError, "invalid port part");
  }

  return IPEndpoint{address.value(), static_cast<uint16_t>(port)};
}

bool operator==(const IPEndpoint& a, const IPEndpoint& b) {
  return (a.address == b.address) && (a.port == b.port);
}

bool operator!=(const IPEndpoint& a, const IPEndpoint& b) {
  return !(a == b);
}

bool IPAddress::operator<(const IPAddress& other) const {
  if (version() != other.version()) {
    return version() < other.version();
  }

  if (IsV4()) {
    return memcmp(bytes_.data(), other.bytes_.data(), 4) < 0;
  } else {
    return memcmp(bytes_.data(), other.bytes_.data(), 16) < 0;
  }
}

bool operator<(const IPEndpoint& a, const IPEndpoint& b) {
  if (a.address != b.address) {
    return a.address < b.address;
  }

  return a.port < b.port;
}

std::ostream& operator<<(std::ostream& out, const IPAddress& address) {
  uint8_t values[16];
  size_t len = 0;
  char separator;
  size_t values_per_separator;
  int value_width;
  if (address.IsV4()) {
    out << std::dec;
    address.CopyToV4(values);
    len = 4;
    separator = '.';
    values_per_separator = 1;
    value_width = 0;
  } else if (address.IsV6()) {
    out << std::hex << std::setfill('0') << std::right;
    address.CopyToV6(values);
    len = 16;
    separator = ':';
    values_per_separator = 2;
    value_width = 2;
  }
  for (size_t i = 0; i < len; ++i) {
    if (i > 0 && (i % values_per_separator == 0)) {
      out << separator;
    }
    out << std::setw(value_width) << static_cast<int>(values[i]);
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const IPEndpoint& endpoint) {
  if (endpoint.address.IsV6()) {
    out << '[';
  }
  out << endpoint.address;
  if (endpoint.address.IsV6()) {
    out << ']';
  }
  return out << ':' << std::dec << static_cast<int>(endpoint.port);
}

std::string IPEndpoint::ToString() const {
  std::ostringstream name;
  name << *this;
  return name.str();
}

}  // namespace openscreen
