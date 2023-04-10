/*
 * Copyright (c) 2022, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

// Tests for https://crbug.com/aomedia/3326.
//
// Set cfg.g_forced_max_frame_width and cfg.g_forced_max_frame_height and
// encode two frames of increasing sizes. The second aom_codec_encode() should
// not crash or have memory errors.

#include <memory>

#include "aom/aomcx.h"
#include "aom/aom_encoder.h"
#include "config/aom_config.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace {

// cfg.g_lag_in_frames must be set to 0 or 1 to allow the frame size to change,
// as required by the following check in encoder_set_config() in
// av1/av1_cx_iface.c:
//
//   if (cfg->g_w != ctx->cfg.g_w || cfg->g_h != ctx->cfg.g_h) {
//     if (cfg->g_lag_in_frames > 1 || cfg->g_pass != AOM_RC_ONE_PASS)
//       ERROR("Cannot change width or height after initialization");
//     ...
//   }

void RunTest(unsigned int usage, unsigned int lag_in_frames,
             const char *tune_metric) {
  // A buffer of gray samples. Large enough for 128x128 and 256x256, YUV 4:2:0.
  constexpr size_t kImageDataSize = 256 * 256 + 2 * 128 * 128;
  std::unique_ptr<unsigned char[]> img_data(new unsigned char[kImageDataSize]);
  ASSERT_NE(img_data, nullptr);
  memset(img_data.get(), 128, kImageDataSize);

  aom_codec_iface_t *iface = aom_codec_av1_cx();
  aom_codec_enc_cfg_t cfg;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_enc_config_default(iface, &cfg, usage));
  cfg.g_w = 128;
  cfg.g_h = 128;
  cfg.g_forced_max_frame_width = 256;
  cfg.g_forced_max_frame_height = 256;
  cfg.g_lag_in_frames = lag_in_frames;
  aom_codec_ctx_t enc;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_enc_init(&enc, iface, &cfg, 0));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_set_option(&enc, "tune", tune_metric));

  aom_image_t img;
  EXPECT_EQ(&img,
            aom_img_wrap(&img, AOM_IMG_FMT_I420, 128, 128, 1, img_data.get()));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_encode(&enc, &img, 0, 1, 0));

  cfg.g_w = 256;
  cfg.g_h = 256;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_enc_config_set(&enc, &cfg));

  EXPECT_EQ(&img,
            aom_img_wrap(&img, AOM_IMG_FMT_I420, 256, 256, 1, img_data.get()));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_encode(&enc, &img, 0, 1, 0));

  EXPECT_EQ(AOM_CODEC_OK, aom_codec_encode(&enc, nullptr, 0, 0, 0));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_destroy(&enc));
}

#if !CONFIG_REALTIME_ONLY

TEST(EncodeForcedMaxFrameWidthHeight, GoodQualityLag0TunePSNR) {
  RunTest(AOM_USAGE_GOOD_QUALITY, /*lag_in_frames=*/0, "psnr");
}

TEST(EncodeForcedMaxFrameWidthHeight, GoodQualityLag0TuneSSIM) {
  RunTest(AOM_USAGE_GOOD_QUALITY, /*lag_in_frames=*/0, "ssim");
}

TEST(EncodeForcedMaxFrameWidthHeight, GoodQualityLag1TunePSNR) {
  RunTest(AOM_USAGE_GOOD_QUALITY, /*lag_in_frames=*/1, "psnr");
}

TEST(EncodeForcedMaxFrameWidthHeight, GoodQualityLag1TuneSSIM) {
  RunTest(AOM_USAGE_GOOD_QUALITY, /*lag_in_frames=*/1, "ssim");
}

#endif  // !CONFIG_REALTIME_ONLY

TEST(EncodeForcedMaxFrameWidthHeight, RealtimeLag0TunePSNR) {
  RunTest(AOM_USAGE_REALTIME, /*lag_in_frames=*/0, "psnr");
}

TEST(EncodeForcedMaxFrameWidthHeight, RealtimeLag0TuneSSIM) {
  RunTest(AOM_USAGE_REALTIME, /*lag_in_frames=*/0, "ssim");
}

TEST(EncodeForcedMaxFrameWidthHeight, RealtimeLag1TunePSNR) {
  RunTest(AOM_USAGE_REALTIME, /*lag_in_frames=*/1, "psnr");
}

TEST(EncodeForcedMaxFrameWidthHeight, RealtimeLag1TuneSSIM) {
  RunTest(AOM_USAGE_REALTIME, /*lag_in_frames=*/1, "ssim");
}

TEST(EncodeForcedMaxFrameWidthHeight, MaxFrameSizeTooBig) {
  aom_codec_iface_t *iface = aom_codec_av1_cx();
  aom_codec_enc_cfg_t cfg;
  EXPECT_EQ(AOM_CODEC_OK,
            aom_codec_enc_config_default(iface, &cfg, AOM_USAGE_REALTIME));
  cfg.g_w = 256;
  cfg.g_h = 256;
  cfg.g_forced_max_frame_width = 131072;
  cfg.g_forced_max_frame_height = 131072;
  aom_codec_ctx_t enc;
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_enc_init(&enc, iface, &cfg, 0));
}

TEST(EncodeForcedMaxFrameWidthHeight, FirstFrameTooBig) {
  aom_codec_iface_t *iface = aom_codec_av1_cx();
  aom_codec_enc_cfg_t cfg;
  EXPECT_EQ(AOM_CODEC_OK,
            aom_codec_enc_config_default(iface, &cfg, AOM_USAGE_REALTIME));
  cfg.g_w = 258;
  cfg.g_h = 256;
  cfg.g_forced_max_frame_width = 256;
  cfg.g_forced_max_frame_height = 256;
  aom_codec_ctx_t enc;
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_enc_init(&enc, iface, &cfg, 0));
  cfg.g_w = 256;
  cfg.g_h = 258;
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_enc_init(&enc, iface, &cfg, 0));
  cfg.g_w = 256;
  cfg.g_h = 256;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_enc_init(&enc, iface, &cfg, 0));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_destroy(&enc));
}

TEST(EncodeForcedMaxFrameWidthHeight, SecondFrameTooBig) {
  // A buffer of gray samples. Large enough for 128x128 and 256x256, YUV 4:2:0.
  constexpr size_t kImageDataSize = 256 * 256 + 2 * 128 * 128;
  std::unique_ptr<unsigned char[]> img_data(new unsigned char[kImageDataSize]);
  ASSERT_NE(img_data, nullptr);
  memset(img_data.get(), 128, kImageDataSize);

  aom_codec_iface_t *iface = aom_codec_av1_cx();
  aom_codec_enc_cfg_t cfg;
  EXPECT_EQ(AOM_CODEC_OK,
            aom_codec_enc_config_default(iface, &cfg, AOM_USAGE_REALTIME));
  cfg.g_w = 128;
  cfg.g_h = 128;
  cfg.g_forced_max_frame_width = 255;
  cfg.g_forced_max_frame_height = 256;
  aom_codec_ctx_t enc;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_enc_init(&enc, iface, &cfg, 0));

  aom_image_t img;
  EXPECT_EQ(&img,
            aom_img_wrap(&img, AOM_IMG_FMT_I420, 128, 128, 1, img_data.get()));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_encode(&enc, &img, 0, 1, 0));

  cfg.g_w = 256;
  cfg.g_h = 256;
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_enc_config_set(&enc, &cfg));

  EXPECT_EQ(AOM_CODEC_OK, aom_codec_destroy(&enc));
}

}  // namespace
