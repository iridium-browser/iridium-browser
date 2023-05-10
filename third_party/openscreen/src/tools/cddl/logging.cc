// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/cddl/logging.h"

// static
void Logger::Abort(const char* condition) {
  std::cerr << "CHECK(" << condition << ") failed!" << std::endl;
  std::abort();
}

void Logger::InitializeInstance() {
  is_initialized_ = true;

  WriteLog("CDDL GENERATION TOOL");
  WriteLog("---------------------------------------------\n");
}

void Logger::VerifyInitialized() {
  if (!is_initialized_) {
    InitializeInstance();
  }
}

const char* Logger::MakePrintable(const std::string& data) {
  return data.c_str();
}

Logger::Logger() {
  is_initialized_ = false;
}

// Static:
Logger* Logger::Get() {
  return Logger::singleton_;
}

// Static:
Logger* Logger::singleton_ = new Logger();
