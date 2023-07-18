/*
 * Copyright 2021 The libgav1 Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __cplusplus
#error Do not compile this file with a C++ compiler
#endif

// clang-format off
#include "src/gav1/version.h"
// clang-format on

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSERT_EQ(a, b)                                                      \
  do {                                                                       \
    if ((a) != (b)) {                                                        \
      fprintf(stderr, "Assertion failure: (%s) == (%s), at %s:%d\n", #a, #b, \
              __FILE__, __LINE__);                                           \
      fprintf(stderr, "C VersionTest failed\n");                             \
      exit(1);                                                               \
    }                                                                        \
  } while (0)

#define ASSERT_NE(a, b)                                                      \
  do {                                                                       \
    if ((a) == (b)) {                                                        \
      fprintf(stderr, "Assertion failure: (%s) != (%s), at %s:%d\n", #a, #b, \
              __FILE__, __LINE__);                                           \
      fprintf(stderr, "C VersionTest failed\n");                             \
      exit(1);                                                               \
    }                                                                        \
  } while (0)

#define ASSERT_TRUE(a)                                                   \
  do {                                                                   \
    if (!(a)) {                                                          \
      fprintf(stderr, "Assertion failure: %s, at %s:%d\n", #a, __FILE__, \
              __LINE__);                                                 \
      fprintf(stderr, "C VersionTest failed\n");                         \
      exit(1);                                                           \
    }                                                                    \
  } while (0)

#define ASSERT_FALSE(a)                                                     \
  do {                                                                      \
    if (a) {                                                                \
      fprintf(stderr, "Assertion failure: !(%s), at %s:%d\n", #a, __FILE__, \
              __LINE__);                                                    \
      fprintf(stderr, "C VersionTest failed\n");                            \
      exit(1);                                                              \
    }                                                                       \
  } while (0)

static void VersionTestGetVersion(void) {
  const int library_version = Libgav1GetVersion();
  ASSERT_EQ((library_version >> 24) & 0xff, 0);
  // Note if we link against a shared object there's potential for a mismatch
  // if a different library is loaded at runtime.
  ASSERT_EQ((library_version >> 16) & 0xff, LIBGAV1_MAJOR_VERSION);
  ASSERT_EQ((library_version >> 8) & 0xff, LIBGAV1_MINOR_VERSION);
  ASSERT_EQ(library_version & 0xff, LIBGAV1_PATCH_VERSION);

  const int header_version = LIBGAV1_VERSION;
  ASSERT_EQ((header_version >> 24) & 0xff, 0);
  ASSERT_EQ((header_version >> 16) & 0xff, LIBGAV1_MAJOR_VERSION);
  ASSERT_EQ((header_version >> 8) & 0xff, LIBGAV1_MINOR_VERSION);
  ASSERT_EQ(header_version & 0xff, LIBGAV1_PATCH_VERSION);
}

static void VersionTestGetVersionString(void) {
  const char* version = Libgav1GetVersionString();
  ASSERT_NE(version, NULL);
}

static void VersionTestGetBuildConfiguration(void) {
  const char* config = Libgav1GetBuildConfiguration();
  ASSERT_NE(config, NULL);
}

int main(void) {
  fprintf(stderr, "C VersionTest started\n");
  VersionTestGetVersion();
  VersionTestGetVersionString();
  VersionTestGetBuildConfiguration();
  fprintf(stderr, "C VersionTest passed\n");
  return 0;
}
