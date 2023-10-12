// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/base/error.h"

#include <sstream>

namespace openscreen {

Error::Error() = default;

Error::Error(const Error& error) = default;

Error::Error(Error&& error) noexcept = default;

Error::Error(Code code) : code_(code) {}

Error::Error(Code code, const std::string& message)
    : code_(code), message_(message) {}

Error::Error(Code code, std::string&& message)
    : code_(code), message_(std::move(message)) {}

Error::~Error() = default;

Error& Error::operator=(const Error& other) = default;

Error& Error::operator=(Error&& other) = default;

bool Error::operator==(const Error& other) const {
  return code_ == other.code_ && message_ == other.message_;
}

bool Error::operator!=(const Error& other) const {
  return !(*this == other);
}

bool Error::operator==(Code code) const {
  return code_ == code;
}

bool Error::operator!=(Code code) const {
  return !(*this == code);
}

std::ostream& operator<<(std::ostream& os, const Error::Code& code) {
  if (code == Error::Code::kNone) {
    return os << "Success";
  }
  os << "Failure: ";
  switch (code) {
    case Error::Code::kAgain:
      return os << "Transient";
    case Error::Code::kCborParsing:
      return os << "CborParsing";
    case Error::Code::kCborEncoding:
      return os << "CborEncoding";
    case Error::Code::kCborIncompleteMessage:
      return os << "CborIncompleteMessage";
    case Error::Code::kCborInvalidMessage:
      return os << "CborInvalidMessage";
    case Error::Code::kCborInvalidResponseId:
      return os << "CborInvalidResponseId";
    case Error::Code::kNoAvailableReceivers:
      return os << "NoAvailableReceivers";
    case Error::Code::kRequestCancelled:
      return os << "RequestCancelled";
    case Error::Code::kNoPresentationFound:
      return os << "NoPresentationFound";
    case Error::Code::kPreviousStartInProgress:
      return os << "PreviousStartInProgress";
    case Error::Code::kUnknownStartError:
      return os << "UnknownStartError";
    case Error::Code::kUnknownRequestId:
      return os << "UnknownRequestId";
    case Error::Code::kAddressInUse:
      return os << "AddressInUse";
    case Error::Code::kDomainNameTooLong:
      return os << "DomainNameTooLong";
    case Error::Code::kDomainNameLabelTooLong:
      return os << "DomainNameLabelTooLong";
    case Error::Code::kIOFailure:
      return os << "IOFailure";
    case Error::Code::kInitializationFailure:
      return os << "InitializationFailure";
    case Error::Code::kInvalidIPV4Address:
      return os << "InvalidIPV4Address";
    case Error::Code::kInvalidIPV6Address:
      return os << "InvalidIPV6Address";
    case Error::Code::kConnectionFailed:
      return os << "ConnectionFailed";
    case Error::Code::kSocketOptionSettingFailure:
      return os << "SocketOptionSettingFailure";
    case Error::Code::kSocketAcceptFailure:
      return os << "SocketAcceptFailure";
    case Error::Code::kSocketBindFailure:
      return os << "SocketBindFailure";
    case Error::Code::kSocketClosedFailure:
      return os << "SocketClosedFailure";
    case Error::Code::kSocketConnectFailure:
      return os << "SocketConnectFailure";
    case Error::Code::kSocketInvalidState:
      return os << "SocketInvalidState";
    case Error::Code::kSocketListenFailure:
      return os << "SocketListenFailure";
    case Error::Code::kSocketReadFailure:
      return os << "SocketReadFailure";
    case Error::Code::kSocketSendFailure:
      return os << "SocketSendFailure";
    case Error::Code::kMdnsRegisterFailure:
      return os << "MdnsRegisterFailure";
    case Error::Code::kMdnsReadFailure:
      return os << "MdnsReadFailure";
    case Error::Code::kMdnsNonConformingFailure:
      return os << "kMdnsNonConformingFailure";
    case Error::Code::kParseError:
      return os << "ParseError";
    case Error::Code::kUnknownMessageType:
      return os << "UnknownMessageType";
    case Error::Code::kNoActiveConnection:
      return os << "NoActiveConnection";
    case Error::Code::kAlreadyClosed:
      return os << "AlreadyClosed";
    case Error::Code::kNoStartedPresentation:
      return os << "NoStartedPresentation";
    case Error::Code::kPresentationAlreadyStarted:
      return os << "PresentationAlreadyStarted";
    case Error::Code::kInvalidConnectionState:
      return os << "InvalidConnectionState";
    case Error::Code::kJsonParseError:
      return os << "JsonParseError";
    case Error::Code::kJsonWriteError:
      return os << "JsonWriteError";
    case Error::Code::kFatalSSLError:
      return os << "FatalSSLError";
    case Error::Code::kRSAKeyGenerationFailure:
      return os << "RSAKeyGenerationFailure";
    case Error::Code::kRSAKeyParseError:
      return os << "RSAKeyParseError";
    case Error::Code::kEVPInitializationError:
      return os << "EVPInitializationError";
    case Error::Code::kCertificateCreationError:
      return os << "CertificateCreationError";
    case Error::Code::kCertificateValidationError:
      return os << "CertificateValidationError";
    case Error::Code::kSha256HashFailure:
      return os << "Sha256HashFailure";
    case Error::Code::kFileLoadFailure:
      return os << "FileLoadFailure";
    case Error::Code::kErrCertsMissing:
      return os << "ErrCertsMissing";
    case Error::Code::kErrCertsParse:
      return os << "ErrCertsParse";
    case Error::Code::kErrCertsRestrictions:
      return os << "ErrCertsRestrictions";
    case Error::Code::kErrCertsDateInvalid:
      return os << "ErrCertsDateInvalid";
    case Error::Code::kErrCertsVerifyGeneric:
      return os << "ErrCertsVerifyGeneric";
    case Error::Code::kErrCertsVerifyUntrustedCert:
      return os << "kErrCertsVerifyUntrustedCert";
    case Error::Code::kErrCrlInvalid:
      return os << "ErrCrlInvalid";
    case Error::Code::kErrCertsRevoked:
      return os << "ErrCertsRevoked";
    case Error::Code::kErrCertsPathlen:
      return os << "ErrCertsPathlen";
    case Error::Code::kErrCertSerialize:
      return os << "ErrCertSerialize";
    case Error::Code::kCastV2PeerCertEmpty:
      return os << "kCastV2PeerCertEmpty";
    case Error::Code::kCastV2WrongPayloadType:
      return os << "kCastV2WrongPayloadType";
    case Error::Code::kCastV2NoPayload:
      return os << "kCastV2NoPayload";
    case Error::Code::kCastV2PayloadParsingFailed:
      return os << "kCastV2PayloadParsingFailed";
    case Error::Code::kCastV2MessageError:
      return os << "CastV2kMessageError";
    case Error::Code::kCastV2NoResponse:
      return os << "kCastV2NoResponse";
    case Error::Code::kCastV2FingerprintNotFound:
      return os << "kCastV2FingerprintNotFound";
    case Error::Code::kCastV2CertNotSignedByTrustedCa:
      return os << "kCastV2CertNotSignedByTrustedCa";
    case Error::Code::kCastV2CannotExtractPublicKey:
      return os << "kCastV2CannotExtractPublicKey";
    case Error::Code::kCastV2SignedBlobsMismatch:
      return os << "kCastV2SignedBlobsMismatch";
    case Error::Code::kCastV2TlsCertValidityPeriodTooLong:
      return os << "kCastV2TlsCertValidityPeriodTooLong";
    case Error::Code::kCastV2TlsCertValidStartDateInFuture:
      return os << "kCastV2TlsCertValidStartDateInFuture";
    case Error::Code::kCastV2TlsCertExpired:
      return os << "kCastV2TlsCertExpired";
    case Error::Code::kCastV2SenderNonceMismatch:
      return os << "kCastV2SenderNonceMismatch";
    case Error::Code::kCastV2DigestUnsupported:
      return os << "kCastV2DigestUnsupported";
    case Error::Code::kCastV2SignatureEmpty:
      return os << "kCastV2SignatureEmpty";
    case Error::Code::kCastV2ChannelNotOpen:
      return os << "kCastV2ChannelNotOpen";
    case Error::Code::kCastV2AuthenticationError:
      return os << "kCastV2AuthenticationError";
    case Error::Code::kCastV2ConnectError:
      return os << "kCastV2ConnectError";
    case Error::Code::kCastV2CastSocketError:
      return os << "kCastV2CastSocketError";
    case Error::Code::kCastV2TransportError:
      return os << "kCastV2TransportError";
    case Error::Code::kCastV2InvalidMessage:
      return os << "kCastV2InvalidMessage";
    case Error::Code::kCastV2InvalidChannelId:
      return os << "kCastV2InvalidChannelId";
    case Error::Code::kCastV2ConnectTimeout:
      return os << "kCastV2ConnectTimeout";
    case Error::Code::kCastV2PingTimeout:
      return os << "kCastV2PingTimeout";
    case Error::Code::kCastV2ChannelPolicyMismatch:
      return os << "kCastV2ChannelPolicyMismatch";
    case Error::Code::kCreateSignatureFailed:
      return os << "kCreateSignatureFailed";
    case Error::Code::kUpdateReceivedRecordFailure:
      return os << "kUpdateReceivedRecordFailure";
    case Error::Code::kRecordPublicationError:
      return os << "kRecordPublicationError";
    case Error::Code::kProcessReceivedRecordFailure:
      return os << "ProcessReceivedRecordFailure";
    case Error::Code::kUnknownError:
      return os << "UnknownError";
    case Error::Code::kNotImplemented:
      return os << "NotImplemented";
    case Error::Code::kInsufficientBuffer:
      return os << "InsufficientBuffer";
    case Error::Code::kParameterInvalid:
      return os << "ParameterInvalid";
    case Error::Code::kParameterOutOfRange:
      return os << "ParameterOutOfRange";
    case Error::Code::kParameterNullPointer:
      return os << "ParameterNullPointer";
    case Error::Code::kIndexOutOfBounds:
      return os << "IndexOutOfBounds";
    case Error::Code::kItemAlreadyExists:
      return os << "ItemAlreadyExists";
    case Error::Code::kItemNotFound:
      return os << "ItemNotFound";
    case Error::Code::kOperationInvalid:
      return os << "OperationInvalid";
    case Error::Code::kOperationInProgress:
      return os << "OperationInProgress";
    case Error::Code::kOperationCancelled:
      return os << "OperationCancelled";
    case Error::Code::kInterrupted:
      return os << "Interrupted";
    case Error::Code::kUnknownCodec:
      return os << "UnknownCodec";
    case Error::Code::kInvalidCodecParameter:
      return os << "InvalidCodecParameter";
    case Error::Code::kSocketFailure:
      return os << "SocketFailure";
    case Error::Code::kUnencryptedOffer:
      return os << "UnencryptedOffer";
    case Error::Code::kRemotingNotSupported:
      return os << "RemotingNotSupported";
    case Error::Code::kNoStreamSelected:
      return os << "NoStreamSelected";
    case Error::Code::kAnswerTimeout:
      return os << "AnswerTimeout";
    case Error::Code::kInvalidAnswer:
      return os << "InvalidAnswer";
    case Error::Code::kMessageTimeout:
      return os << "MessageTimeout";
    case Error::Code::kNone:
      break;
  }

  // Unused 'return' to get around failure on GCC.
  return os;
}

std::string Error::ToString() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::string ToString(openscreen::Error::Code code) {
  std::ostringstream ss;
  ss << code;
  return ss.str();
}

std::ostream& operator<<(std::ostream& out, const Error& error) {
  out << error.code() << " = \"" << error.message() << "\"";
  return out;
}

// static
const Error& Error::None() {
  static Error& error = *new Error(Code::kNone);
  return error;
}

}  // namespace openscreen
