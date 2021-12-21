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

#include <cstdlib>
#include <vector>

#include "av1/encoder/cost.h"
#include "av1/encoder/tpl_model.h"
#include "av1/encoder/encoder.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace {

double laplace_prob(double q_step, double b, double zero_bin_ratio,
                    int qcoeff) {
  int abs_qcoeff = abs(qcoeff);
  double z0 = fmax(exp(-zero_bin_ratio / 2 * q_step / b), TPL_EPSILON);
  if (abs_qcoeff == 0) {
    double p0 = 1 - z0;
    return p0;
  } else {
    assert(abs_qcoeff > 0);
    double z = fmax(exp(-q_step / b), TPL_EPSILON);
    double p = z0 / 2 * (1 - z) * pow(z, abs_qcoeff - 1);
    return p;
  }
}
TEST(TplModelTest, ExponentialEntropyBoundaryTest1) {
  double b = 0;
  double q_step = 1;
  double entropy = av1_exponential_entropy(q_step, b);
  EXPECT_NEAR(entropy, 0, 0.00001);
}

TEST(TplModelTest, TransformCoeffEntropyTest1) {
  // Check the consistency between av1_estimate_coeff_entropy() and
  // laplace_prob()
  double b = 1;
  double q_step = 1;
  double zero_bin_ratio = 2;
  for (int qcoeff = -256; qcoeff < 256; ++qcoeff) {
    double rate = av1_estimate_coeff_entropy(q_step, b, zero_bin_ratio, qcoeff);
    double prob = laplace_prob(q_step, b, zero_bin_ratio, qcoeff);
    double ref_rate = -log2(prob);
    EXPECT_DOUBLE_EQ(rate, ref_rate);
  }
}

TEST(TplModelTest, TransformCoeffEntropyTest2) {
  // Check the consistency between av1_estimate_coeff_entropy(), laplace_prob()
  // and av1_laplace_entropy()
  double b = 1;
  double q_step = 1;
  double zero_bin_ratio = 2;
  double est_expected_rate = 0;
  for (int qcoeff = -20; qcoeff < 20; ++qcoeff) {
    double rate = av1_estimate_coeff_entropy(q_step, b, zero_bin_ratio, qcoeff);
    double prob = laplace_prob(q_step, b, zero_bin_ratio, qcoeff);
    est_expected_rate += prob * rate;
  }
  double expected_rate = av1_laplace_entropy(q_step, b, zero_bin_ratio);
  EXPECT_NEAR(expected_rate, est_expected_rate, 0.001);
}

TEST(TplModelTest, DeltaRateCostZeroFlow) {
  // When srcrf_dist equal to recrf_dist, av1_delta_rate_cost should return 0
  int64_t srcrf_dist = 256;
  int64_t recrf_dist = 256;
  int64_t delta_rate = 512;
  int pixel_num = 256;
  int64_t rate_cost =
      av1_delta_rate_cost(delta_rate, recrf_dist, srcrf_dist, pixel_num);
  EXPECT_EQ(rate_cost, 0);
}

// a reference function of av1_delta_rate_cost() with delta_rate using bit as
// basic unit
double ref_delta_rate_cost(int64_t delta_rate, double src_rec_ratio,
                           int pixel_count) {
  assert(src_rec_ratio <= 1 && src_rec_ratio >= 0);
  double bits_per_pixel = (double)delta_rate / pixel_count;
  double p = pow(2, bits_per_pixel);
  double flow_rate_per_pixel =
      sqrt(p * p / (src_rec_ratio * p * p + (1 - src_rec_ratio)));
  double rate_cost = pixel_count * log2(flow_rate_per_pixel);
  return rate_cost;
}

TEST(TplModelTest, DeltaRateCostReference) {
  const int64_t scale = TPL_DEP_COST_SCALE_LOG2 + AV1_PROB_COST_SHIFT;
  std::vector<int64_t> srcrf_dist_arr = { 256, 257, 312 };
  std::vector<int64_t> recrf_dist_arr = { 512, 288, 620 };
  std::vector<int64_t> delta_rate_arr = { 10, 278, 100 };
  for (size_t t = 0; t < srcrf_dist_arr.size(); ++t) {
    int64_t srcrf_dist = srcrf_dist_arr[t];
    int64_t recrf_dist = recrf_dist_arr[t];
    int64_t delta_rate = delta_rate_arr[t];
    int64_t scaled_delta_rate = delta_rate << scale;
    int pixel_count = 256;
    int64_t rate_cost = av1_delta_rate_cost(scaled_delta_rate, recrf_dist,
                                            srcrf_dist, pixel_count);
    rate_cost >>= scale;
    double src_rec_ratio = (double)srcrf_dist / recrf_dist;
    double ref_rate_cost =
        ref_delta_rate_cost(delta_rate, src_rec_ratio, pixel_count);
    EXPECT_NEAR((double)rate_cost, ref_rate_cost, 1);
  }
}

