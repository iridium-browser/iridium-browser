// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/crypto/openssl_util.h"

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdint.h>

#include <sstream>
#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "util/osp_logging.h"

namespace openscreen {

namespace {

// Callback routine for OpenSSL to print error messages. |str| is a
// nullptr-terminated string of length |len| containing diagnostic information
// such as the library, function and reason for the error, the file and line
// where the error originated, plus potentially any context-specific
// information about the error. |context| contains a pointer to user-supplied
// data, which is currently unused.
// If this callback returns a value <= 0, OpenSSL will stop processing the
// error queue and return, otherwise it will continue calling this function
// until all errors have been removed from the queue.
int OpenSSLErrorCallback(const char* str, size_t len, void* context) {
  OSP_DVLOG << "\t" << absl::string_view(str, len);
  return 1;
}

}  // namespace

void EnsureOpenSSLInit() {
  // If SSL fails to initialize, we can't run crypto.
  OSP_CHECK(OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, nullptr) == 1);
}

void EnsureOpenSSLCleanup() {
  EVP_cleanup();
}

void ClearOpenSSLERRStack(const Location& location) {
  if (OSP_DCHECK_IS_ON()) {
    uint32_t error_num = ERR_peek_error();
    if (error_num == 0) {
      return;
    }

    OSP_DVLOG << "OpenSSL ERR_get_error stack from " << location.ToString();
    ERR_print_errors_cb(&OpenSSLErrorCallback, nullptr);
  } else {
    ERR_clear_error();
  }
}

// General note about SSL errors. Error messages are pushed to the general
// OpenSSL error queue. Call ClearOpenSSLERRStack before calling any
// SSL methods.
Error GetSSLError(const SSL* ssl, int return_code) {
  const int error_code = SSL_get_error(ssl, return_code);
  if (error_code == SSL_ERROR_NONE) {
    return Error::None();
  }

  // Create error message w/ unwind of error stack + original SSL error string.
  std::stringstream msg;
  msg << "boringssl error (" << error_code
      << "): " << SSL_error_description(error_code);
  while (uint32_t packed_error = ERR_get_error()) {
    msg << "\nerr stack: " << ERR_reason_error_string(packed_error);
  }
  std::string message = msg.str();
  switch (error_code) {
    case SSL_ERROR_ZERO_RETURN:
      return Error(Error::Code::kSocketClosedFailure, std::move(message));

    case SSL_ERROR_WANT_READ:     // fallthrough
    case SSL_ERROR_WANT_WRITE:    // fallthrough
    case SSL_ERROR_WANT_CONNECT:  // fallthrough
    case SSL_ERROR_WANT_ACCEPT:   // fallthrough
    case SSL_ERROR_WANT_X509_LOOKUP:
      return Error(Error::Code::kAgain, std::move(message));

    case SSL_ERROR_SYSCALL:  // fallthrough
    case SSL_ERROR_SSL:
      return Error(Error::Code::kFatalSSLError, std::move(message));
  }
  OSP_NOTREACHED();
}
}  // namespace openscreen
