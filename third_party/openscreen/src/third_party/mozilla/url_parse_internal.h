// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_MOZILLA_URL_PARSE_INTERNAL_H_
#define THIRD_PARTY_MOZILLA_URL_PARSE_INTERNAL_H_

namespace openscreen {

struct Component;

static constexpr char kHttpsScheme[] = "https";
static constexpr char kHttpScheme[] = "http";
static constexpr char kFileScheme[] = "file";
static constexpr char kFtpScheme[] = "ftp";
static constexpr char kWssScheme[] = "wss";
static constexpr char kWsScheme[] = "ws";
static constexpr char kFileSystemScheme[] = "filesystem";
static constexpr char kMailtoScheme[] = "mailto";

// Returns whether the character |ch| should be treated as a slash.
bool IsURLSlash(char ch);

// Returns whether the character |ch| can be safely removed for the URL.
bool ShouldTrimFromURL(char ch);

// Given an already-initialized begin index and length, this shrinks the range
// to eliminate "should-be-trimmed" characters. Note that the length does *not*
// indicate the length of untrimmed data from |*begin|, but rather the position
// in the input string (so the string starts at character |*begin| in the spec,
// and goes until |*len|).
void TrimURL(const char* spec, int* begin, int* len, bool trim_path_end = true);

// Returns the number of consecutive slashes in |str| starting from offset
// |begin_offset|.
int CountConsecutiveSlashes(const char* str, int begin_offset, int str_len);

// Given a string and a range inside the string, compares it to the given
// lower-case |compare_to| buffer.
bool CompareSchemeComponent(const char* spec,
                            const Component& component,
                            const char* compare_to);

// Returns whether the scheme given by (spec, component) is a standard scheme
// (i.e. https://url.spec.whatwg.org/#special-scheme).
bool IsStandard(const char* spec, const Component& component);

}  // namespace openscreen

#endif  // THIRD_PARTY_MOZILLA_URL_PARSE_INTERNAL_H_
