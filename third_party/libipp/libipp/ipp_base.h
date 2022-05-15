// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_BASE_H_
#define LIBIPP_IPP_BASE_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ipp_enums.h"  // NOLINT(build/include)
#include "ipp_export.h"  // NOLINT(build/include)
#include "ipp_package.h"  // NOLINT(build/include)

// This is part of libipp. See ipp.h for general information about this
// library.

namespace ipp {

// represents operation id
typedef E_operations_supported Operation;

// represents status-code [rfc8010]
typedef E_status_code Status;

// represents the version of IPP protocol
// The first byte (MSB) of this value correspond to major version number, the
// second one to minor version number.
typedef E_ipp_versions_supported Version;

// This class represents IPP request.
class IPP_EXPORT Request : public Package {
 public:
  // It creates new Request object using class from predefined schema in
  // ipp_operations.h. It returns nullptr if schema for given operation is
  // not known.
  static std::unique_ptr<Request> NewRequest(Operation);

  // Constructor.
  explicit Request(Operation operation_id) : operation_id_(operation_id) {}

  // Returns operation id set in the constructor.
  Operation GetOperationId() const { return operation_id_; }

 private:
  const Operation operation_id_;
};

// This class represents IPP response.
class IPP_EXPORT Response : public Package {
 public:
  // It creates new Response object using class from predefined schema in
  // ipp_operations.h. It returns nullptr if schema for given operation is
  // not known.
  static std::unique_ptr<Response> NewResponse(Operation);

  // Constructor.
  explicit Response(Operation operation_id) : operation_id_(operation_id) {}

  // Returns operation id set in the constructor.
  Operation GetOperationId() const { return operation_id_; }

  // Returns reference to status code of the response.
  Status& StatusCode() { return status_code_; }

 private:
  const Operation operation_id_;
  Status status_code_ = Status::successful_ok;
};

// Single entry in error logs used by Client and Server classes defined below.
struct Log {
  // Description of the error, it is always non-empty string.
  std::string message;
  // Position in the input buffer, set to 0 if unknown.
  std::size_t buf_offset = 0;
  // String with hex representation of part of the frame corresponding to
  // position given in buf_offset. Empty string if buf_offset == 0.
  std::string frame_context;
  // Attribute/Group names where the error occurred. Empty string if
  // unknown.
  std::string parser_context;
};

// Forward declaration of internal structure.
struct Protocol;

// This is a main class to build/parse frames from the client side.
class IPP_EXPORT Client {
 public:
  // Constructor. Parameter version and request_id are initial values for
  // variables used in frame header, they are both stored in the Client object.
  // The value of request_id is incremented each time the method
  // BuildRequestFrom() is called. When (request_id == 0), the first
  // constructed request will have (request_id == 1).
  explicit Client(Version version = Version::_1_1, int32_t request_id = 0);
  // Destructor
  ~Client();

  // Get/Set current version number of the IPP protocol.
  Version GetVersionNumber() const;
  void SetVersionNumber(Version);

  // Clears internal buffer and error log and increments request_id by 1.
  // Then fills internal buffer with the content defined in given
  // Request object. The given object may be modified by the method,
  // the following attributes are set automatically if their state is
  // AttrState::unset: attributes-charset, attributes-natural-language.
  void BuildRequestFrom(Request* request);

  // Returns the length of the frame constructed with BuildRequestFrom()
  // method or read with ReadResponseFrameFrom() method.
  std::size_t GetFrameLength() const;

  // Writes the frame from internal buffer to |buf|. The given vector is
  // resized to frame's size that equals the value returned by
  // GetFrameLength().
  // Fails when some values provided in BuildRequestFrom() are incorrect.
  // In that case, the given buffer may be partially written. The reason of
  // the failure is saved to the error log (see GetErrorLog() method).
  bool WriteRequestFrameTo(std::vector<uint8_t>* buf) const;

  // Clears internal buffer and error log.
  // Then fills internal buffer with the content extracted from a given
  // frame with response.
  // Returns true <=> whole frame was parsed. Even if the method returns
  // true, there still may be some errors and/or warnings in the error log
  // (see GetErrorLog() method). In this case, some attributes/groups may
  // be missing in the final output.
  // Returns false <=> the parser has spotted an error it cannot recover from.
  // In this case the internal buffer will contain incorrect data. The last
  // entry in the error log contains a reason of the failure.
  bool ReadResponseFrameFrom(const uint8_t* ptr, const uint8_t* const buf_end);
  bool ReadResponseFrameFrom(const std::vector<uint8_t>&);

