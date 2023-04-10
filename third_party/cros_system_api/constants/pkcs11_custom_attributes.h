// Copyright 2020 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_CONSTANTS_PKCS11_CUSTOM_ATTRIBUTES_H_
#define SYSTEM_API_CONSTANTS_PKCS11_CUSTOM_ATTRIBUTES_H_

#include <stdint.h>

namespace pkcs11_custom_attributes {

// Matches CK_ATTRIBUTE_TYPE unsigned long int from pkcs11t.h
// TODO(https://crbug.com/1068991): Move pkcs11t.h to a place accessible by
// system_api and reuse the type.
using CkAttributeType = unsigned long int;

// Matches CKA_VENDOR_DEFINED from pkcs11t.h
// TODO(https://crbug.com/1068991): Move pkcs11t.h to a place accessible by
// system_api and reuse the constant.
constexpr CkAttributeType kCkaVendorDefined = 0x80000000;

// Attributes for storing metadata with keys.
// 0x43724f53 has been chosen as the ASCII representation of "CrOS" to be
// something unlikely to collide with other components defining custom
// attributes (such as NSS).
constexpr CkAttributeType kCkaChromeOsFirstAttribute =
    kCkaVendorDefined | 0x43724f53;

// This attribute encodes in which contexts the key can be used.
// The type of the value of this attribute is a byte array (array of CK_BYTEs)
// representing a serialized KeyPermissions proto message.
constexpr CkAttributeType kCkaChromeOsKeyPermissions =
    kCkaChromeOsFirstAttribute;

// This attribute stores the profile ID for keys added through the built-in
// certificate provisioning feature.
// The type of the value of this attribute is a byte array (array of CK_BYTEs)
// that identifies a client certificate profile. This should be treated as
// opaque data. Implementation detail: chrome will dump the std::string into
// this, without appending a null terminator.
constexpr CkAttributeType kCkaChromeOsBuiltinProvisioningProfileId =
    kCkaChromeOsFirstAttribute + 1;

}  // namespace pkcs11_custom_attributes

#endif  // SYSTEM_API_CONSTANTS_PKCS11_CUSTOM_ATTRIBUTES_H_
