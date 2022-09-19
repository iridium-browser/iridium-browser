// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cast/standalone_receiver/sdl_video_player.h"

#include <sstream>
#include <utility>

#include "cast/standalone_receiver/avcodec_glue.h"
#include "util/enum_name_table.h"
#include "util/osp_logging.h"
#include "util/trace_logging.h"

namespace openscreen {
namespace cast {

namespace {
constexpr char kVideoMediaType[] = "video";
}  // namespace

constexpr EnumNameTable<VideoCodec, 6> kFfmpegCodecDescriptors{
    {{"h264", VideoCodec::kH264},
     {"vp8", VideoCodec::kVp8},
     {"hevc", VideoCodec::kHevc},
     {"vp9", VideoCodec::kVp9},
     {"libaom-av1", VideoCodec::kAv1}}};

SDLVideoPlayer::SDLVideoPlayer(ClockNowFunctionPtr now_function,
                               TaskRunner* task_runner,
                               Receiver* receiver,
                               VideoCodec codec,
                               SDL_Renderer* renderer,
                               std::function<void()> error_callback)
    : SDLPlayerBase(now_function,
                    task_runner,
                    receiver,
                    GetEnumName(kFfmpegCodecDescriptors, codec).value(),
                    std::move(error_callback),
                    kVideoMediaType),
      renderer_(renderer) {
  OSP_DCHECK(renderer_);
}

SDLVideoPlayer::~SDLVideoPlayer() = default;

bool SDLVideoPlayer::RenderWhileIdle(
    const SDLPlayerBase::PresentableFrame* frame) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  // Attempt to re-render the same content.
  if (state() == kPresented && frame) {
    const auto result = RenderNextFrame(*frame);
    if (result) {
      return true;
    }
    OnFatalError(result.error().message());
    // Fall-through to the "red splash" rendering below.
  }

  if (state() == kError) {
    // Paint "red splash" to indicate an error state.
    constexpr struct { int r = 128, g = 0, b = 0, a = 255; } kRedSplashColor;
    SDL_SetRenderDrawColor(renderer_, kRedSplashColor.r, kRedSplashColor.g,
                           kRedSplashColor.b, kRedSplashColor.a);
    SDL_RenderClear(renderer_);
  } else if (state() == kWaitingForFirstFrame || !frame) {
    // Paint "blue splash" to indicate the "waiting for first frame" state.
    constexpr struct { int r = 0, g = 0, b = 128, a = 255; } kBlueSplashColor;
    SDL_SetRenderDrawColor(renderer_, kBlueSplashColor.r, kBlueSplashColor.g,
                           kBlueSplashColor.b, kBlueSplashColor.a);
    SDL_RenderClear(renderer_);
  }

