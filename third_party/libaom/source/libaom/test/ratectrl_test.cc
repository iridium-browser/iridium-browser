/*
 * Copyright (c) 2021, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "av1/encoder/firstpass.h"
#include "av1/encoder/ratectrl.h"
#include "av1/encoder/tpl_model.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace {

TEST(RatectrlTest, QModeGetQIndexTest) {
  int base_q_index = 36;
  int gf_update_type = INTNL_ARF_UPDATE;
  int gf_pyramid_level = 1;
  int arf_q = 100;
  int q_index = av1_q_mode_get_q_index(base_q_index, gf_update_type,
                                       gf_pyramid_level, arf_q);
  EXPECT_EQ(q_index, arf_q);

  gf_update_type = INTNL_ARF_UPDATE;
  gf_pyramid_level = 3;
  q_index = av1_q_mode_get_q_index(base_q_index, gf_update_type,
                                   gf_pyramid_level, arf_q);
  EXPECT_LT(q_index, arf_q);

  gf_update_type = LF_UPDATE;
  q_index = av1_q_mode_get_q_index(base_q_index, gf_update_type,
                                   gf_pyramid_level, arf_q);
  EXPECT_EQ(q_index, base_q_index);
}

#if !CONFIG_REALTIME_ONLY
// TODO(angiebird): Move this test to tpl_mode_test.cc
TEST(RatectrlTest, QModeComputeGOPQIndicesTest) {
  const int base_q_index = 80;
  double qstep_ratio_list[5] = { 0.5, 1, 1, 1, 0.5 };
  const aom_bit_depth_t bit_depth = AOM_BITS_8;

  const int gf_frame_index = 0;
  GF_GROUP gf_group = {};
  gf_group.size = 5;
  const int layer_depth[5] = { 1, 3, 2, 3, 1 };
  const int update_type[5] = { KF_UPDATE, INTNL_ARF_UPDATE,
                               INTNL_OVERLAY_UPDATE, INTNL_ARF_UPDATE,
                               ARF_UPDATE };

  for (int i = 0; i < gf_group.size; i++) {
    gf_group.layer_depth[i] = layer_depth[i];
    gf_group.update_type[i] = update_type[i];
  }

  const int arf_q = av1_get_q_index_from_qstep_ratio(
      base_q_index, qstep_ratio_list[0], bit_depth);

  av1_q_mode_compute_gop_q_indices(gf_frame_index, base_q_index,
                                   qstep_ratio_list, bit_depth, &gf_group,
                                   gf_group.q_val);

  for (int i = 0; i < gf_group.size; i++) {
    if (layer_depth[i] == 1) {
      EXPECT_EQ(gf_group.q_val[i], arf_q);
    } else {
      EXPECT_GT(gf_group.q_val[i], arf_q);
    }
  }
}
#endif  // !CONFIG_REALTIME_ONLY

}  // namespace
