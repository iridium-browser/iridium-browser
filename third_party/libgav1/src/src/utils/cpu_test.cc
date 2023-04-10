// Copyright 2021 The libgav1 Authors
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

#if defined(__linux__)
#include <unistd.h>

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#endif  // defined(__linux__)

#include "gtest/gtest.h"
#include "src/utils/logging.h"

namespace libgav1 {
namespace {

#if defined(__linux__)

// Sample code for getting the number of performance CPU cores. The following
// sources were consulted:
// * https://www.kernel.org/doc/html/latest/admin-guide/cputopology.html
// * cpu-hotplug.txt: CPU hotplug Support in Linux(tm) Kernel
//   https://lwn.net/Articles/537570/
// * https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-devices-system-cpu
// * Android bionic source code of get_nprocs():
//   libc/bionic/sysinfo.cpp
// * glibc 2.30 source code of get_nprocs():
//   sysdeps/unix/sysv/linux/getsysstats.c
//
// Tested on:
// * Asus Nexus 7 2013: Qualcomm Snapdragon 600, 32-bit Android 6.0.1
//   (Marshmallow). Brings cores online and offline dynamically. (The tablet
//   has 4 cores. "0", "0-1", "0-2", and "0-3" have all been observed in the
//   /sys/devices/system/cpu/online file.) This causes the number of cores
//   currently online to potentially be lower than the number of cores that can
//   be brought online quickly.
// * General Mobile 4G: Qualcomm Snapdragon 410, 32-bit Android 7.1.1 (Nougat).
// * Motorola Moto G5 Plus: Qualcomm Snapdragon 625, 32-bit Android 8.1.0
//   (Oreo).
// * Motorola Moto G7 Play: Qualcomm Snapdragon 632, 32-bit Android 9 (Pie).
//   All 8 cores have the same cpuinfo_max_freq (1804800), but there are two
//   values of cpuinfo_min_freq: cores 0-3 have 614400 and cores 4-7 have
//   633600. We would need to check cpuinfo_min_freq to differentiate the two
//   kinds of cores (Qualcomm Kryo 250 Gold and Qualcomm Kryo 250 Silver).
// * Pixel 2 XL: Qualcomm Snapdragon 835, 64-bit Android 9 (Pie).
// * Pixel 3: Qualcomm Snapdragon 845, 64-bit Android 9 (Pie).
// * Pixel 3a: Qualcomm Snapdragon 670, 64-bit Android 9 (Pie).
// * Samsung Galaxy S6: Samsung Exynos 7 Octa (7420), 64-bit Android 7.0
//   (Nougat).
// * Samsung Galaxy S8+ (SM-G955FD): Samsung Exynos 8895, 64-bit Android 8.0.0.
//
// Note: The sample code needs to use the 'long' type because it is the return
// type of the Standard C Library function strtol(). The ClangTidy warnings are
// suppressed with NOLINT(google-runtime-int) comments.

// Returns the number of online processor cores.
int GetNumberOfProcessorsOnline() {
  // See https://developer.android.com/ndk/guides/cpu-features.
  long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);  // NOLINT(google-runtime-int)
  if (num_cpus < 0) {
    LIBGAV1_DLOG(ERROR, "sysconf(_SC_NPROCESSORS_ONLN) failed: %s.",
                 strerror(errno));
    return 0;
  }
  // It is safe to cast num_cpus to int. sysconf(_SC_NPROCESSORS_ONLN) returns
  // the return value of get_nprocs(), which is an int.
  return static_cast<int>(num_cpus);
}

// These CPUs support heterogeneous multiprocessing.
#if defined(__arm__) || defined(__aarch64__)

// A helper function used by GetNumberOfPerformanceCoresOnline().
//
// Returns the cpuinfo_max_freq value (in kHz) of the given CPU. Returns 0 on
// failure.
long GetCpuinfoMaxFreq(int cpu_index) {  // NOLINT(google-runtime-int)
  char buffer[128];
  const int rv = snprintf(
      buffer, sizeof(buffer),
      "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpu_index);
  if (rv < 0 || rv >= sizeof(buffer)) {
    LIBGAV1_DLOG(ERROR, "snprintf failed, or |buffer| is too small.");
    return 0;
  }
  FILE* file = fopen(buffer, "r");
  if (file == nullptr) {
    LIBGAV1_DLOG(ERROR, "fopen(\"%s\", \"r\") failed: %s.", buffer,
                 strerror(errno));
    return 0;
  }
  char* const str = fgets(buffer, sizeof(buffer), file);
  fclose(file);
  if (str == nullptr) {
    LIBGAV1_DLOG(ERROR, "fgets failed.");
    return 0;
  }
  const long freq = strtol(str, nullptr, 10);  // NOLINT(google-runtime-int)
  if (freq <= 0 || freq == LONG_MAX) {
    LIBGAV1_DLOG(ERROR,
                 "No conversion can be performed, or the converted value is "
                 "invalid: %ld.",
                 freq);
    return 0;
  }
  return freq;
}

// Returns the number of performance CPU cores that are online. The number of
// efficiency CPU cores is subtracted from the total number of CPU cores. Uses
// cpuinfo_max_freq to determine whether a CPU is a performance core or an
// efficiency core.
//
// This function is not perfect. For example, the Snapdragon 632 SoC used in
// Motorola Moto G7 has performance and efficiency cores with the same
// cpuinfo_max_freq but different cpuinfo_min_freq. This function fails to
// differentiate the two kinds of cores and reports all the cores as
// performance cores.
int GetNumberOfPerformanceCoresOnline() {
  // Get the online CPU list. Some examples of the online CPU list are:
  //   "0-7"
  //   "0"
  //   "0-1,2,3,4-7"
  char online[512];
  FILE* file = fopen("/sys/devices/system/cpu/online", "r");
  if (file == nullptr) {
    LIBGAV1_DLOG(ERROR,
                 "fopen(\"/sys/devices/system/cpu/online\", \"r\") failed: %s.",
                 strerror(errno));
    return 0;
  }
  char* const str = fgets(online, sizeof(online), file);
  fclose(file);
  file = nullptr;
  if (str == nullptr) {
    LIBGAV1_DLOG(ERROR, "fgets failed.");
    return 0;
  }
  LIBGAV1_DLOG(INFO, "The online CPU list is %s", online);

  // Count the number of the slowest CPUs. Some SoCs such as Snapdragon 855
  // have performance cores with different max frequencies, so only the slowest
  // CPUs are efficiency cores. If we count the number of the fastest CPUs, we
  // will fail to count the second fastest performance cores.
  long slowest_cpu_freq = LONG_MAX;  // NOLINT(google-runtime-int)
  int num_slowest_cpus = 0;
  int num_cpus = 0;
  const char* cp = online;
  int range_begin = -1;
  while (true) {
    char* str_end;
    const int cpu = static_cast<int>(strtol(cp, &str_end, 10));
    if (str_end == cp) {
      break;
    }
    cp = str_end;
    if (*cp == '-') {
      range_begin = cpu;
    } else {
      if (range_begin == -1) {
        range_begin = cpu;
      }

      num_cpus += cpu - range_begin + 1;
      for (int i = range_begin; i <= cpu; ++i) {
        const long freq = GetCpuinfoMaxFreq(i);  // NOLINT(google-runtime-int)
        if (freq <= 0) {
          return 0;
        }
        LIBGAV1_DLOG(INFO, "cpu%d max frequency is %ld kHz.", i, freq);
        if (freq < slowest_cpu_freq) {
          slowest_cpu_freq = freq;
          num_slowest_cpus = 0;
        }
        if (freq == slowest_cpu_freq) {
          ++num_slowest_cpus;
        }
      }

      range_begin = -1;
    }
    if (*cp == '\0') {
      break;
    }
    ++cp;
  }

  LIBGAV1_DLOG(INFO, "There are %d CPU cores.", num_cpus);
  LIBGAV1_DLOG(INFO,
               "%d CPU cores are the slowest, with max frequency %ld kHz.",
               num_slowest_cpus, slowest_cpu_freq);
  // If there are faster CPU cores than the slowest CPU cores, exclude the
  // slowest CPU cores.
  if (num_slowest_cpus < num_cpus) {
    num_cpus -= num_slowest_cpus;
  }
  return num_cpus;
}

#else

// Assume symmetric multiprocessing.
int GetNumberOfPerformanceCoresOnline() {
  return GetNumberOfProcessorsOnline();
}

#endif

#endif  // defined(__linux__)

/*
  Run this test with logging enabled on an Android device:
  64-bit Android:
    tests/run_android_test.sh --test cpu --enable_asserts
  32-bit Android:
    tests/run_android_test.sh --test cpu --arch arm \
        --enable_asserts
*/
TEST(CpuTest, GetNumberOfPerformanceCoresOnline) {
#if defined(__linux__)
  const int num_cpus = GetNumberOfProcessorsOnline();
  ASSERT_NE(num_cpus, 0);
  LIBGAV1_DLOG(INFO, "There are %d cores online.", num_cpus);
  const int num_performance_cpus = GetNumberOfPerformanceCoresOnline();
  ASSERT_NE(num_performance_cpus, 0);
  LIBGAV1_DLOG(INFO, "There are %d performance cores online.",
               num_performance_cpus);
#endif  // defined(__linux__)
}

}  // namespace
}  // namespace libgav1