  return state() != kScheduledToPresent;
}

ErrorOr<Clock::time_point> SDLVideoPlayer::RenderNextFrame(
    const SDLPlayerBase::PresentableFrame& frame) {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  OSP_DCHECK(frame.decoded_frame);
  const AVFrame& picture = *frame.decoded_frame;

  // Punt if the |picture| format is not compatible with those supported by SDL.
  const uint32_t sdl_format = GetSDLPixelFormat(picture);
  if (sdl_format == SDL_PIXELFORMAT_UNKNOWN) {
    std::ostringstream error;
    error << "SDL does not support AVPixelFormat " << picture.format;
    return Error(Error::Code::kUnknownError, error.str());
  }

  // If there is already a SDL texture, check that its format and size matches
  // that of |picture|. If not, release the existing texture.
  if (texture_) {
    uint32_t texture_format = SDL_PIXELFORMAT_UNKNOWN;
    int texture_width = -1;
    int texture_height = -1;
    SDL_QueryTexture(texture_.get(), &texture_format, nullptr, &texture_width,
                     &texture_height);
    if (texture_format != sdl_format || texture_width != picture.width ||
        texture_height != picture.height) {
      texture_.reset();
    }
  }

  // If necessary, recreate a SDL texture having the same format and size as
  // that of |picture|.
  if (!texture_) {
    const auto EvalDescriptionString = [&] {
      std::ostringstream error;
      error << SDL_GetPixelFormatName(sdl_format) << " at " << picture.width
            << "×" << picture.height;
      return error.str();
    };
    OSP_LOG_INFO << "Creating SDL texture for " << EvalDescriptionString();
    texture_ =
        MakeUniqueSDLTexture(renderer_, sdl_format, SDL_TEXTUREACCESS_STREAMING,
                             picture.width, picture.height);
    if (!texture_) {
      std::ostringstream error;
      error << "Unable to (re)create SDL texture for format: "
            << EvalDescriptionString();
      return Error(Error::Code::kUnknownError, error.str());
    }
  }

  // Upload the |picture_| to the SDL texture.
  void* pixels = nullptr;
  int stride = 0;
  SDL_LockTexture(texture_.get(), nullptr, &pixels, &stride);
  const auto picture_format = static_cast<AVPixelFormat>(picture.format);
  const int pixels_size = av_image_get_buffer_size(
      picture_format, picture.width, picture.height, stride);
  constexpr int kSDLTextureRowAlignment = 1;  // SDL doesn't use word-alignment.
  av_image_copy_to_buffer(static_cast<uint8_t*>(pixels), pixels_size,
                          picture.data, picture.linesize, picture_format,
                          picture.width, picture.height,
                          kSDLTextureRowAlignment);
  SDL_UnlockTexture(texture_.get());

  // Render the SDL texture to the render target. Quality-related issues that a
  // production-worthy player should account for that are not being done here:
  //
  // 1. Need to account for AVPicture's sample_aspect_ratio property. Otherwise,
  //    content may appear "squashed" in one direction to the user.
  //
  // 2. SDL has no concept of color space, and so the color information provided
  //    with the AVPicture might not match the assumptions being made within
  //    SDL. Content may appear with washed-out colors, not-entirely-black
  //    blacks, striped gradients, etc.
  const SDL_Rect src_rect = {
      static_cast<int>(picture.crop_left), static_cast<int>(picture.crop_top),
      picture.width - static_cast<int>(picture.crop_left + picture.crop_right),
      picture.height -
          static_cast<int>(picture.crop_top + picture.crop_bottom)};
  SDL_Rect dst_rect = {0, 0, 0, 0};
  SDL_RenderGetLogicalSize(renderer_, &dst_rect.w, &dst_rect.h);
  if (src_rect.w != dst_rect.w || src_rect.h != dst_rect.h) {
    // Make the SDL rendering size the same as the frame's visible size. This
    // lets SDL automatically handle letterboxing and scaling details, so that
    // the video fits within the on-screen window.
    dst_rect.w = src_rect.w;
    dst_rect.h = src_rect.h;
    SDL_RenderSetLogicalSize(renderer_, dst_rect.w, dst_rect.h);
  }
  // Clear with black, for the "letterboxing" borders.
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_.get(), &src_rect, &dst_rect);

  return frame.presentation_time;
}

void SDLVideoPlayer::Present() {
  TRACE_DEFAULT_SCOPED(TraceCategory::kStandaloneReceiver);
  SDL_RenderPresent(renderer_);
}

// static
uint32_t SDLVideoPlayer::GetSDLPixelFormat(const AVFrame& picture) {
  switch (picture.format) {
    case AV_PIX_FMT_NONE:
      break;
    case AV_PIX_FMT_YUV420P:
      return SDL_PIXELFORMAT_IYUV;
    case AV_PIX_FMT_YUYV422:
      return SDL_PIXELFORMAT_YUY2;
    case AV_PIX_FMT_UYVY422:
      return SDL_PIXELFORMAT_UYVY;
    case AV_PIX_FMT_YVYU422:
      return SDL_PIXELFORMAT_YVYU;
    case AV_PIX_FMT_NV12:
      return SDL_PIXELFORMAT_NV12;
    case AV_PIX_FMT_NV21:
      return SDL_PIXELFORMAT_NV21;
    case AV_PIX_FMT_RGB24:
      return SDL_PIXELFORMAT_RGB24;
    case AV_PIX_FMT_BGR24:
      return SDL_PIXELFORMAT_BGR24;
    case AV_PIX_FMT_ARGB:
      return SDL_PIXELFORMAT_ARGB32;
    case AV_PIX_FMT_RGBA:
      return SDL_PIXELFORMAT_RGBA32;
    case AV_PIX_FMT_ABGR:
      return SDL_PIXELFORMAT_ABGR32;
    case AV_PIX_FMT_BGRA:
      return SDL_PIXELFORMAT_BGRA32;
    default:
      break;
  }
  return SDL_PIXELFORMAT_UNKNOWN;
}

}  // namespace cast
}  // namespace openscreen
