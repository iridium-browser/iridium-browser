// Copyright 2012 Google LLC
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifdef HAVE_CONFIG_H
#include <config.h>  // Must come first
#endif

#include "common/mac/arch_utilities.h"

#include <mach-o/arch.h>
#include <mach-o/fat.h>
#include <stdio.h>
#include <string.h>

#ifndef CPU_SUBTYPE_ARM_V7S
#define CPU_SUBTYPE_ARM_V7S (static_cast<cpu_subtype_t>(11))
#endif  // CPU_SUBTYPE_ARM_V7S

#ifndef CPU_TYPE_ARM64
#define CPU_TYPE_ARM64 (CPU_TYPE_ARM | CPU_ARCH_ABI64)
#endif  // CPU_TYPE_ARM64

#ifndef CPU_SUBTYPE_ARM64_ALL
#define CPU_SUBTYPE_ARM64_ALL (static_cast<cpu_subtype_t>(0))
#endif  // CPU_SUBTYPE_ARM64_ALL

#ifndef CPU_SUBTYPE_ARM64_E
#define CPU_SUBTYPE_ARM64_E (static_cast<cpu_subtype_t>(2))
#endif  // CPU_SUBTYPE_ARM64_E

std::optional<ArchInfo> GetArchInfoFromName(const char* arch_name) {
  // TODO: Remove this when the OS knows about arm64.
  if (!strcmp("arm64", arch_name))
    return ArchInfo{CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64_ALL};

  if (!strcmp("arm64e", arch_name))
    return ArchInfo{CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64_E};
  // TODO: Remove this when the OS knows about armv7s.
  if (!strcmp("armv7s", arch_name))
    return ArchInfo{CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S};

  const NXArchInfo* info = NXGetArchInfoFromName(arch_name);
  if (info)
    return ArchInfo{info->cputype, info->cpusubtype};
  return std::nullopt;
}

const char* GetNameFromCPUType(cpu_type_t cpu_type, cpu_subtype_t cpu_subtype) {
  // TODO: Remove this when the OS knows about arm64.
  if (cpu_type == CPU_TYPE_ARM64 && cpu_subtype == CPU_SUBTYPE_ARM64_ALL) {
    return "arm64";
  }

  if (cpu_type == CPU_TYPE_ARM64 && cpu_subtype == CPU_SUBTYPE_ARM64_E) {
    return "arm64e";
  }

  // TODO: Remove this when the OS knows about armv7s.
  if (cpu_type == CPU_TYPE_ARM && cpu_subtype == CPU_SUBTYPE_ARM_V7S) {
    return "armv7s";
  }

  const NXArchInfo* info = NXGetArchInfoFromCpuType(cpu_type, cpu_subtype);
  if (info)
    return info->name;
  return kUnknownArchName;
}

// TODO(crbug.com/1242776): The "#ifndef __APPLE__" should be here, but the
// system version of NXGetLocalArchInfo returns incorrect information on
// x86_64 machines (treating them as just x86), so use the Breakpad version
// all the time for now.
namespace {

enum Architecture {
  kArch_i386 = 0,
  kArch_x86_64,
  kArch_x86_64h,
  kArch_arm,
  kArch_arm64,
  kArch_arm64e,
  kArch_ppc,
  // This must be last.
  kNumArchitectures
};

// enum Architecture above and kKnownArchitectures below
// must be kept in sync.
const NXArchInfo kKnownArchitectures[] = {
  {
    "i386",
    CPU_TYPE_I386,
    CPU_SUBTYPE_I386_ALL,
    NX_LittleEndian,
    "Intel 80x86"
  },
  {
    "x86_64",
    CPU_TYPE_X86_64,
    CPU_SUBTYPE_X86_64_ALL,
    NX_LittleEndian,
    "Intel x86-64"
  },
  {
    "x86_64h",
    CPU_TYPE_X86_64,
    CPU_SUBTYPE_X86_64_H,
    NX_LittleEndian,
    "Intel x86-64h Haswell"
  },
  {
    "arm",
    CPU_TYPE_ARM,
    CPU_SUBTYPE_ARM_ALL,
    NX_LittleEndian,
    "ARM"
  },
  {
    "arm64",
    CPU_TYPE_ARM64,
    CPU_SUBTYPE_ARM64_ALL,
    NX_LittleEndian,
    "ARM64"
  },
  {
    "arm64e",
    CPU_TYPE_ARM64,
    CPU_SUBTYPE_ARM64_E,
    NX_LittleEndian,
    "ARM64e"
  },
  {
    "ppc",
    CPU_TYPE_POWERPC,
    CPU_SUBTYPE_POWERPC_ALL,
    NX_BigEndian,
    "PowerPC"
  }
};

}  // namespace

ArchInfo GetLocalArchInfo(void) {
  Architecture arch;
#if defined(__i386__)
  arch = kArch_i386;
#elif defined(__x86_64__)
  arch = kArch_x86_64;
#elif defined(__arm64)
  arch = kArch_arm64;
#elif defined(__arm__)
  arch = kArch_arm;
#elif defined(__powerpc__)
  arch = kArch_ppc;
#else
  #error "Unsupported CPU architecture"
#endif
  NXArchInfo info = kKnownArchitectures[arch];
  return {info.cputype, info.cpusubtype};
}

#ifndef __APPLE__

const NXArchInfo *NXGetArchInfoFromName(const char *name) {
  for (int arch = 0; arch < kNumArchitectures; ++arch) {
    if (!strcmp(name, kKnownArchitectures[arch].name)) {
      return &kKnownArchitectures[arch];
    }
  }
  return NULL;
}

const NXArchInfo *NXGetArchInfoFromCpuType(cpu_type_t cputype,
                                           cpu_subtype_t cpusubtype) {
  const NXArchInfo *candidate = NULL;
  for (int arch = 0; arch < kNumArchitectures; ++arch) {
    if (kKnownArchitectures[arch].cputype == cputype) {
      if (kKnownArchitectures[arch].cpusubtype == cpusubtype) {
        return &kKnownArchitectures[arch];
      }
      if (!candidate) {
        candidate = &kKnownArchitectures[arch];
      }
    }
  }
  return candidate;
}
#endif  // !__APPLE__
