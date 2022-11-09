// Copyright 2019 Google LLC.
// Use of this source code is governed by the BSD-3-Clause license that can be
// found in the LICENSE.md file.

#ifndef SkGifCodec_DEFINED
#define SkGifCodec_DEFINED

#include "include/codec/SkCodec.h"

namespace SkGifCodec {

// Returns true if the span of bytes appears to be GIF encoded data.
bool IsGif(const void*, size_t);

// Assumes IsGif was called and returned true.
// Reads enough of the stream to determine the image format.
std::unique_ptr<SkCodec> MakeFromStream(std::unique_ptr<SkStream>, SkCodec::Result*);

}  // namespace SkGifCodec
#endif  // SkGifCodec_DEFINED
