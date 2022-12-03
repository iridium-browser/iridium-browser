/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * nacl_cpuallowlist_test.c
 */
#include "native_client/src/include/portability.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/platform_qualify/nacl_cpuallowlist.h"

static void CPUIDAllowlistUnitTests(void) {
  /* blocklist tests */
  if (!NaCl_VerifyBlocklist()) {
    fprintf(stderr, "ERROR: blocklist malformed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsBlocklisted(NACL_BLOCKLIST_TEST_ENTRY)) {
    fprintf(stderr, "ERROR: blocklist test 1 failed\n");
    exit(-1);
  }
  if (NaCl_CPUIsBlocklisted("GenuineFooFooCPU")) {
    fprintf(stderr, "ERROR: blocklist test 2 failed\n");
    exit(-1);
  }
  printf("All blocklist unit tests passed\n");
  /* allowlist tests */
  /* NOTE: allowlist is not currently used */
  if (!NaCl_VerifyAllowlist()) {
    fprintf(stderr, "ERROR: allowlist malformed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsAllowlisted(" FakeEntry0000000000")) {
    fprintf(stderr, "ERROR: allowlist search 1 failed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsAllowlisted("GenuineIntel00000f43")) {
    fprintf(stderr, "ERROR: allowlist search 2 failed\n");
    exit(-1);
  }
  if (!NaCl_CPUIsAllowlisted("zFakeEntry0000000000")) {
    fprintf(stderr, "ERROR: allowlist search 3 failed\n");
    exit(-1);
  }
  if (NaCl_CPUIsAllowlisted("a")) {
    fprintf(stderr, "ERROR: allowlist search 4 didn't fail\n");
    exit(-1);
  }
  if (NaCl_CPUIsAllowlisted("")) {
    fprintf(stderr, "ERROR: allowlist search 5 didn't fail\n");
    exit(-1);
  }
  if (NaCl_CPUIsAllowlisted("zFakeEntry0000000001")) {
    fprintf(stderr, "ERROR: allowlist search 6 didn't fail\n");
    exit(-1);
  }
  if (NaCl_CPUIsAllowlisted("zFakeEntry00000000000")) {
    fprintf(stderr, "ERROR: allowlist search 7 didn't fail\n");
    exit(-1);
  }
  printf("All allowlist unit tests passed\n");
}

int main(void) {
  NaClCPUData data;
  NaClCPUDataGet(&data);
  printf("allow list ID: %s\n", GetCPUIDString(&data));
  if (NaCl_ThisCPUIsAllowlisted()) {
    printf("this CPU is on the allowlist\n");
  } else {
    printf("this CPU is NOT on the allowlist\n");
  }
  if (NaCl_ThisCPUIsBlocklisted()) {
    printf("this CPU is on the blocklist\n");
  } else {
    printf("this CPU is NOT on the blocklist\n");
  }

  CPUIDAllowlistUnitTests();
  return 0;
}
