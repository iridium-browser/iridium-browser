// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_RECEIVER_AVCODEC_GLUE_H_
#define CAST_STANDALONE_RECEIVER_AVCODEC_GLUE_H_

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
}

namespace openscreen {
namespace cast {

// Macro that, for an AVFoo, generates code for:
//
//  using FooUniquePtr = std::unique_ptr<Foo, FooFreer>;
//  FooUniquePtr MakeUniqueFoo(...args...);
#define DEFINE_AV_UNIQUE_PTR(name, create_func, free_statement)         \
  namespace internal {                                                  \
  struct name##Freer {                                                  \
    void operator()(name* obj) const {                                  \
      if (obj) {                                                        \
        free_statement;                                                 \
      }                                                                 \
    }                                                                   \
  };                                                                    \
  }                                                                     \
                                                                        \
  using name##UniquePtr = std::unique_ptr<name, internal::name##Freer>; \
                                                                        \
  template <typename... Args>                                           \
  name##UniquePtr MakeUnique##name(Args&&... args) {                    \
    return name##UniquePtr(create_func(std::forward<Args>(args)...));   \
  }

DEFINE_AV_UNIQUE_PTR(AVCodecParserContext,
                     av_parser_init,
                     av_parser_close(obj));
DEFINE_AV_UNIQUE_PTR(AVCodecContext,
                     avcodec_alloc_context3,
                     avcodec_free_context(&obj));
DEFINE_AV_UNIQUE_PTR(AVPacket, av_packet_alloc, av_packet_free(&obj));
DEFINE_AV_UNIQUE_PTR(AVFrame, av_frame_alloc, av_frame_free(&obj));

#undef DEFINE_AV_UNIQUE_PTR

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_RECEIVER_AVCODEC_GLUE_H_
