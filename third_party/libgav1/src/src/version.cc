// Copyright 2019 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/gav1/version.h"

#define LIBGAV1_TOSTRING(x) #x
#define LIBGAV1_STRINGIFY(x) LIBGAV1_TOSTRING(x)
#define LIBGAV1_DOT_SEPARATED(M, m, p) M##.##m##.##p
#define LIBGAV1_DOT_SEPARATED_VERSION(M, m, p) LIBGAV1_DOT_SEPARATED(M, m, p)
#define LIBGAV1_DOT_VERSION                                                   \
  LIBGAV1_DOT_SEPARATED_VERSION(LIBGAV1_MAJOR_VERSION, LIBGAV1_MINOR_VERSION, \
                                LIBGAV1_PATCH_VERSION)

#define LIBGAV1_VERSION_STRING LIBGAV1_STRINGIFY(LIBGAV1_DOT_VERSION)

extern "C" {

int Libgav1GetVersion() { return LIBGAV1_VERSION; }
const char* Libgav1GetVersionString() { return LIBGAV1_VERSION_STRING; }

const char* Libgav1GetBuildConfiguration() {
  // TODO(jzern): cmake can generate the detail or in other cases we could
  // produce one based on the known defines along with the defaults based on
  // the toolchain, e.g., LIBGAV1_ENABLE_NEON from cpu.h.
  return "Not available.";
}

}  // extern "C"
