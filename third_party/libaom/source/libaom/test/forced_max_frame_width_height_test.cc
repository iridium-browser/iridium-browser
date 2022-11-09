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
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace {

TEST(EncodeForcedMaxFrameWidthHeight, RealtimeTunePSNR) {
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
  cfg.g_forced_max_frame_width = 256;
  cfg.g_forced_max_frame_height = 256;
  aom_codec_ctx_t enc;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_enc_init(&enc, iface, &cfg, 0));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_set_option(&enc, "tune", "psnr"));

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

TEST(EncodeForcedMaxFrameWidthHeight, RealtimeTuneSSIM) {
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
  cfg.g_forced_max_frame_width = 256;
  cfg.g_forced_max_frame_height = 256;
  aom_codec_ctx_t enc;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_enc_init(&enc, iface, &cfg, 0));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_set_option(&enc, "tune", "ssim"));

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

}  // namespace
