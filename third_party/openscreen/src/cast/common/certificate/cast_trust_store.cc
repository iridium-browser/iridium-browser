// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/cast_trust_store.h"

#include <utility>

#include "util/crypto/pem_helpers.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

// -------------------------------------------------------------------------
// Cast trust anchors.
// -------------------------------------------------------------------------

// There are two trusted roots for Cast certificate chains:
//
//   (1) CN=Cast Root CA    (kCastRootCaDer)
//   (2) CN=Eureka Root CA  (kEurekaRootCaDer)
//
// These constants are defined by the files included next:

#include "cast/common/certificate/cast_root_ca_cert_der-inc.h"
#include "cast/common/certificate/eureka_root_ca_der-inc.h"

}  // namespace

// static
CastTrustStore* CastTrustStore::GetInstance() {
  if (!store_) {
    store_ = new CastTrustStore();
  }
  return store_;
}

// static
void CastTrustStore::ResetInstance() {
  delete store_;
  store_ = nullptr;
}

// static
CastTrustStore* CastTrustStore::CreateInstanceForTest(
    const std::vector<uint8_t>& trust_anchor_der) {
  OSP_DCHECK(!store_);
  store_ = new CastTrustStore(trust_anchor_der);
  return store_;
}

// static
CastTrustStore* CastTrustStore::CreateInstanceFromPemFile(
    absl::string_view file_path) {
  OSP_DCHECK(!store_);

  store_ = new CastTrustStore();
  store_->trust_store_ = TrustStore::CreateInstanceFromPemFile(file_path);
  return store_;
}

CastTrustStore::CastTrustStore() {
  trust_store_.certs.emplace_back(MakeTrustAnchor(kCastRootCaDer));
  trust_store_.certs.emplace_back(MakeTrustAnchor(kEurekaRootCaDer));
}

CastTrustStore::CastTrustStore(const std::vector<uint8_t>& trust_anchor_der) {
  trust_store_.certs.emplace_back(MakeTrustAnchor(trust_anchor_der));
}

CastTrustStore::CastTrustStore(TrustStore trust_store)
    : trust_store_(std::move(trust_store)) {}

CastTrustStore::~CastTrustStore() = default;

// static
CastTrustStore* CastTrustStore::store_ = nullptr;

}  // namespace cast
}  // namespace openscreen