  // Parses the content of the response loaded with ReadResponseFrameFrom
  // method and stores it in given the Response object.
  // When the parameter log_unknown_values is true, this method adds to the
  // error log info about all unknown attributes and groups extracted from
  // the frame and not known by given Response object.
  // Returns true <=> whole frame was parsed. Even if the method returns
  // true, there still may be some errors and/or warnings in the error log
  // (see GetErrorLog() method). In this case, some attributes/groups may
  // be missing in the final output.
  // Returns false <=> the parser has spotted an error it cannot recover from.
  // In this case the given Response object  will contain incorrect data. The
  // last entry in the error log contains a reason of the failure.
  bool ParseResponseAndSaveTo(Response* response,
                              bool log_unknown_values = false);

  // Returns the error log.
  const std::vector<Log>& GetErrorLog() const;

 private:
  std::unique_ptr<Protocol> protocol_;
};

// This is a main class to build/parse frames from the server side.
class IPP_EXPORT Server {
 public:
  // Constructor. Parameter version and request_id are initial values for
  // variables used in frame header, they are both stored in the Server object.
  // These two parameters are updated in ReadRequestFrameFrom method by
  // replacing them with values contained in a received request. The only
  // reason to set these values in the constructor is to build a response frame
  // without prior parsing of any request.
  explicit Server(Version version = Version::_1_1, int32_t request_id = 1);
  // Destructor
  ~Server();

  // Clears internal buffer and error log.
  // Then fills internal buffer with the content extracted from a given
  // frame with request. The internal fields with version and request_id are
  // set to values read from the given frame.
  // Returns true <=> whole frame was parsed. Even if the method returns
  // true, there still may be some errors and/or warnings in the error log
  // (see GetErrorLog() method). In this case, some attributes/groups may
  // be missing in the final output.
  // Returns false <=> the parser has spotted an error it cannot recover from.
  // In this case the internal buffer will contain incorrect data. The last
  // entry in the error log contains a reason of the failure.
  bool ReadRequestFrameFrom(const uint8_t* ptr, const uint8_t* const buf_end);
  bool ReadRequestFrameFrom(const std::vector<uint8_t>&);

  // This method returns operation id after successful run of
  // ReadRequestFrameFrom().
  Operation GetOperationId() const;

  // Get/Set current version number of the IPP protocol.
  Version GetVersionNumber() const;
  void SetVersionNumber(Version);

  // Parses the content of the request loaded with ReadRequestFrameFrom
  // method and stores it in given the Request object.
  // When the parameter log_unknown_values is true, this method adds to the
  // error log info about all unknown attributes and groups extracted from
  // the frame and not known by given Request object.
  // Returns true <=> whole frame was parsed. Even if the method returns
  // true, there still may be some errors and/or warnings in the error log
  // (see GetErrorLog() method). In this case, some attributes/groups may
  // be missing in the final output.
  // Returns false <=> the parser has spotted an error it cannot recover from.
  // In this case the given Request object will contain incorrect data. The
  // last entry in the error log contains a reason of the failure.
  bool ParseRequestAndSaveTo(Request* request, bool log_unknown_values = false);

  // Clears internal buffer and error log, fields with the protocol version
  // and request_id is left untouched.
  // Then fills internal buffer with the content defined in the given
  // Response object. The given object may be modified by the method,
  // the following attributes are set automatically if their state is
  // AttrState::unset: attributes-charset, attributes-natural-language,
  // status-message.
  void BuildResponseFrom(Response* response);

  // Returns the length of the frame constructed with BuildResponseFrom()
  // method or read with ReadRequestFrameFrom() method.
  std::size_t GetFrameLength() const;

  // Writes the frame from internal buffer to |buf|. The given vector is
  // resized to frame's size that equals the value returned by
  // GetFrameLength().
  // Fails when some values provided in BuildResponseFrom() are incorrect.
  // In that case, the given buffer may be partially written. The reason
  // of the failure is saved to the error log (see GetErrorLog() method).
  bool WriteResponseFrameTo(std::vector<uint8_t>* buf) const;

  // Returns the error log.
  const std::vector<Log>& GetErrorLog() const;

 private:
  std::unique_ptr<Protocol> protocol_;
};
}  // namespace ipp

#endif  //  LIBIPP_IPP_BASE_H_
