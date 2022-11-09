/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Windows-specific routines for verifying that Data Execution Prevention is
 * functional.
 */

#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"

int NaClAttemptToExecuteDataAtAddr(uint8_t *thunk_buffer, size_t size) {
  /*
   * All supported versions of Windows have DEP. No need to test this.
   */
  return 1;
}

/*
 * Returns 1 if Data Execution Prevention is present and working.
 */
int NaClAttemptToExecuteData(void) {
  /*
   * All supported versions of Windows have DEP. No need to test this.
   */
  return 1;
}
