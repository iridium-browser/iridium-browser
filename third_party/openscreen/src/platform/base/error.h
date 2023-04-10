// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_ERROR_H_
#define PLATFORM_BASE_ERROR_H_

#include <cassert>
#include <ostream>
#include <string>
#include <utility>

#include "platform/base/macros.h"

namespace openscreen {

// Represents an error returned by an OSP library operation.  An error has a
// code and an optional message.
class Error {
 public:
  // TODO(crbug.com/openscreen/65): Group/rename OSP-specific errors
  // NOTE: new values should be added to the end of the of enum and existing
  // values should not be changed.
  enum class Code : int8_t {
    // No error occurred.
    kNone = 0,

    // A transient condition prevented the operation from proceeding (e.g.,
    // cannot send on a non-blocking socket without blocking). This indicates
    // the caller should try again later.
    kAgain = -1,

    // CBOR errors.
    kCborParsing = 1,
    kCborEncoding = 2,
    kCborIncompleteMessage = 3,
    kCborInvalidResponseId = 4,
    kCborInvalidMessage = 5,

    // Presentation start errors.
    kNoAvailableReceivers = 6,
    kRequestCancelled = 7,
    kNoPresentationFound = 8,
    kPreviousStartInProgress = 9,
    kUnknownStartError = 10,
    kUnknownRequestId = 11,

    kAddressInUse = 12,
    kDomainNameTooLong = 13,
    kDomainNameLabelTooLong = 14,

    kIOFailure = 15,
    kInitializationFailure = 16,
    kInvalidIPV4Address = 17,
    kInvalidIPV6Address = 18,
    kConnectionFailed = 19,

    kSocketOptionSettingFailure = 20,
    kSocketAcceptFailure = 21,
    kSocketBindFailure = 22,
    kSocketClosedFailure = 23,
    kSocketConnectFailure = 24,
    kSocketInvalidState = 25,
    kSocketListenFailure = 26,
    kSocketReadFailure = 27,
    kSocketSendFailure = 28,

    // MDNS errors.
    kMdnsRegisterFailure = 29,
    kMdnsReadFailure = 30,
    kMdnsNonConformingFailure = 31,

    kParseError = 32,
    kUnknownMessageType = 33,

    kNoActiveConnection = 34,
    kAlreadyClosed = 35,
    kInvalidConnectionState = 36,
    kNoStartedPresentation = 37,
    kPresentationAlreadyStarted = 38,

    kJsonParseError = 39,
    kJsonWriteError = 40,

    // OpenSSL errors.

    // Was unable to generate an RSA key.
    kRSAKeyGenerationFailure = 41,
    kRSAKeyParseError = 42,

    // Was unable to initialize an EVP_PKEY type.
    kEVPInitializationError = 43,

    // Was unable to generate a certificate.
    kCertificateCreationError = 44,

    // Certificate failed validation.
    kCertificateValidationError = 45,

    // Failed to produce a hashing digest.
    kSha256HashFailure = 46,

    // A non-recoverable SSL library error has occurred.
    kFatalSSLError = 47,
    kFileLoadFailure = 48,

    // Cast certificate errors.

    // Certificates were not provided for verification.
    kErrCertsMissing = 49,

    // The certificates provided could not be parsed.
    kErrCertsParse = 50,

    // Key usage is missing or is not set to Digital Signature.
    // This error could also be thrown if the CN is missing.
    kErrCertsRestrictions = 51,

    // The current date is before the notBefore date or after the notAfter date.
    kErrCertsDateInvalid = 52,

    // The certificate failed to chain to a trusted root.
    kErrCertsVerifyGeneric = 53,

    // The certificate was not found in the trust store.
    kErrCertsVerifyUntrustedCert = 54,

    // The CRL is missing or failed to verify.
    kErrCrlInvalid = 55,

    // One of the certificates in the chain is revoked.
    kErrCertsRevoked = 56,

    // The pathlen constraint of the root certificate was exceeded.
    kErrCertsPathlen = 57,

    // The certificate provided could not be serialized.
    kErrCertSerialize = 58,

    // Cast authentication errors.
    kCastV2PeerCertEmpty = 59,
    kCastV2WrongPayloadType = 60,
    kCastV2NoPayload = 61,
    kCastV2PayloadParsingFailed = 62,
    kCastV2MessageError = 63,
    kCastV2NoResponse = 64,
    kCastV2FingerprintNotFound = 65,
    kCastV2CertNotSignedByTrustedCa = 66,
    kCastV2CannotExtractPublicKey = 67,
    kCastV2SignedBlobsMismatch = 68,
    kCastV2TlsCertValidityPeriodTooLong = 69,
    kCastV2TlsCertValidStartDateInFuture = 70,
    kCastV2TlsCertExpired = 71,
    kCastV2SenderNonceMismatch = 72,
    kCastV2DigestUnsupported = 73,
    kCastV2SignatureEmpty = 74,

