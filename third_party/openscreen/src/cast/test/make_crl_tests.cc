// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <unistd.h>

#include "cast/common/certificate/cast_crl.h"
#include "cast/common/certificate/date_time.h"
#include "cast/common/certificate/testing/test_helpers.h"
#include "cast/common/public/certificate_types.h"
#include "platform/test/paths.h"
#include "util/crypto/certificate_utils.h"
#include "util/crypto/digest_sign.h"
#include "util/crypto/pem_helpers.h"
#include "util/crypto/sha2.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {
namespace {

std::string* AddRevokedPublicKeyHash(TbsCrl* tbs_crl, X509* cert) {
  std::string* pubkey_hash = tbs_crl->add_revoked_public_key_hashes();
  std::string pubkey_spki = GetSpkiTlv(cert);
  *pubkey_hash = SHA256HashString(pubkey_spki).value();
  return pubkey_hash;
}

void AddSerialNumberRange(TbsCrl* tbs_crl,
                          X509* issuer,
                          uint64_t first,
                          uint64_t last) {
  SerialNumberRange* serial_range = tbs_crl->add_revoked_serial_number_ranges();
  std::string issuer_spki = GetSpkiTlv(issuer);
  serial_range->set_issuer_public_key_hash(
      SHA256HashString(issuer_spki).value());
  serial_range->set_first_serial_number(first);
  serial_range->set_last_serial_number(last);
}

TbsCrl MakeTbsCrl(uint64_t not_before,
                  uint64_t not_after,
                  X509* device_cert,
                  X509* inter_cert) {
  TbsCrl tbs_crl;
  tbs_crl.set_version(0);
  tbs_crl.set_not_before_seconds(not_before);
  tbs_crl.set_not_after_seconds(not_after);

  // NOTE: By default, include a hash which should not match any included certs.
  std::string* pubkey_hash = AddRevokedPublicKeyHash(&tbs_crl, device_cert);
  (*pubkey_hash)[0] ^= 0xff;

  // NOTE: Include default serial number range at device-level, which should not
  // include any of our certs.
  ErrorOr<uint64_t> maybe_serial =
      ParseDerUint64(X509_get0_serialNumber(device_cert));
  OSP_DCHECK(maybe_serial);
  uint64_t serial = maybe_serial.value();
  OSP_DCHECK_LE(serial, UINT64_MAX - 200);
  AddSerialNumberRange(&tbs_crl, inter_cert, serial + 100, serial + 200);

  return tbs_crl;
}

// Pack into a CrlBundle and sign with |crl_inter_key|.  |crl_inter_der| must be
// directly signed by a Cast CRL root CA (possibly distinct from Cast root CA).
void PackCrlIntoFile(const std::string& filename,
                     const TbsCrl& tbs_crl,
                     const std::string& crl_inter_der,
                     EVP_PKEY* crl_inter_key) {
  CrlBundle crl_bundle;
  Crl* crl = crl_bundle.add_crls();
  std::string* tbs_crl_serial = crl->mutable_tbs_crl();
  tbs_crl.SerializeToString(tbs_crl_serial);
  crl->set_signer_cert(crl_inter_der);
  ErrorOr<std::string> signature = SignData(
      EVP_sha256(), crl_inter_key,
      ByteView{reinterpret_cast<const uint8_t*>(tbs_crl_serial->data()),
               tbs_crl_serial->size()});
  OSP_DCHECK(signature);
  crl->set_signature(std::move(signature.value()));

  std::string output;
  crl_bundle.SerializeToString(&output);
  int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  OSP_DCHECK_GE(fd, 0);
  OSP_DCHECK_EQ(write(fd, output.data(), output.size()),
                static_cast<int>(output.size()));
  close(fd);
}

int CastMain() {
  const std::string data_path = GetTestDataPath() + "cast/receiver/channel/";
  bssl::UniquePtr<EVP_PKEY> inter_key =
      ReadKeyFromPemFile(data_path + "inter_key.pem");
  bssl::UniquePtr<EVP_PKEY> crl_inter_key =
      ReadKeyFromPemFile(data_path + "crl_inter_key.pem");
  OSP_DCHECK(inter_key);
  OSP_DCHECK(crl_inter_key);

  std::vector<std::string> chain_der =
      ReadCertificatesFromPemFile(data_path + "device_chain.pem");
  std::vector<std::string> crl_inter_der =
      ReadCertificatesFromPemFile(data_path + "crl_inter.pem");
  OSP_DCHECK_EQ(chain_der.size(), 3u);
  OSP_DCHECK_EQ(crl_inter_der.size(), 1u);

  std::string& device_der = chain_der[0];
  std::string& inter_der = chain_der[1];
  std::string& root_der = chain_der[2];

  auto* data = reinterpret_cast<const uint8_t*>(device_der.data());
  bssl::UniquePtr<X509> device_cert{
      d2i_X509(nullptr, &data, device_der.size())};
  data = reinterpret_cast<const uint8_t*>(inter_der.data());
  bssl::UniquePtr<X509> inter_cert{d2i_X509(nullptr, &data, inter_der.size())};
  data = reinterpret_cast<const uint8_t*>(root_der.data());
  bssl::UniquePtr<X509> root_cert{d2i_X509(nullptr, &data, root_der.size())};
  data = reinterpret_cast<const uint8_t*>(crl_inter_der[0].data());
  bssl::UniquePtr<X509> crl_inter_cert{
      d2i_X509(nullptr, &data, crl_inter_der[0].size())};
  OSP_DCHECK(device_cert);
  OSP_DCHECK(inter_cert);
  OSP_DCHECK(root_cert);
  OSP_DCHECK(crl_inter_cert);

  // NOTE: CRL where everything should pass.
  DateTime july2019 = {};
  july2019.month = 7;
  july2019.year = 2019;
  july2019.day = 16;
  DateTime july2020 = {};
  july2020.month = 7;
  july2020.year = 2020;
  july2020.day = 23;
  std::chrono::seconds not_before = DateTimeToSeconds(july2019);
  std::chrono::seconds not_after = DateTimeToSeconds(july2020);
  {
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after.count(),
                                device_cert.get(), inter_cert.get());
    PackCrlIntoFile(data_path + "good_crl.pb", tbs_crl, crl_inter_der[0],
                    crl_inter_key.get());
  }

