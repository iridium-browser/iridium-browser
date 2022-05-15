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

#include <array>
#include <algorithm>
#include <memory>
#include <vector>

#include "av1/ratectrl_qmode.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace aom {

void test_gop_display_order(const GopStruct &gop_struct) {
  // Test whether show frames' order indices are sequential
  int ref_order_idx = 0;
  for (const auto &gop_frame : gop_struct.gop_frame_list) {
    if (gop_frame.is_show_frame) {
      EXPECT_EQ(gop_frame.order_idx, ref_order_idx);
      ref_order_idx++;
    }
  }
}

void test_colocated_show_frame(const GopStruct &gop_struct) {
  // Test whether each non show frame has a colocated show frame
  int gop_size = static_cast<int>(gop_struct.gop_frame_list.size());
  for (int gop_idx = 0; gop_idx < gop_size; ++gop_idx) {
    auto &gop_frame = gop_struct.gop_frame_list[gop_idx];
    if (gop_frame.is_show_frame == 0) {
      bool found_colocated_ref_frame = false;
      for (int i = gop_idx + 1; i < gop_size; ++i) {
        auto &next_gop_frame = gop_struct.gop_frame_list[i];
        if (gop_frame.order_idx == next_gop_frame.order_idx) {
          found_colocated_ref_frame = true;
          EXPECT_EQ(gop_frame.update_ref_idx, next_gop_frame.colocated_ref_idx);
          EXPECT_TRUE(next_gop_frame.is_show_frame);
        }
        if (gop_frame.update_ref_idx == next_gop_frame.update_ref_idx) {
          break;
        }
      }
      EXPECT_TRUE(found_colocated_ref_frame);
    }
  }
}

void test_layer_depth(const GopStruct &gop_struct, int max_layer_depth) {
  int gop_size = static_cast<int>(gop_struct.gop_frame_list.size());
  for (int gop_idx = 0; gop_idx < gop_size; ++gop_idx) {
    const auto &gop_frame = gop_struct.gop_frame_list[gop_idx];
    if (gop_frame.is_key_frame) {
      EXPECT_EQ(gop_frame.layer_depth, 0);
    }

    if (gop_frame.is_arf_frame) {
      EXPECT_LT(gop_frame.layer_depth, max_layer_depth);
    }

    if (!gop_frame.is_key_frame && !gop_frame.is_arf_frame) {
      EXPECT_EQ(gop_frame.layer_depth, max_layer_depth);
    }
  }
}

void test_arf_interval(const GopStruct &gop_struct) {
  std::vector<int> arf_order_idx_list;
  for (const auto &gop_frame : gop_struct.gop_frame_list) {
    if (gop_frame.is_arf_frame) {
      arf_order_idx_list.push_back(gop_frame.order_idx);
    }
  }
  std::sort(arf_order_idx_list.begin(), arf_order_idx_list.end());
  int arf_count = static_cast<int>(arf_order_idx_list.size());
  for (int i = 1; i < arf_count; ++i) {
    int arf_interval = arf_order_idx_list[i] - arf_order_idx_list[i - 1];
    EXPECT_GE(arf_interval, kMinArfInterval);
  }
}

TEST(RateControlQModeTest, ConstructGopARF) {
  int show_frame_count = 16;
  const int max_ref_frames = 8;
  const bool has_key_frame = false;
  RefFrameManager ref_frame_manager(max_ref_frames);
  GopStruct gop_struct =
      construct_gop(&ref_frame_manager, show_frame_count, has_key_frame);
  test_gop_display_order(gop_struct);
  test_colocated_show_frame(gop_struct);
  const int max_layer_depth =
      ref_frame_manager.ForwardMaxSize() + kLayerDepthOffset;
  test_layer_depth(gop_struct, max_layer_depth);
  test_arf_interval(gop_struct);
}

TEST(RateControlQModeTest, ConstructGopKey) {
  int show_frame_count = 16;
  int max_ref_frames = 8;
  int has_key_frame = 1;
  RefFrameManager ref_frame_manager(max_ref_frames);
  GopStruct gop_struct =
      construct_gop(&ref_frame_manager, show_frame_count, has_key_frame);
  test_gop_display_order(gop_struct);
  test_colocated_show_frame(gop_struct);
  const int max_layer_depth =
      ref_frame_manager.ForwardMaxSize() + kLayerDepthOffset;
  test_layer_depth(gop_struct, max_layer_depth);
  test_arf_interval(gop_struct);
}