    // Cast channel errors.
    kCastV2ChannelNotOpen = 75,
    kCastV2AuthenticationError = 76,
    kCastV2ConnectError = 77,
    kCastV2CastSocketError = 78,
    kCastV2TransportError = 79,
    kCastV2InvalidMessage = 80,
    kCastV2InvalidChannelId = 81,
    kCastV2ConnectTimeout = 82,
    kCastV2PingTimeout = 83,
    kCastV2ChannelPolicyMismatch = 84,

    kCreateSignatureFailed = 85,

    // Discovery errors.
    kUpdateReceivedRecordFailure = 86,
    kRecordPublicationError = 87,
    kProcessReceivedRecordFailure = 88,

    // Generic errors.
    kUnknownError = 89,
    kNotImplemented = 90,
    kInsufficientBuffer = 91,
    kParameterInvalid = 92,
    kParameterOutOfRange = 93,
    kParameterNullPointer = 94,
    kIndexOutOfBounds = 95,
    kItemAlreadyExists = 96,
    kItemNotFound = 97,
    kOperationInvalid = 98,
    kOperationInProgress = 99,
    kOperationCancelled = 100,
    kInterrupted = 101,

    // Cast streaming errors.
    kUnknownCodec = 102,
    kInvalidCodecParameter = 103,
    kSocketFailure = 104,
    kUnencryptedOffer = 105,
    kRemotingNotSupported = 106,
    kNoStreamSelected = 107,

    // An Answer timeout means that the receiver failed to reply to our Offer
    // within a reasonable amount of time.
    kAnswerTimeout = 108,

    // Received an ANSWER, but it was invalid.
    kInvalidAnswer = 109,

    // A generic message timeout occured.
    kMessageTimeout = 110,
  };

  Error();
  Error(const Error& error);
  Error(Error&& error) noexcept;

  Error(Code code);  // NOLINT
  Error(Code code, const std::string& message);
  Error(Code code, std::string&& message);
  ~Error();

  Error& operator=(const Error& other);
  Error& operator=(Error&& other);
  bool operator==(const Error& other) const;
  bool operator!=(const Error& other) const;

  // Special case comparison with codes. Without this case, comparisons will
  // not work as expected, e.g.
  // const Error foo(Error::Code::kItemNotFound, "Didn't find an item");
  // foo == Error::Code::kItemNotFound is actually false.
  bool operator==(Code code) const;
  bool operator!=(Code code) const;
  bool ok() const { return code_ == Code::kNone; }

  Code code() const { return code_; }
  const std::string& message() const { return message_; }
  std::string& message() { return message_; }

  static const Error& None();

  std::string ToString() const;

 private:
  Code code_ = Code::kNone;
  std::string message_;
};

std::string ToString(openscreen::Error::Code code);
std::ostream& operator<<(std::ostream& os, const Error::Code& code);
std::ostream& operator<<(std::ostream& out, const Error& error);

// A convenience function to return a single value from a function that can
// return a value or an error.  For normal results, construct with a ValueType*
// (ErrorOr takes ownership) and the Error will be kNone with an empty message.
// For Error results, construct with an error code and value.
//
// Example:
//
// ErrorOr<Bar> Foo::DoSomething() {
//   if (success) {
//     return Bar();
//   } else {
//     return Error(kBadThingHappened, "No can do");
//   }
// }
//
// TODO(mfoltz): Add support for type conversions.
template <typename ValueType>
class ErrorOr {
 public:
  static ErrorOr<ValueType> None() {
    static ErrorOr<ValueType> error(Error::Code::kNone);
    return error;
  }

  ErrorOr(const ValueType& value) : value_(value), is_value_(true) {}  // NOLINT
  ErrorOr(ValueType&& value) noexcept                                  // NOLINT
      : value_(std::move(value)), is_value_(true) {}

