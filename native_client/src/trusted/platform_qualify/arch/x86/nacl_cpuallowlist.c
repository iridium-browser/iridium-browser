/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * An implementation of CPU allowlisting for x86's CPUID identification scheme.
 */

#include "native_client/src/include/portability.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/platform_qualify/nacl_cpuallowlist.h"


static int idcmp(const void *s1, const void *s2) {
  return strncmp((char *)s1, *(char **)s2, kCPUIDStringLength);
}

/* NOTE: The blocklist must be in alphabetical order. */
static const char* const kNaClCpuBlocklist[] = {
/*          1         2        */
/* 12345678901234567890 + '\0' */
  " FakeEntry0000000000",
  " FakeEntry0000000001",
  NACL_BLOCKLIST_TEST_ENTRY,
  "zFakeEntry0000000000",
};

/* NOTE: The allowlist must be in alphabetical order. */
static const char* const kNaClCpuAllowlist[] = {
/*          1         2        */
/* 12345678901234567890 + '\0' */
  " FakeEntry0000000000",
  " FakeEntry0000000001",
  " FakeEntry0000000002",
  " FakeEntry0000000003",
  " FakeEntry0000000004",
  " FakeEntry0000000005",
  "GenuineIntel00000f43",
  "zFakeEntry0000000000",
};



static int VerifyCpuList(const char* const cpulist[], int n) {
  int i;
  /* check lengths */
  for (i = 0; i < n; i++) {
    if (strlen(cpulist[i]) != kCPUIDStringLength - 1) {
      return 0;
    }
  }

  /* check order */
  for (i = 1; i < n; i++) {
    if (strncmp(cpulist[i-1], cpulist[i], kCPUIDStringLength) >= 0) {
      return 0;
    }
  }

  return 1;
}


int NaCl_VerifyBlocklist(void) {
  return VerifyCpuList(kNaClCpuBlocklist, NACL_ARRAY_SIZE(kNaClCpuBlocklist));
}


int NaCl_VerifyAllowlist(void) {
  return VerifyCpuList(kNaClCpuAllowlist, NACL_ARRAY_SIZE(kNaClCpuAllowlist));
}


static int IsCpuInList(const char *myid, const char* const cpulist[], int n) {
  return (NULL != bsearch(myid, cpulist, n, sizeof(char*), idcmp));
}


/* for testing */
int NaCl_CPUIsAllowlisted(const char *myid) {
  return IsCpuInList(myid,
                     kNaClCpuAllowlist,
                     NACL_ARRAY_SIZE(kNaClCpuAllowlist));
}

/* for testing */
int NaCl_CPUIsBlocklisted(const char *myid) {
  return IsCpuInList(myid,
                     kNaClCpuBlocklist,
                     NACL_ARRAY_SIZE(kNaClCpuBlocklist));
}


int NaCl_ThisCPUIsAllowlisted(void) {
  NaClCPUData data;
  const char* myid;
  NaClCPUDataGet(&data);
  myid = GetCPUIDString(&data);
  return IsCpuInList(myid,
                     kNaClCpuAllowlist,
                     NACL_ARRAY_SIZE(kNaClCpuAllowlist));
}

int NaCl_ThisCPUIsBlocklisted(void) {
  NaClCPUData data;
  const char* myid;
  NaClCPUDataGet(&data);
  myid = GetCPUIDString(&data);
  return IsCpuInList(myid,
                     kNaClCpuBlocklist,
                     NACL_ARRAY_SIZE(kNaClCpuBlocklist));
}
