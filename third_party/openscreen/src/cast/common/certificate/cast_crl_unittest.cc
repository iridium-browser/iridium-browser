// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/common/certificate/cast_crl.h"

#include "cast/common/certificate/cast_cert_validator.h"
#include "cast/common/certificate/date_time.h"
#include "cast/common/certificate/proto/test_suite.pb.h"
#include "cast/common/certificate/testing/test_helpers.h"
#include "cast/common/public/trust_store.h"
#include "gtest/gtest.h"
#include "platform/test/paths.h"
#include "testing/util/read_file.h"
#include "util/osp_logging.h"

namespace openscreen {
namespace cast {

// TODO(crbug.com/openscreen/90): Remove these after Chromium is migrated to
// openscreen::cast
using DeviceCertTestSuite = ::cast::certificate::DeviceCertTestSuite;
using VerificationResult = ::cast::certificate::VerificationResult;
using DeviceCertTest = ::cast::certificate::DeviceCertTest;

namespace {

// Indicates the expected result of test step's verification.
enum TestStepResult {
  kResultSuccess,
  kResultFail,
};

// Verifies that the provided certificate chain is valid at the specified time
// and chains up to a trust anchor.
bool TestVerifyCertificate(TestStepResult expected_result,
                           const std::vector<std::string>& der_certs,
                           const DateTime& time,
                           TrustStore* cast_trust_store) {
  std::unique_ptr<ParsedCertificate> target_cert;
  CastDeviceCertPolicy policy;
  Error result =
      VerifyDeviceCert(der_certs, time, &target_cert, &policy, nullptr,
                       CRLPolicy::kCrlOptional, cast_trust_store);
  bool success = (result.code() == Error::Code::kNone) ==
                 (expected_result == kResultSuccess);
  EXPECT_TRUE(success);
  return success;
}

// Verifies that the provided Cast CRL is signed by a trusted issuer
// and that the CRL can be parsed successfully.
// The validity of the CRL is also checked at the specified time.
bool TestVerifyCRL(TestStepResult expected_result,
                   const std::string& crl_bundle,
                   const DateTime& time,
                   TrustStore* crl_trust_store) {
  std::unique_ptr<CastCRL> crl =
      ParseAndVerifyCRL(crl_bundle, time, crl_trust_store);

  bool success = (crl != nullptr) == (expected_result == kResultSuccess);
  EXPECT_TRUE(success);
  return success;
}

// Verifies that the certificate chain provided is not revoked according to
// the provided Cast CRL at |cert_time|.
// The provided CRL is verified at |crl_time|.
// If |crl_required| is set, then a valid Cast CRL must be provided.
// Otherwise, a missing CRL is be ignored.
bool TestVerifyRevocation(Error::Code expected_result,
                          const std::vector<std::string>& der_certs,
                          const std::string& crl_bundle,
                          const DateTime& crl_time,
                          const DateTime& cert_time,
                          bool crl_required,
                          TrustStore* cast_trust_store,
                          TrustStore* crl_trust_store) {
  std::unique_ptr<CastCRL> crl;
  if (!crl_bundle.empty()) {
    crl = ParseAndVerifyCRL(crl_bundle, crl_time, crl_trust_store);
    EXPECT_NE(crl.get(), nullptr);
  }

  std::unique_ptr<ParsedCertificate> target_cert;
  CastDeviceCertPolicy policy;
  CRLPolicy crl_policy =
      crl_required ? CRLPolicy::kCrlRequired : CRLPolicy::kCrlOptional;
  Error result = VerifyDeviceCert(der_certs, cert_time, &target_cert, &policy,
                                  crl.get(), crl_policy, cast_trust_store);
  EXPECT_EQ(expected_result, result.code());
  return expected_result == result.code();
}

const std::string& GetSpecificTestDataPath() {
  static std::string data_path = GetTestDataPath() + "cast/common/certificate/";
  return data_path;
}

bool RunTest(const DeviceCertTest& test_case) {
  std::unique_ptr<TrustStore> crl_trust_store;
  std::unique_ptr<TrustStore> cast_trust_store;
  if (test_case.use_test_trust_anchors()) {
    crl_trust_store = TrustStore::CreateInstanceFromPemFile(
        GetSpecificTestDataPath() + "certificates/cast_crl_test_root_ca.pem");
    cast_trust_store = TrustStore::CreateInstanceFromPemFile(
        GetSpecificTestDataPath() + "certificates/cast_test_root_ca.pem");
  } else {
    crl_trust_store = CastCRLTrustStore::Create();
    cast_trust_store = CastTrustStore::Create();
  }

  std::vector<std::string> der_cert_path;
  for (const auto& cert : test_case.der_cert_path()) {
    der_cert_path.push_back(cert);
  }

  DateTime cert_verification_time;
  EXPECT_TRUE(DateTimeFromSeconds(test_case.cert_verification_time_seconds(),
                                  &cert_verification_time));

  uint64_t crl_verify_time = test_case.crl_verification_time_seconds();
  DateTime crl_verification_time;
  EXPECT_TRUE(DateTimeFromSeconds(crl_verify_time, &crl_verification_time));
  if (crl_verify_time == 0) {
    crl_verification_time = cert_verification_time;
  }

  std::string crl_bundle = test_case.crl_bundle();
  switch (test_case.expected_result()) {
    case ::cast::certificate::PATH_VERIFICATION_FAILED:
      return TestVerifyCertificate(kResultFail, der_cert_path,
                                   cert_verification_time,
                                   cast_trust_store.get());
    case ::cast::certificate::CRL_VERIFICATION_FAILED:
      return TestVerifyCRL(kResultFail, crl_bundle, crl_verification_time,
                           crl_trust_store.get());
    case ::cast::certificate::REVOCATION_CHECK_FAILED_WITHOUT_CRL:
      return TestVerifyCertificate(kResultSuccess, der_cert_path,
                                   cert_verification_time,
                                   cast_trust_store.get()) &&
             TestVerifyCRL(kResultFail, crl_bundle, crl_verification_time,
                           crl_trust_store.get()) &&
             TestVerifyRevocation(
                 Error::Code::kErrCrlInvalid, der_cert_path, crl_bundle,
                 crl_verification_time, cert_verification_time, true,
                 cast_trust_store.get(), crl_trust_store.get());
    case ::cast::certificate::
        CRL_EXPIRED_AFTER_INITIAL_VERIFICATION:  // fallthrough
    case ::cast::certificate::REVOCATION_CHECK_FAILED:
      return TestVerifyCertificate(kResultSuccess, der_cert_path,
                                   cert_verification_time,
                                   cast_trust_store.get()) &&
             TestVerifyCRL(kResultSuccess, crl_bundle, crl_verification_time,
                           crl_trust_store.get()) &&
             TestVerifyRevocation(
                 Error::Code::kErrCertsRevoked, der_cert_path, crl_bundle,
                 crl_verification_time, cert_verification_time, true,
                 cast_trust_store.get(), crl_trust_store.get());
    case ::cast::certificate::SUCCESS:
      return (crl_bundle.empty() ||
              TestVerifyCRL(kResultSuccess, crl_bundle, crl_verification_time,
                            crl_trust_store.get())) &&
             TestVerifyCertificate(kResultSuccess, der_cert_path,
                                   cert_verification_time,
                                   cast_trust_store.get()) &&
             TestVerifyRevocation(Error::Code::kNone, der_cert_path, crl_bundle,
                                  crl_verification_time, cert_verification_time,
                                  !crl_bundle.empty(), cast_trust_store.get(),
                                  crl_trust_store.get());
    case ::cast::certificate::UNSPECIFIED:
      return false;
  }
  return false;
}

// Parses the provided test suite provided in wire-format proto.
// Each test contains the inputs and the expected output.
// To see the description of the test, execute the test.
// These tests are generated by a test generator in google3.
void RunTestSuite(const std::string& test_suite_file_name) {
  std::string testsuite_raw = ReadEntireFileToString(test_suite_file_name);
  ASSERT_FALSE(testsuite_raw.empty());
  DeviceCertTestSuite test_suite;
  ASSERT_TRUE(test_suite.ParseFromString(testsuite_raw));
  int successes = 0;

  for (auto const& test_case : test_suite.tests()) {
    bool result = RunTest(test_case);
    successes += result;
    EXPECT_TRUE(result) << test_case.description();
  }
  OSP_LOG_IF(ERROR, successes != test_suite.tests().size())
      << "successes: " << successes
      << ", failures: " << (test_suite.tests().size() - successes);
}

TEST(CastCertificateTest, TestSuite1) {
  RunTestSuite(GetSpecificTestDataPath() + "testsuite/testsuite1.pb");
}

}  // namespace
}  // namespace cast
}  // namespace openscreen