  ErrorOr(const Error& error) : error_(error), is_value_(false) {  // NOLINT
    assert(error_.code() != Error::Code::kNone);
  }
  ErrorOr(Error&& error) noexcept  // NOLINT
      : error_(std::move(error)), is_value_(false) {
    assert(error_.code() != Error::Code::kNone);
  }
  ErrorOr(Error::Code code) : error_(code), is_value_(false) {  // NOLINT
    assert(error_.code() != Error::Code::kNone);
  }
  ErrorOr(Error::Code code, std::string message)
      : error_(code, std::move(message)), is_value_(false) {
    assert(error_.code() != Error::Code::kNone);
  }

  ErrorOr(const ErrorOr& other) = delete;
  ErrorOr(ErrorOr&& other) noexcept : is_value_(other.is_value_) {
    // NB: Both |value_| and |error_| are uninitialized memory at this point!
    // Unlike the other constructors, the compiler will not auto-generate
    // constructor calls for either union member because neither appeared in
    // this constructor's initializer list.
    if (other.is_value_) {
      new (&value_) ValueType(std::move(other.value_));
    } else {
      new (&error_) Error(std::move(other.error_));
    }
  }

  ErrorOr& operator=(const ErrorOr& other) = delete;
  ErrorOr& operator=(ErrorOr&& other) noexcept {
    this->~ErrorOr<ValueType>();
    new (this) ErrorOr<ValueType>(std::move(other));
    return *this;
  }

  ~ErrorOr() {
    // NB: |value_| or |error_| must be explicitly destroyed since the compiler
    // will not auto-generate the destructor calls for union members.
    if (is_value_) {
      value_.~ValueType();
    } else {
      error_.~Error();
    }
  }

  bool is_error() const { return !is_value_; }
  bool is_value() const { return is_value_; }

  // Unlike Error, we CAN provide an operator bool here, since it is
  // more obvious to callers that ErrorOr<Foo> will be true if it's Foo.
  operator bool() const { return is_value_; }

  const Error& error() const {
    assert(!is_value_);
    return error_;
  }
  Error& error() {
    assert(!is_value_);
    return error_;
  }

  const ValueType& value() const {
    assert(is_value_);
    return value_;
  }
  ValueType& value() {
    assert(is_value_);
    return value_;
  }

  // Move only value or fallback
  ValueType&& value(ValueType&& fallback) {
    if (is_value()) {
      return std::move(value());
    }
    return std::forward<ValueType>(fallback);
  }

  // Copy only value or fallback
  ValueType value(ValueType fallback) const {
    if (is_value()) {
      return value();
    }
    return std::move(fallback);
  }

 private:
  // Only one of these is an active member, determined by |is_value_|. Since
  // they are union'ed, they must be explicitly constructed and destroyed.
  union {
    ValueType value_;
    Error error_;
  };

  // If true, |value_| is initialized and active. Otherwise, |error_| is
  // initialized and active.
  const bool is_value_;
};

// Define comparison operators using SFINAE.
template <typename ValueType>
bool operator<(const ErrorOr<ValueType>& lhs, const ErrorOr<ValueType>& rhs) {
  // Handle the cases where one side is an error.
  if (lhs.is_error() != rhs.is_error()) {
    return lhs.is_error();
  }

  // Handle the case where both sides are errors.
  if (lhs.is_error()) {
    return static_cast<int8_t>(lhs.error().code()) <
           static_cast<int8_t>(rhs.error().code());
  }

  // Handle the case where both are values.
  return lhs.value() < rhs.value();
}

template <typename ValueType>
bool operator>(const ErrorOr<ValueType>& lhs, const ErrorOr<ValueType>& rhs) {
  return rhs < lhs;
}

template <typename ValueType>
bool operator<=(const ErrorOr<ValueType>& lhs, const ErrorOr<ValueType>& rhs) {
  return !(lhs > rhs);
}

template <typename ValueType>
bool operator>=(const ErrorOr<ValueType>& lhs, const ErrorOr<ValueType>& rhs) {
  return !(rhs < lhs);
}

template <typename ValueType>
bool operator==(const ErrorOr<ValueType>& lhs, const ErrorOr<ValueType>& rhs) {
  // Handle the cases where one side is an error.
  if (lhs.is_error() != rhs.is_error()) {
    return false;
  }

  // Handle the case where both sides are errors.
  if (lhs.is_error()) {
    return lhs.error() == rhs.error();
  }

  // Handle the case where both are values.
  return lhs.value() == rhs.value();
}

template <typename ValueType>
bool operator!=(const ErrorOr<ValueType>& lhs, const ErrorOr<ValueType>& rhs) {
  return !(lhs == rhs);
}

}  // namespace openscreen

#endif  // PLATFORM_BASE_ERROR_H_