TEST(TplModelTest, GetOverlapAreaHasOverlap) {
  // The block a's area is [10, 17) x [18, 24).
  // The block b's area is [8, 15) x [17, 23).
  // The overlapping area between block a and block b is [10, 15) x [18, 23).
  // Therefore, the size of the area is (15 - 10) * (23 - 18) = 25.
  int row_a = 10;
  int col_a = 18;
  int row_b = 8;
  int col_b = 17;
  int height = 7;
  int width = 6;
  int overlap_area =
      av1_get_overlap_area(row_a, col_a, row_b, col_b, width, height);
  EXPECT_EQ(overlap_area, 25);
}

TEST(TplModelTest, GetOverlapAreaNoOverlap) {
  // The block a's area is [10, 14) x [18, 22).
  // The block b's area is [5, 9) x [5, 9).
  // Threre is no overlapping area between block a and block b.
  // Therefore, the return value should be zero.
  int row_a = 10;
  int col_a = 18;
  int row_b = 5;
  int col_b = 5;
  int height = 4;
  int width = 4;
  int overlap_area =
      av1_get_overlap_area(row_a, col_a, row_b, col_b, width, height);
  EXPECT_EQ(overlap_area, 0);
}

TEST(TPLModelTest, EstimateFrameRateTest) {
  /*
   * Transform size: 16x16
   * Frame count: 16
   * Transform block count: 20
   */
  const int txfm_size = 256;  // 16x16
  const int frame_count = 16;
  int q_index_list[16];
  int valid_list[16];
  TplTxfmStats stats_list[16];

  for (int i = 0; i < frame_count; i++) {
    q_index_list[i] = 1;
    valid_list[i] = 1;
    stats_list[i].txfm_block_count = 8;

    for (int j = 0; j < txfm_size; j++) {
      stats_list[i].abs_coeff_sum[j] = 0;
    }
  }

  double result = av1_estimate_gop_bitrate(q_index_list, frame_count,
                                           stats_list, valid_list, NULL);
  EXPECT_NEAR(result, 0, 0.1);
}

TEST(TPLModelTest, TxfmStatsInitTest) {
  TplTxfmStats tpl_txfm_stats;
  av1_init_tpl_txfm_stats(&tpl_txfm_stats);
  EXPECT_EQ(tpl_txfm_stats.coeff_num, 256);
  EXPECT_EQ(tpl_txfm_stats.txfm_block_count, 0);
  for (int i = 0; i < tpl_txfm_stats.coeff_num; ++i) {
    EXPECT_DOUBLE_EQ(tpl_txfm_stats.abs_coeff_sum[i], 0);
  }
}

TEST(TPLModelTest, TxfmStatsAccumulateTest) {
  TplTxfmStats sub_stats;
  av1_init_tpl_txfm_stats(&sub_stats);
  sub_stats.txfm_block_count = 17;
  for (int i = 0; i < sub_stats.coeff_num; ++i) {
    sub_stats.abs_coeff_sum[i] = i;
  }

  TplTxfmStats accumulated_stats;
  av1_init_tpl_txfm_stats(&accumulated_stats);
  accumulated_stats.txfm_block_count = 13;
  for (int i = 0; i < accumulated_stats.coeff_num; ++i) {
    accumulated_stats.abs_coeff_sum[i] = 5 * i;
  }

  av1_accumulate_tpl_txfm_stats(&sub_stats, &accumulated_stats);
  EXPECT_DOUBLE_EQ(accumulated_stats.txfm_block_count, 30);
  for (int i = 0; i < accumulated_stats.coeff_num; ++i) {
    EXPECT_DOUBLE_EQ(accumulated_stats.abs_coeff_sum[i], 6 * i);
  }
}

