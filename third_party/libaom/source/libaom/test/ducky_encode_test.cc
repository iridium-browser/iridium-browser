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
#include <cerrno>
#include <cstring>
#include <fstream>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "av1/encoder/encoder.h"
#include "av1/qmode_rc/ducky_encode.h"
#include "av1/qmode_rc/ratectrl_qmode.h"
#include "av1/qmode_rc/ratectrl_qmode_interface.h"
#include "test/video_source.h"
#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace aom {

constexpr int kMaxRefFrames = 7;

TEST(DuckyEncodeTest, ComputeFirstPassStats) {
  aom_rational_t frame_rate = { 30, 1 };
  VideoInfo video_info = { 352,        288,
                           frame_rate, AOM_IMG_FMT_I420,
                           1,          "bus_352x288_420_f20_b8.yuv" };
  video_info.file_path =
      libaom_test::GetDataPath() + "/" + video_info.file_path;
  DuckyEncode ducky_encode(video_info, BLOCK_64X64, kMaxRefFrames, 3, 128);
  std::vector<FIRSTPASS_STATS> frame_stats =
      ducky_encode.ComputeFirstPassStats();
  EXPECT_EQ(frame_stats.size(), static_cast<size_t>(video_info.frame_count));
  for (size_t i = 0; i < frame_stats.size(); ++i) {
    // FIRSTPASS_STATS's first element is frame
    EXPECT_EQ(frame_stats[i].frame, i);
  }
}

TEST(DuckyEncodeTest, EncodeFrame) {
  aom_rational_t frame_rate = { 30, 1 };
  VideoInfo video_info = { 352,        288,
                           frame_rate, AOM_IMG_FMT_I420,
                           17,         "bus_352x288_420_f20_b8.yuv" };
  video_info.file_path =
      libaom_test::GetDataPath() + "/" + video_info.file_path;
  DuckyEncode ducky_encode(video_info, BLOCK_64X64, kMaxRefFrames, 3, 128);
  std::vector<FIRSTPASS_STATS> frame_stats =
      ducky_encode.ComputeFirstPassStats();
  ducky_encode.StartEncode(frame_stats);
  // We set coding_frame_count to a arbitrary number that smaller than
  // 17 here.
  // TODO(angiebird): Set coding_frame_count properly, once the DuckyEncode can
  // provide proper information.
  int coding_frame_count = 5;
  EncodeFrameDecision decision = { aom::EncodeFrameMode::kNone,
                                   aom::EncodeGopMode::kNone,
                                   {} };
  for (int i = 0; i < coding_frame_count; ++i) {
    ducky_encode.AllocateBitstreamBuffer(video_info);
    EncodeFrameResult encode_frame_result = ducky_encode.EncodeFrame(decision);
  }
  ducky_encode.EndEncode();
}

TEST(DuckyEncodeTest, EncodeFrameWithQindex) {
  aom_rational_t frame_rate = { 30, 1 };
  VideoInfo video_info = { 352,        288,
                           frame_rate, AOM_IMG_FMT_I420,
                           17,         "bus_352x288_420_f20_b8.yuv" };
  video_info.file_path =
      libaom_test::GetDataPath() + "/" + video_info.file_path;
  DuckyEncode ducky_encode(video_info, BLOCK_64X64, kMaxRefFrames, 3, 128);
  std::vector<FIRSTPASS_STATS> frame_stats =
      ducky_encode.ComputeFirstPassStats();
  ducky_encode.StartEncode(frame_stats);
  // We set coding_frame_count to a arbitrary number that smaller than
  // 17 here.
  // TODO(angiebird): Set coding_frame_count properly, once the DuckyEncode can
  // provide proper information.
  int coding_frame_count = 5;
  int q_index = 0;
  EncodeFrameDecision decision = { aom::EncodeFrameMode::kQindex,
                                   aom::EncodeGopMode::kNone,
                                   { q_index, -1, {}, {} } };
  for (int i = 0; i < coding_frame_count; ++i) {
    ducky_encode.AllocateBitstreamBuffer(video_info);
    EncodeFrameResult encode_frame_result = ducky_encode.EncodeFrame(decision);
    EXPECT_EQ(encode_frame_result.dist, 0);
  }
  ducky_encode.EndEncode();
}

