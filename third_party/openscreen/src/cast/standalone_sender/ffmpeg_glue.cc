// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_sender/ffmpeg_glue.h"

#include <libavcodec/version.h>

#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace cast {
namespace internal {

AVFormatContext* CreateAVFormatContextForFile(const char* path) {
  AVFormatContext* format_context = nullptr;
#if LIBAVCODEC_VERSION_MAJOR < 59
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  av_register_all();
#pragma GCC diagnostic pop
#endif  // LIBAVCODEC_VERSION_MAJOR < 59

  int result = avformat_open_input(&format_context, path, nullptr, nullptr);
  if (result < 0) {
    OSP_LOG_ERROR << "Cannot open " << path << ": " << AvErrorToString(result);
    return nullptr;
  }
  result = avformat_find_stream_info(format_context, nullptr);
  if (result < 0) {
    avformat_close_input(&format_context);
    OSP_LOG_ERROR << "Cannot find stream info in " << path << ": "
                  << AvErrorToString(result);
    return nullptr;
  }
  return format_context;
}

}  // namespace internal

std::string AvErrorToString(int error_num) {
  std::string out(AV_ERROR_MAX_STRING_SIZE, '\0');
  av_make_error_string(data(out), out.length(), error_num);
  return out;
}

}  // namespace cast
}  // namespace openscreen