TEST(TPLModelTest, TxfmStatsRecordTest) {
  TplTxfmStats stats1;
  TplTxfmStats stats2;
  av1_init_tpl_txfm_stats(&stats1);
  av1_init_tpl_txfm_stats(&stats2);

  tran_low_t coeff[256];
  for (int i = 0; i < 256; ++i) {
    coeff[i] = i;
  }
  av1_record_tpl_txfm_block(&stats1, coeff);
  EXPECT_EQ(stats1.txfm_block_count, 1);

  // we record the same transform block twice for testing purpose
  av1_record_tpl_txfm_block(&stats2, coeff);
  av1_record_tpl_txfm_block(&stats2, coeff);
  EXPECT_EQ(stats2.txfm_block_count, 2);

  EXPECT_EQ(stats1.coeff_num, 256);
  EXPECT_EQ(stats2.coeff_num, 256);
  for (int i = 0; i < 256; ++i) {
    EXPECT_DOUBLE_EQ(stats2.abs_coeff_sum[i], 2 * stats1.abs_coeff_sum[i]);
  }
}

/*
 * Helper method to brute-force search for the closest q_index
 * that achieves the specified bit budget.
 */
int find_gop_q_iterative(double bit_budget, const double *qstep_ratio_list,
                         GF_GROUP gf_group, const int *stats_valid_list,
                         TplTxfmStats *stats_list, int gf_frame_index,
                         aom_bit_depth_t bit_depth) {
  // Brute force iterative method to find the optimal q.
  // Use the result to test against the binary search result.

  // Initial estimate when q = 255
  av1_q_mode_compute_gop_q_indices(gf_frame_index, 255, qstep_ratio_list,
                                   bit_depth, &gf_group, gf_group.q_val);
  double curr_estimate = av1_estimate_gop_bitrate(
      gf_group.q_val, gf_group.size, stats_list, stats_valid_list, NULL);
  double best_estimate_budget_distance = fabs(curr_estimate - bit_budget);
  int best_q = 255;

  // Start at q = 254 because we already have an estimate for q = 255.
  for (int q = 254; q >= 0; q--) {
    av1_q_mode_compute_gop_q_indices(gf_frame_index, q, qstep_ratio_list,
                                     bit_depth, &gf_group, gf_group.q_val);
    curr_estimate = av1_estimate_gop_bitrate(
        gf_group.q_val, gf_group.size, stats_list, stats_valid_list, NULL);
    double curr_estimate_budget_distance = fabs(curr_estimate - bit_budget);
    if (curr_estimate_budget_distance <= best_estimate_budget_distance) {
      best_estimate_budget_distance = curr_estimate_budget_distance;
      best_q = q;
    }
  }
  return best_q;
}

TEST(TplModelTest, QModeEstimateBaseQTest) {
  GF_GROUP gf_group = {};
  gf_group.size = 25;
  TplTxfmStats stats_list[25];
  int q_index_list[25];
  const int gf_group_update_types[25] = { 0, 3, 6, 6, 6, 1, 5, 1, 5, 6, 1, 5, 1,
                                          5, 6, 6, 1, 5, 1, 5, 6, 1, 5, 1, 4 };
  int stats_valid_list[25] = { 0 };
  const int gf_frame_index = 0;
  const aom_bit_depth_t bit_depth = AOM_BITS_8;
  const double scale_factor = 1.0;

  double qstep_ratio_list[25];
  for (int i = 0; i < 25; i++) {
    qstep_ratio_list[i] = 1;
  }

  for (int i = 0; i < gf_group.size; i++) {
    stats_valid_list[i] = 1;
    gf_group.update_type[i] = gf_group_update_types[i];
    stats_list[i].txfm_block_count = 8;

    for (int j = 0; j < 256; j++) {
      stats_list[i].abs_coeff_sum[j] = 1000 + j;
    }
  }

  // Test multiple bit budgets.
  const std::vector<double> bit_budgets = { 0,      100,    1000,   10000,
                                            100000, 300000, 500000, 750000,
                                            800000, DBL_MAX };

  for (double bit_budget : bit_budgets) {
    // Binary search method to find the optimal q.
    const int result = av1_q_mode_estimate_base_q(
        &gf_group, stats_list, stats_valid_list, bit_budget, gf_frame_index,
        bit_depth, scale_factor, qstep_ratio_list, q_index_list, NULL);
    const int test_result = find_gop_q_iterative(
        bit_budget, qstep_ratio_list, gf_group, stats_valid_list, stats_list,
        gf_frame_index, bit_depth);

    if (bit_budget == 0) {
      EXPECT_EQ(result, 255);
    } else if (bit_budget == DBL_MAX) {
      EXPECT_EQ(result, 0);
    }

    EXPECT_EQ(result, test_result);
  }
}