TEST(DuckyEncodeRCTest, EncodeVideoWithRC) {
  aom_rational_t frame_rate = { 30, 1 };
  const int frame_number = 35;
  const int frame_width = 352;
  const int frame_height = 288;
  VideoInfo video_info = { frame_width,  frame_height,
                           frame_rate,   AOM_IMG_FMT_I420,
                           frame_number, "bus_352x288_420_f20_b8.yuv" };
  video_info.file_path =
      libaom_test::GetDataPath() + "/" + video_info.file_path;
  DuckyEncode ducky_encode(video_info, BLOCK_64X64, kMaxRefFrames, 3, 128);

  AV1RateControlQMode qmode_rc;
  RateControlParam rc_param = {};
  rc_param.max_gop_show_frame_count = 16;
  rc_param.min_gop_show_frame_count = 4;
  rc_param.ref_frame_table_size = 5;
  rc_param.max_ref_frames = 3;
  rc_param.base_q_index = 45;
  rc_param.max_distinct_q_indices_per_frame = 8;
  rc_param.max_distinct_lambda_scales_per_frame = 1;
  rc_param.frame_width = frame_width;
  rc_param.frame_height = frame_height;
  rc_param.tpl_pass_count = TplPassCount::kOneTplPass;
  rc_param.tpl_pass_index = 0;
  const Status status = qmode_rc.SetRcParam(rc_param);
  ASSERT_TRUE(status.ok());
  FirstpassInfo firstpass_info;
  firstpass_info.stats_list = ducky_encode.ComputeFirstPassStats();
  constexpr int kBlockSize = 16;
  firstpass_info.num_mbs_16x16 = ((frame_width + kBlockSize - 1) / kBlockSize) *
                                 ((frame_height + kBlockSize - 1) / kBlockSize);
  const auto gop_info = qmode_rc.DetermineGopInfo(firstpass_info);
  ASSERT_TRUE(gop_info.status().ok());
  const GopStructList &gop_list = gop_info.value();

  std::vector<aom::GopEncodeInfo> tpl_pass_gop_encode_info_list;
  std::vector<aom::TplGopStats> tpl_gop_stats_list;
  for (const auto &gop_struct : gop_list) {
    const auto gop_encode_info =
        qmode_rc.GetTplPassGopEncodeInfo(gop_struct, firstpass_info);
    ASSERT_TRUE(gop_encode_info.status().ok());
    tpl_pass_gop_encode_info_list.push_back(std::move(*gop_encode_info));
  }

  tpl_gop_stats_list = ducky_encode.ComputeTplStats(
      firstpass_info.stats_list, gop_list, tpl_pass_gop_encode_info_list);

  std::vector<aom::GopEncodeInfo> final_pass_gop_encode_info_list;
  aom::RefFrameTable ref_frame_table;
  for (size_t i = 0; i < gop_list.size(); ++i) {
    const aom::GopStruct &gop_struct = gop_list[i];
    const aom::TplGopStats &tpl_gop_stats = tpl_gop_stats_list[i];
    std::vector<aom::LookaheadStats> lookahead_stats = {};
    for (size_t lookahead_index = 1;
         lookahead_index <= 1 && i + lookahead_index < gop_list.size();
         ++lookahead_index) {
      lookahead_stats.push_back({ &gop_list[i + lookahead_index],
                                  &tpl_gop_stats_list[i + lookahead_index] });
    }
    const auto gop_encode_info =
        qmode_rc.GetGopEncodeInfo(gop_struct, tpl_gop_stats, lookahead_stats,
                                  firstpass_info, ref_frame_table);
    ASSERT_TRUE(gop_encode_info.status().ok());
    ref_frame_table = gop_encode_info.value().final_snapshot;
    final_pass_gop_encode_info_list.push_back(std::move(*gop_encode_info));
  }

  ducky_encode.StartEncode(firstpass_info.stats_list);
  std::vector<aom::EncodeFrameResult> encoded_frames_list =
      ducky_encode.EncodeVideo(gop_list, final_pass_gop_encode_info_list);
  ducky_encode.EndEncode();

  EXPECT_THAT(encoded_frames_list,
              testing::Each(testing::Field(
                  "psnr", &aom::EncodeFrameResult::psnr, testing::Gt(37))));
}

TEST(DuckyEncodeTest, EncodeFrameMode) {
  EXPECT_EQ(DUCKY_ENCODE_FRAME_MODE_NONE,
            static_cast<DUCKY_ENCODE_FRAME_MODE>(EncodeFrameMode::kNone));
  EXPECT_EQ(DUCKY_ENCODE_FRAME_MODE_QINDEX,
            static_cast<DUCKY_ENCODE_FRAME_MODE>(EncodeFrameMode::kQindex));
  EXPECT_EQ(
      DUCKY_ENCODE_FRAME_MODE_QINDEX_RDMULT,
      static_cast<DUCKY_ENCODE_FRAME_MODE>(EncodeFrameMode::kQindexRdmult));
}

}  // namespace aom
