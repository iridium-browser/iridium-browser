// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTIL_CRYPTO_OPENSSL_UTIL_H_
#define UTIL_CRYPTO_OPENSSL_UTIL_H_

#include <openssl/ssl.h>
#include <stddef.h>

#include <cstring>

#include "platform/base/error.h"
#include "platform/base/location.h"
#include "platform/base/macros.h"

namespace openscreen {
// Initialize OpenSSL if it isn't already initialized. This must be called
// before any other OpenSSL functions though it is safe and cheap to call this
// multiple times.
// This function is thread-safe, and OpenSSL will only ever be initialized once.
// OpenSSL will be properly shut down on program exit.
// Multiple sequential calls to EnsureOpenSSLInit or EnsureOpenSSLCleanup are
// ignored by OpenSSL itself.
void EnsureOpenSSLInit();
void EnsureOpenSSLCleanup();

// Drains the OpenSSL ERR_get_error stack. On a debug build the error codes
// are send to VLOG(1), on a release build they are disregarded. In most
// cases you should pass CURRENT_LOCATION as the |location|.
void ClearOpenSSLERRStack(const Location& location);

Error GetSSLError(const SSL* ssl, int return_code);

// Place an instance of this class on the call stack to automatically clear
// the OpenSSL error stack on function exit.
class OpenSSLErrStackTracer {
 public:
  // Pass CURRENT_LOCATION as |location|, to help track the source of OpenSSL
  // error messages. Note any diagnostic emitted will be tagged with the
  // location of the constructor call as it's not possible to trace a
  // destructor's callsite.
  explicit OpenSSLErrStackTracer(const Location& location)
      : location_(location) {
    EnsureOpenSSLInit();
  }
  ~OpenSSLErrStackTracer() { ClearOpenSSLERRStack(location_); }

 private:
  const Location location_;

  OSP_DISALLOW_IMPLICIT_CONSTRUCTORS(OpenSSLErrStackTracer);
};

}  // namespace openscreen

#endif  // UTIL_CRYPTO_OPENSSL_UTIL_H_