static TplBlockStats create_toy_tpl_block_stats(int h, int w, int r, int c,
                                                int cost_diff) {
  TplBlockStats tpl_block_stats = {};
  tpl_block_stats.height = h;
  tpl_block_stats.width = w;
  tpl_block_stats.row = r;
  tpl_block_stats.col = c;
  // A random trick that makes inter_cost - intra_cost = cost_diff;
  tpl_block_stats.intra_cost = cost_diff / 2;
  tpl_block_stats.inter_cost = cost_diff + cost_diff / 2;
  tpl_block_stats.ref_frame_index = { -1, -1 };
  return tpl_block_stats;
}

static TplFrameStats create_toy_tpl_frame_stats_with_diff_sizes(
    int min_block_size, int max_block_size) {
  TplFrameStats frame_stats;
  const int max_h = max_block_size;
  const int max_w = max_h;
  const int count = max_block_size / min_block_size;
  frame_stats.min_block_size = min_block_size;
  frame_stats.frame_height = max_h * count;
  frame_stats.frame_width = max_w * count;
  for (int i = 0; i < count; ++i) {
    for (int j = 0; j < count; ++j) {
      int h = max_h >> i;
      int w = max_w >> j;
      for (int u = 0; u * h < max_h; ++u) {
        for (int v = 0; v * w < max_w; ++v) {
          int r = max_h * i + h * u;
          int c = max_w * j + w * v;
          int cost_diff = std::rand() % 16;
          TplBlockStats block_stats =
              create_toy_tpl_block_stats(h, w, r, c, cost_diff);
          frame_stats.block_stats_list.push_back(block_stats);
        }
      }
    }
  }
  return frame_stats;
}

static void augment_tpl_frame_stats_with_ref_frames(
    TplFrameStats *tpl_frame_stats,
    const std::array<int, kBlockRefCount> &ref_frame_index) {
  for (auto &block_stats : tpl_frame_stats->block_stats_list) {
    block_stats.ref_frame_index = ref_frame_index;
  }
}
static void augment_tpl_frame_stats_with_motion_vector(
    TplFrameStats *tpl_frame_stats,
    const std::array<MotionVector, kBlockRefCount> &mv) {
  for (auto &block_stats : tpl_frame_stats->block_stats_list) {
    block_stats.mv = mv;
  }
}

static RefFrameTable create_toy_ref_frame_table(int frame_count) {
  RefFrameTable ref_frame_table;
  const int ref_frame_table_size = static_cast<int>(ref_frame_table.size());
  EXPECT_LE(frame_count, ref_frame_table_size);
  for (int i = 0; i < frame_count; ++i) {
    ref_frame_table[i] = gop_frame_basic(i, i, 0, 0, 0, 1, 0);
  }
  for (int i = frame_count; i < ref_frame_table_size; ++i) {
    ref_frame_table[i] = gop_frame_invalid();
  }
  return ref_frame_table;
}

static MotionVector create_fullpel_mv(int row, int col) {
  return { row, col, 0 };
}

TEST(RateControlQModeTest, CreateTplFrameDepStats) {
  TplFrameStats frame_stats = create_toy_tpl_frame_stats_with_diff_sizes(8, 16);
  TplFrameDepStats frame_dep_stats =
      create_tpl_frame_dep_stats_wo_propagation(frame_stats);
  EXPECT_EQ(frame_stats.min_block_size, frame_dep_stats.unit_size);
  const int unit_rows = static_cast<int>(frame_dep_stats.unit_stats.size());
  const int unit_cols = static_cast<int>(frame_dep_stats.unit_stats[0].size());
  EXPECT_EQ(frame_stats.frame_height, unit_rows * frame_dep_stats.unit_size);
  EXPECT_EQ(frame_stats.frame_width, unit_cols * frame_dep_stats.unit_size);
  const double sum_cost_diff = tpl_frame_dep_stats_accumulate(frame_dep_stats);

  const double ref_sum_cost_diff = tpl_frame_stats_accumulate(frame_stats);
  EXPECT_NEAR(sum_cost_diff, ref_sum_cost_diff, 0.0000001);
}

TEST(RateControlQModeTest, GetBlockOverlapArea) {
  const int size = 8;
  const int r0 = 8;
  const int c0 = 9;
  std::vector<int> r1 = { 8, 10, 16, 10, 8, 100 };
  std::vector<int> c1 = { 9, 12, 17, 5, 100, 9 };
  std::vector<int> ref_overlap = { 64, 30, 0, 24, 0, 0 };
  for (int i = 0; i < static_cast<int>(r1.size()); ++i) {
    const int overlap0 = get_block_overlap_area(r0, c0, r1[i], c1[i], size);
    const int overlap1 = get_block_overlap_area(r1[i], c1[i], r0, c0, size);
    EXPECT_EQ(overlap0, ref_overlap[i]);
    EXPECT_EQ(overlap1, ref_overlap[i]);
  }
}

