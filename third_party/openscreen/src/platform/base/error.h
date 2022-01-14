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
  enum class Code : int8_t {
    // No error occurred.
    kNone = 0,

    // A transient condition prevented the operation from proceeding (e.g.,
    // cannot send on a non-blocking socket without blocking). This indicates
    // the caller should try again later.
    kAgain = -1,

    // CBOR errors.
    kCborParsing = 1,
    kCborEncoding,
    kCborIncompleteMessage,
    kCborInvalidResponseId,
    kCborInvalidMessage,

    // Presentation start errors.
    kNoAvailableReceivers,
    kRequestCancelled,
    kNoPresentationFound,
    kPreviousStartInProgress,
    kUnknownStartError,
    kUnknownRequestId,

    kAddressInUse,
    kDomainNameTooLong,
    kDomainNameLabelTooLong,

    kIOFailure,
    kInitializationFailure,
    kInvalidIPV4Address,
    kInvalidIPV6Address,
    kConnectionFailed,

    kSocketOptionSettingFailure,
    kSocketAcceptFailure,
    kSocketBindFailure,
    kSocketClosedFailure,
    kSocketConnectFailure,
    kSocketInvalidState,
    kSocketListenFailure,
    kSocketReadFailure,
    kSocketSendFailure,

    // MDNS errors.
    kMdnsRegisterFailure,
    kMdnsReadFailure,
    kMdnsNonConformingFailure,

    kParseError,
    kUnknownMessageType,

    kNoActiveConnection,
    kAlreadyClosed,
    kInvalidConnectionState,
    kNoStartedPresentation,
    kPresentationAlreadyStarted,

    kJsonParseError,
    kJsonWriteError,

    // OpenSSL errors.

    // Was unable to generate an RSA key.
    kRSAKeyGenerationFailure,
    kRSAKeyParseError,

    // Was unable to initialize an EVP_PKEY type.
    kEVPInitializationError,

    // Was unable to generate a certificate.
    kCertificateCreationError,

    // Certificate failed validation.
    kCertificateValidationError,

    // Failed to produce a hashing digest.
    kSha256HashFailure,

    // A non-recoverable SSL library error has occurred.
    kFatalSSLError,
    kFileLoadFailure,

    // Cast certificate errors.

    // Certificates were not provided for verification.
    kErrCertsMissing,

    // The certificates provided could not be parsed.
    kErrCertsParse,

    // Key usage is missing or is not set to Digital Signature.
    // This error could also be thrown if the CN is missing.
    kErrCertsRestrictions,

    // The current date is before the notBefore date or after the notAfter date.
    kErrCertsDateInvalid,

    // The certificate failed to chain to a trusted root.
    kErrCertsVerifyGeneric,

    // The certificate was not found in the trust store.
    kErrCertsVerifyUntrustedCert,

    // The CRL is missing or failed to verify.
    kErrCrlInvalid,

    // One of the certificates in the chain is revoked.
    kErrCertsRevoked,

    // The pathlen constraint of the root certificate was exceeded.
    kErrCertsPathlen,

    // Cast authentication errors.
    kCastV2PeerCertEmpty,
    kCastV2WrongPayloadType,
    kCastV2NoPayload,
    kCastV2PayloadParsingFailed,
    kCastV2MessageError,
    kCastV2NoResponse,
    kCastV2FingerprintNotFound,
    kCastV2CertNotSignedByTrustedCa,
    kCastV2CannotExtractPublicKey,
    kCastV2SignedBlobsMismatch,
    kCastV2TlsCertValidityPeriodTooLong,
    kCastV2TlsCertValidStartDateInFuture,
    kCastV2TlsCertExpired,
    kCastV2SenderNonceMismatch,
    kCastV2DigestUnsupported,
    kCastV2SignatureEmpty,

    // Cast channel errors.
    kCastV2ChannelNotOpen,
    kCastV2AuthenticationError,
    kCastV2ConnectError,
    kCastV2CastSocketError,
    kCastV2TransportError,
    kCastV2InvalidMessage,
    kCastV2InvalidChannelId,
    kCastV2ConnectTimeout,
    kCastV2PingTimeout,
    kCastV2ChannelPolicyMismatch,

    kCreateSignatureFailed,

    // Discovery errors.
    kUpdateReceivedRecordFailure,
    kRecordPublicationError,
    kProcessReceivedRecordFailure,

    // Generic errors.
    kUnknownError,
    kNotImplemented,
    kInsufficientBuffer,
    kParameterInvalid,
    kParameterOutOfRange,
    kParameterNullPointer,
    kIndexOutOfBounds,
    kItemAlreadyExists,
    kItemNotFound,
    kOperationInvalid,
    kOperationInProgress,
    kOperationCancelled,

    // Cast streaming errors
    kTypeError,
    kUnknownCodec,
    kInvalidCodecParameter,
    kSocketFailure,
    kUnencryptedOffer,
    kRemotingNotSupported,

    // A negotiation failure means that the current negotiation must be
    // restarted by the sender.
    kNegotiationFailure,
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
