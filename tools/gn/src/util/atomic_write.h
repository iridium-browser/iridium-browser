// Copyright (c) 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_ATOMIC_WRITE_H_
#define TOOLS_GN_ATOMIC_WRITE_H_

#include "base/files/file_path.h"

namespace util {

// Writes the given buffer into the file, overwriting any data that was
// previously there. The write is performed atomically by first writing the
// contents to a temporary file and then moving it into place. Returns the
// number of bytes written, or -1 on error.
int WriteFileAtomically(const base::FilePath& filename,
                        const char* data,
                        int size);

}  // namespace util

#endif  // TOOLS_GN_ATOMIC_WRITE_H_