TEST(RateControlQModeTest, TplFrameDepStatsPropagateSingleZeroMotion) {
  // cur frame with coding_idx 1 use ref frame with coding_idx 0
  const std::array<int, kBlockRefCount> ref_frame_index = { 0, -1 };
  TplFrameStats frame_stats = create_toy_tpl_frame_stats_with_diff_sizes(8, 16);
  augment_tpl_frame_stats_with_ref_frames(&frame_stats, ref_frame_index);

  TplGopDepStats gop_dep_stats;
  const int frame_count = 2;
  // ref frame with coding_idx 0
  TplFrameDepStats frame_dep_stats0 = create_tpl_frame_dep_stats_empty(
      frame_stats.frame_height, frame_stats.frame_width,
      frame_stats.min_block_size);
  gop_dep_stats.frame_dep_stats_list.push_back(frame_dep_stats0);

  // cur frame with coding_idx 1
  const TplFrameDepStats frame_dep_stats1 =
      create_tpl_frame_dep_stats_wo_propagation(frame_stats);
  gop_dep_stats.frame_dep_stats_list.push_back(frame_dep_stats1);

  const RefFrameTable ref_frame_table = create_toy_ref_frame_table(frame_count);
  tpl_frame_dep_stats_propagate(frame_stats, ref_frame_table, &gop_dep_stats);

  // cur frame with coding_idx 1
  const double ref_sum_cost_diff = tpl_frame_stats_accumulate(frame_stats);

  // ref frame with coding_idx 0
  const double sum_cost_diff =
      tpl_frame_dep_stats_accumulate(gop_dep_stats.frame_dep_stats_list[0]);

  // The sum_cost_diff between coding_idx 0 and coding_idx 1 should be equal
  // because every block in cur frame has zero motion, use ref frame with
  // coding_idx 0 for prediction, and ref frame itself is empty.
  EXPECT_NEAR(sum_cost_diff, ref_sum_cost_diff, 0.0000001);
}

TEST(RateControlQModeTest, TplFrameDepStatsPropagateCompoundZeroMotion) {
  // cur frame with coding_idx 2 use two ref frames with coding_idx 0 and 1
  const std::array<int, kBlockRefCount> ref_frame_index = { 0, 1 };
  TplFrameStats frame_stats = create_toy_tpl_frame_stats_with_diff_sizes(8, 16);
  augment_tpl_frame_stats_with_ref_frames(&frame_stats, ref_frame_index);

  TplGopDepStats gop_dep_stats;
  const int frame_count = 3;
  // ref frame with coding_idx 0
  const TplFrameDepStats frame_dep_stats0 = create_tpl_frame_dep_stats_empty(
      frame_stats.frame_height, frame_stats.frame_width,
      frame_stats.min_block_size);
  gop_dep_stats.frame_dep_stats_list.push_back(frame_dep_stats0);

  // ref frame with coding_idx 1
  const TplFrameDepStats frame_dep_stats1 = create_tpl_frame_dep_stats_empty(
      frame_stats.frame_height, frame_stats.frame_width,
      frame_stats.min_block_size);
  gop_dep_stats.frame_dep_stats_list.push_back(frame_dep_stats1);

  // cur frame with coding_idx 2
  const TplFrameDepStats frame_dep_stats2 =
      create_tpl_frame_dep_stats_wo_propagation(frame_stats);
  gop_dep_stats.frame_dep_stats_list.push_back(frame_dep_stats2);

  const RefFrameTable ref_frame_table = create_toy_ref_frame_table(frame_count);
  tpl_frame_dep_stats_propagate(frame_stats, ref_frame_table, &gop_dep_stats);

  // cur frame with coding_idx 1
  const double ref_sum_cost_diff = tpl_frame_stats_accumulate(frame_stats);

  // ref frame with coding_idx 0
  const double sum_cost_diff0 =
      tpl_frame_dep_stats_accumulate(gop_dep_stats.frame_dep_stats_list[0]);
  EXPECT_NEAR(sum_cost_diff0, ref_sum_cost_diff * 0.5, 0.0000001);

  // ref frame with coding_idx 1
  const double sum_cost_diff1 =
      tpl_frame_dep_stats_accumulate(gop_dep_stats.frame_dep_stats_list[1]);
  EXPECT_NEAR(sum_cost_diff1, ref_sum_cost_diff * 0.5, 0.0000001);
}

