// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "util/sys_info.h"

#include "base/logging.h"
#include "util/build_config.h"

#if defined(OS_POSIX)
#include <sys/utsname.h>
#include <unistd.h>
#endif

#if defined(OS_WIN)
#include <windows.h>
#endif

std::string OperatingSystemArchitecture() {
#if defined(OS_POSIX)
  struct utsname info;
  if (uname(&info) < 0) {
    NOTREACHED();
    return std::string();
  }
  std::string arch(info.machine);
  std::string os(info.sysname);
  if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") {
    arch = "x86";
  } else if (arch == "i86pc") {
    // Solaris and illumos systems report 'i86pc' (an Intel x86 PC) as their
    // machine for both 32-bit and 64-bit x86 systems.  Considering the rarity
    // of 32-bit systems at this point, it is safe to assume 64-bit.
    arch = "x86_64";
  } else if (arch == "amd64") {
    arch = "x86_64";
  } else if (os == "AIX" || os == "OS400") {
    arch = "ppc64";
  } else if (std::string(info.sysname) == "OS/390") {
    arch = "s390x";
  }
  return arch;
#elif defined(OS_WIN)
  SYSTEM_INFO system_info = {};
  ::GetNativeSystemInfo(&system_info);
  switch (system_info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
      return "x86";
    case PROCESSOR_ARCHITECTURE_AMD64:
      return "x86_64";
    case PROCESSOR_ARCHITECTURE_IA64:
      return "ia64";
  }
  return std::string();
#else
#error
#endif
}

int NumberOfProcessors() {
#if defined(OS_ZOS)
  return __get_num_online_cpus();

#elif defined(OS_POSIX)
  // sysconf returns the number of "logical" (not "physical") processors on both
  // Mac and Linux.  So we get the number of max available "logical" processors.
  //
  // Note that the number of "currently online" processors may be fewer than the
  // returned value of NumberOfProcessors(). On some platforms, the kernel may
  // make some processors offline intermittently, to save power when system
  // loading is low.
  //
  // One common use case that needs to know the processor count is to create
  // optimal number of threads for optimization. It should make plan according
  // to the number of "max available" processors instead of "currently online"
  // ones. The kernel should be smart enough to make all processors online when
  // it has sufficient number of threads waiting to run.
  long res = sysconf(_SC_NPROCESSORS_CONF);
  if (res == -1) {
    NOTREACHED();
    return 1;
  }

  return static_cast<int>(res);
#elif defined(OS_WIN)
  return ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
#else
#error
#endif
}
