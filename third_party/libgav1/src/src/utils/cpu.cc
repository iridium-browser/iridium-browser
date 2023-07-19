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

#include "src/utils/cpu.h"

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#include <cpuid.h>
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <immintrin.h>  // _xgetbv
#include <intrin.h>
#endif

namespace libgav1 {

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
    defined(_M_X64)
namespace {

#if defined(__GNUC__)
void CpuId(int leaf, uint32_t info[4]) {
  __cpuid_count(leaf, 0 /*ecx=subleaf*/, info[0], info[1], info[2], info[3]);
}

uint64_t Xgetbv() {
  const uint32_t ecx = 0;  // ecx specifies the extended control register
  uint32_t eax;
  uint32_t edx;
  __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(ecx));
  return (static_cast<uint64_t>(edx) << 32) | eax;
}
#else   // _MSC_VER
void CpuId(int leaf, uint32_t info[4]) {
  __cpuidex(reinterpret_cast<int*>(info), leaf, 0 /*ecx=subleaf*/);
}

uint64_t Xgetbv() { return _xgetbv(0); }
#endif  // __GNUC__

}  // namespace

uint32_t GetCpuInfo() {
  uint32_t info[4];

  // Get the highest feature value cpuid supports
  CpuId(0, info);
  const int max_cpuid_value = info[0];
  if (max_cpuid_value < 1) return 0;

  CpuId(1, info);
  uint32_t features = 0;
  if ((info[3] & (1 << 26)) != 0) features |= kSSE2;
  if ((info[2] & (1 << 9)) != 0) features |= kSSSE3;
  if ((info[2] & (1 << 19)) != 0) features |= kSSE4_1;

  // Bits 27 (OSXSAVE) & 28 (256-bit AVX)
  if ((info[2] & (3 << 27)) == (3 << 27)) {
    // XMM state and YMM state enabled by the OS
    if ((Xgetbv() & 0x6) == 0x6) {
      features |= kAVX;
      if (max_cpuid_value >= 7) {
        CpuId(7, info);
        if ((info[1] & (1 << 5)) != 0) features |= kAVX2;
      }
    }
  }

  return features;
}
#else
uint32_t GetCpuInfo() { return 0; }
#endif  // x86 || x86_64

}  // namespace libgav1