  // NOTE: CRL used outside its valid time range.
  {
    DateTime august2019 = {};
    august2019.month = 8;
    august2019.year = 2019;
    august2019.day = 16;
    std::chrono::seconds not_after_invalid = DateTimeToSeconds(august2019);
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after_invalid.count(),
                                device_cert.get(), inter_cert.get());
    PackCrlIntoFile(data_path + "invalid_time_crl.pb", tbs_crl,
                    crl_inter_der[0], crl_inter_key.get());
  }

  // NOTE: Device's issuer revoked.
  {
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after.count(),
                                device_cert.get(), inter_cert.get());
    AddRevokedPublicKeyHash(&tbs_crl, inter_cert.get());
    PackCrlIntoFile(data_path + "issuer_revoked_crl.pb", tbs_crl,
                    crl_inter_der[0], crl_inter_key.get());
  }

  // NOTE: Device revoked.
  {
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after.count(),
                                device_cert.get(), inter_cert.get());
    AddRevokedPublicKeyHash(&tbs_crl, device_cert.get());
    PackCrlIntoFile(data_path + "device_revoked_crl.pb", tbs_crl,
                    crl_inter_der[0], crl_inter_key.get());
  }

  // NOTE: Issuer serial revoked.
  {
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after.count(),
                                device_cert.get(), inter_cert.get());
    ErrorOr<uint64_t> maybe_serial =
        ParseDerUint64(X509_get0_serialNumber(inter_cert.get()));
    OSP_DCHECK(maybe_serial);
    uint64_t serial = maybe_serial.value();
    OSP_DCHECK_GE(serial, 10);
    OSP_DCHECK_LE(serial, UINT64_MAX - 20);
    AddSerialNumberRange(&tbs_crl, root_cert.get(), serial - 10, serial + 20);
    PackCrlIntoFile(data_path + "issuer_serial_revoked_crl.pb", tbs_crl,
                    crl_inter_der[0], crl_inter_key.get());
  }

  // NOTE: Device serial revoked.
  {
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after.count(),
                                device_cert.get(), inter_cert.get());
    ErrorOr<uint64_t> maybe_serial =
        ParseDerUint64(X509_get0_serialNumber(device_cert.get()));
    OSP_DCHECK(maybe_serial);
    uint64_t serial = maybe_serial.value();
    OSP_DCHECK_GE(serial, 10);
    OSP_DCHECK_LE(serial, UINT64_MAX - 20);
    AddSerialNumberRange(&tbs_crl, inter_cert.get(), serial - 10, serial + 20);
    PackCrlIntoFile(data_path + "device_serial_revoked_crl.pb", tbs_crl,
                    crl_inter_der[0], crl_inter_key.get());
  }

  // NOTE: Bad |signer_cert| used for Crl (not issued by Cast CRL root).
  {
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after.count(),
                                device_cert.get(), inter_cert.get());
    PackCrlIntoFile(data_path + "bad_signer_cert_crl.pb", tbs_crl, inter_der,
                    inter_key.get());
  }

  // NOTE: Mismatched key for signature in Crl (just looks like bad signature).
  {
    TbsCrl tbs_crl = MakeTbsCrl(not_before.count(), not_after.count(),
                                device_cert.get(), inter_cert.get());
    PackCrlIntoFile(data_path + "bad_signature_crl.pb", tbs_crl,
                    crl_inter_der[0], inter_key.get());
  }

  return 0;
}

}  // namespace
}  // namespace cast
}  // namespace openscreen

int main() {
  return openscreen::cast::CastMain();
}