TEST(TplModelTest, ComputeMVDifferenceTest) {
  TplDepFrame tpl_frame_small;
  tpl_frame_small.is_valid = true;
  tpl_frame_small.mi_rows = 4;
  tpl_frame_small.mi_cols = 4;
  tpl_frame_small.stride = 1;
  uint8_t right_shift_small = 1;
  int step_small = 1 << right_shift_small;

  // Test values for motion vectors.
  int mv_vals_small[4] = { 1, 2, 3, 4 };
  int index = 0;

  // 4x4 blocks means we need to allocate a 4 size array.
  // According to av1_tpl_ptr_pos:
  // (row >> right_shift) * stride + (col >> right_shift)
  // (4 >> 1) * 1 + (4 >> 1) = 4
  TplDepStats stats_buf_small[4];
  tpl_frame_small.tpl_stats_ptr = stats_buf_small;

  for (int row = 0; row < tpl_frame_small.mi_rows; row += step_small) {
    for (int col = 0; col < tpl_frame_small.mi_cols; col += step_small) {
      TplDepStats tpl_stats;
      tpl_stats.ref_frame_index[0] = 0;
      int_mv mv;
      mv.as_mv.row = mv_vals_small[index];
      mv.as_mv.col = mv_vals_small[index];
      index++;
      tpl_stats.mv[0] = mv;
      tpl_frame_small.tpl_stats_ptr[av1_tpl_ptr_pos(
          row, col, tpl_frame_small.stride, right_shift_small)] = tpl_stats;
    }
  }

  int_mv result_mv =
      av1_compute_mv_difference(&tpl_frame_small, 1, 1, step_small,
                                tpl_frame_small.stride, right_shift_small);

  // Expect the result to be exactly equal to 1 because this is the difference
  // between neighboring motion vectors in this instance.
  EXPECT_EQ(result_mv.as_mv.row, 1);
  EXPECT_EQ(result_mv.as_mv.col, 1);
}

TEST(TplModelTest, ComputeMVBitsTest) {
  TplDepFrame tpl_frame;
  tpl_frame.is_valid = true;
  tpl_frame.mi_rows = 16;
  tpl_frame.mi_cols = 16;
  tpl_frame.stride = 24;
  uint8_t right_shift = 2;
  int step = 1 << right_shift;
  // Test values for motion vectors.
  int mv_vals_ordered[16] = { 1, 2,  3,  4,  5,  6,  7,  8,
                              9, 10, 11, 12, 13, 14, 15, 16 };
  int mv_vals[16] = { 1, 16, 2, 15, 3, 14, 4, 13, 5, 12, 6, 11, 7, 10, 8, 9 };
  int index = 0;

  // 16x16 blocks means we need to allocate a 100 size array.
  // According to av1_tpl_ptr_pos:
  // (row >> right_shift) * stride + (col >> right_shift)
  // (16 >> 2) * 24 + (16 >> 2) = 100
  TplDepStats stats_buf[100];
  tpl_frame.tpl_stats_ptr = stats_buf;

  for (int row = 0; row < tpl_frame.mi_rows; row += step) {
    for (int col = 0; col < tpl_frame.mi_cols; col += step) {
      TplDepStats tpl_stats;
      tpl_stats.ref_frame_index[0] = 0;
      int_mv mv;
      mv.as_mv.row = mv_vals_ordered[index];
      mv.as_mv.col = mv_vals_ordered[index];
      index++;
      tpl_stats.mv[0] = mv;
      tpl_frame.tpl_stats_ptr[av1_tpl_ptr_pos(row, col, tpl_frame.stride,
                                              right_shift)] = tpl_stats;
    }
  }

  double result = av1_tpl_compute_frame_mv_entropy(&tpl_frame, right_shift);

  // Expect the result to be low because the motion vectors are ordered.
  // The estimation algorithm takes this into account and reduces the cost.
  EXPECT_NEAR(result, 20, 5);

  index = 0;
  for (int row = 0; row < tpl_frame.mi_rows; row += step) {
    for (int col = 0; col < tpl_frame.mi_cols; col += step) {
      TplDepStats tpl_stats;
      tpl_stats.ref_frame_index[0] = 0;
      int_mv mv;
      mv.as_mv.row = mv_vals[index];
      mv.as_mv.col = mv_vals[index];
      index++;
      tpl_stats.mv[0] = mv;
      tpl_frame.tpl_stats_ptr[av1_tpl_ptr_pos(row, col, tpl_frame.stride,
                                              right_shift)] = tpl_stats;
    }
  }

  result = av1_tpl_compute_frame_mv_entropy(&tpl_frame, right_shift);

  // Expect the result to be higher because the vectors are not ordered.
  // Neighboring vectors will have different values, increasing the cost.
  EXPECT_NEAR(result, 70, 5);
}

}  // namespace
