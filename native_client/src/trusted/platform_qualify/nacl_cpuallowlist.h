/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_CPUALLOWLIST
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_CPUALLOWLIST

#include "native_client/src/include/nacl_base.h"

/* NOTES:
 * The blocklist/allowlist go in an array which must be kept SORTED
 * as it is passed to bsearch.
 * An x86 CPU ID string must be 20 bytes plus a '\0'.
 */
#define NACL_BLOCKLIST_TEST_ENTRY "NaClBlocklistTest123"

EXTERN_C_BEGIN

/* Return 1 if CPU is allowlisted */
int NaCl_ThisCPUIsAllowlisted(void);
/* Return 1 if CPU is blocklisted */
int NaCl_ThisCPUIsBlocklisted(void);

/* Return 1 if list is well-structured. */
int NaCl_VerifyBlocklist(void);
int NaCl_VerifyAllowlist(void);

/* Return 1 if named CPU is allowlisted */
int NaCl_CPUIsAllowlisted(const char *myid);
/* Return 1 if named CPU is blocklisted */
int NaCl_CPUIsBlocklisted(const char *myid);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_CPUALLOWLIST */
