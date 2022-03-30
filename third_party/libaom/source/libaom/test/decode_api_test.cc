/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

#include "config/aom_config.h"

#include "aom/aomdx.h"
#include "aom/aom_decoder.h"

namespace {

TEST(DecodeAPI, InvalidParams) {
  uint8_t buf[1] = { 0 };
  aom_codec_ctx_t dec;

  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_dec_init(NULL, NULL, NULL, 0));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_dec_init(&dec, NULL, NULL, 0));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_decode(NULL, NULL, 0, NULL));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_decode(NULL, buf, 0, NULL));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM,
            aom_codec_decode(NULL, buf, sizeof(buf), NULL));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM,
            aom_codec_decode(NULL, NULL, sizeof(buf), NULL));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_destroy(NULL));
  EXPECT_TRUE(aom_codec_error(NULL) != NULL);
  EXPECT_TRUE(aom_codec_error_detail(NULL) == NULL);

  aom_codec_iface_t *iface = aom_codec_av1_dx();
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_dec_init(NULL, iface, NULL, 0));

  EXPECT_EQ(AOM_CODEC_OK, aom_codec_dec_init(&dec, iface, NULL, 0));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM,
            aom_codec_decode(&dec, NULL, sizeof(buf), NULL));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_decode(&dec, buf, 0, NULL));

  EXPECT_EQ(AOM_CODEC_OK, aom_codec_destroy(&dec));
}

TEST(DecodeAPI, InvalidControlId) {
  aom_codec_iface_t *iface = aom_codec_av1_dx();
  aom_codec_ctx_t dec;
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_dec_init(&dec, iface, NULL, 0));
  EXPECT_EQ(AOM_CODEC_ERROR, aom_codec_control(&dec, -1, 0));
  EXPECT_EQ(AOM_CODEC_INVALID_PARAM, aom_codec_control(&dec, 0, 0));
  EXPECT_EQ(AOM_CODEC_OK, aom_codec_destroy(&dec));
}

}  // namespace
