// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CAST_STANDALONE_SENDER_FFMPEG_GLUE_H_
#define CAST_STANDALONE_SENDER_FFMPEG_GLUE_H_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <memory>
#include <string>
#include <utility>

namespace openscreen {
namespace cast {

namespace internal {

// Convenience allocator for a new AVFormatContext, given a file |path|. Returns
// nullptr on error. Note: MakeUniqueAVFormatContext() is the public API.
AVFormatContext* CreateAVFormatContextForFile(const char* path);

}  // namespace internal

// Macro that, for an AVFoo, generates code for:
//
//  using FooUniquePtr = std::unique_ptr<Foo, FooFreer>;
//  FooUniquePtr MakeUniqueFoo(...args...);
#define DEFINE_AV_UNIQUE_PTR(name, create_func, free_func)              \
  namespace internal {                                                  \
  struct name##Freer {                                                  \
    void operator()(name* obj) const {                                  \
      if (obj) {                                                        \
        free_func(&obj);                                                \
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

DEFINE_AV_UNIQUE_PTR(AVFormatContext,
                     ::openscreen::cast::internal::CreateAVFormatContextForFile,
                     avformat_close_input);
DEFINE_AV_UNIQUE_PTR(AVCodecContext,
                     avcodec_alloc_context3,
                     avcodec_free_context);
DEFINE_AV_UNIQUE_PTR(AVPacket, av_packet_alloc, av_packet_free);
DEFINE_AV_UNIQUE_PTR(AVFrame, av_frame_alloc, av_frame_free);
DEFINE_AV_UNIQUE_PTR(SwrContext, swr_alloc, swr_free);

#undef DEFINE_AV_UNIQUE_PTR

// The av_err2str macro uses a compound literal, which is a C99-only feature.
// So instead, we roll our own here.
// TODO(https://issuetracker.google.com/224642520): dedup with standalone
// receiver.
std::string AvErrorToString(int error_num);

// Macros to enable backwards compability codepaths for older versions of
// ffmpeg, where newer versions have deprecated APIs.  Note that ffmpeg defines
// its own FF_API* macros that are related to removing APIs (not deprecating
// them).
#define _LIBAVUTIL_OLD_CHANNEL_LAYOUT (LIBAVUTIL_VERSION_MAJOR < 57)

}  // namespace cast
}  // namespace openscreen

#endif  // CAST_STANDALONE_SENDER_FFMPEG_GLUE_H_