TEST(RateControlQModeTest, TplFrameDepStatsPropagateSingleWithMotion) {
  // cur frame with coding_idx 1 use ref frame with coding_idx 0
  const std::array<int, kBlockRefCount> ref_frame_index = { 0, -1 };
  const int min_block_size = 8;
  TplFrameStats frame_stats = create_toy_tpl_frame_stats_with_diff_sizes(
      min_block_size, min_block_size * 2);
  augment_tpl_frame_stats_with_ref_frames(&frame_stats, ref_frame_index);

  const int mv_row = min_block_size / 2;
  const int mv_col = min_block_size / 4;
  const double r_ratio = 1.0 / 2;
  const double c_ratio = 1.0 / 4;
  std::array<MotionVector, kBlockRefCount> mv;
  mv[0] = create_fullpel_mv(mv_row, mv_col);
  mv[1] = create_fullpel_mv(0, 0);
  augment_tpl_frame_stats_with_motion_vector(&frame_stats, mv);

  TplGopDepStats gop_dep_stats;
  const int frame_count = 2;
  // ref frame with coding_idx 0
  gop_dep_stats.frame_dep_stats_list.push_back(create_tpl_frame_dep_stats_empty(
      frame_stats.frame_height, frame_stats.frame_width,
      frame_stats.min_block_size));

  // cur frame with coding_idx 1
  gop_dep_stats.frame_dep_stats_list.push_back(
      create_tpl_frame_dep_stats_wo_propagation(frame_stats));

  const RefFrameTable ref_frame_table = create_toy_ref_frame_table(frame_count);
  tpl_frame_dep_stats_propagate(frame_stats, ref_frame_table, &gop_dep_stats);

  const auto &dep_stats0 = gop_dep_stats.frame_dep_stats_list[0];
  const auto &dep_stats1 = gop_dep_stats.frame_dep_stats_list[1];
  const int unit_rows = static_cast<int>(dep_stats0.unit_stats.size());
  const int unit_cols = static_cast<int>(dep_stats0.unit_stats[0].size());
  for (int r = 0; r < unit_rows; ++r) {
    for (int c = 0; c < unit_cols; ++c) {
      double ref_value = 0;
      ref_value += (1 - r_ratio) * (1 - c_ratio) * dep_stats1.unit_stats[r][c];
      if (r - 1 >= 0) {
        ref_value += r_ratio * (1 - c_ratio) * dep_stats1.unit_stats[r - 1][c];
      }
      if (c - 1 >= 0) {
        ref_value += (1 - r_ratio) * c_ratio * dep_stats1.unit_stats[r][c - 1];
      }
      if (r - 1 >= 0 && c - 1 >= 0) {
        ref_value += r_ratio * c_ratio * dep_stats1.unit_stats[r - 1][c - 1];
      }
      EXPECT_NEAR(dep_stats0.unit_stats[r][c], ref_value, 0.0000001);
    }
  }
}

TEST(RateControlQModeTest, ComputeTplGopDepStats) {
  TplGopStats tpl_gop_stats;
  std::vector<RefFrameTable> ref_frame_table_list;
  for (int i = 0; i < 3; i++) {
    // Use the previous frame as reference
    const std::array<int, kBlockRefCount> ref_frame_index = { i - 1, -1 };
    int min_block_size = 8;
    TplFrameStats frame_stats = create_toy_tpl_frame_stats_with_diff_sizes(
        min_block_size, min_block_size * 2);
    augment_tpl_frame_stats_with_ref_frames(&frame_stats, ref_frame_index);
    tpl_gop_stats.frame_stats_list.push_back(frame_stats);

    ref_frame_table_list.push_back(create_toy_ref_frame_table(i));
  }
  const TplGopDepStats &gop_dep_stats =
      compute_tpl_gop_dep_stats(tpl_gop_stats, ref_frame_table_list);

  double ref_sum = 0;
  for (int i = 2; i >= 0; i--) {
    // Due to the linear propagation with zero motion, we can add the
    // frame_stats value and use it as reference sum for dependency stats
    ref_sum += tpl_frame_stats_accumulate(tpl_gop_stats.frame_stats_list[i]);
    const double sum =
        tpl_frame_dep_stats_accumulate(gop_dep_stats.frame_dep_stats_list[i]);
    EXPECT_NEAR(sum, ref_sum, 0.0000001);
    break;
  }
}

}  // namespace aom

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  std::srand(0);
  return RUN_ALL_TESTS();
}
