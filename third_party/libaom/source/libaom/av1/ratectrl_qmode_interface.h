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

#ifndef AOM_AV1_RATECTRL_QMODE_INTERFACE_H_
#define AOM_AV1_RATECTRL_QMODE_INTERFACE_H_

#include <array>
#include <vector>

#include "av1/encoder/firstpass.h"

namespace aom {

constexpr int kBlockRefCount = 2;
constexpr int kRefFrameTableSize = 8;

struct MotionVector {
  int row;          // subpel row
  int col;          // subpel col
  int subpel_bits;  // number of fractional bits used by row/col
};

struct RateControlParam {
  int max_gop_show_frame_count;
  int min_gop_show_frame_count;
  int max_ref_frames;
  int base_q_index;
};

struct TplBlockStats {
  int height;  // pixel height
  int width;   // pixel width
  int row;     // pixel row of the top left corner
  int col;     // pixel col of the top lef corner
  int64_t intra_cost;
  int64_t inter_cost;
  std::array<MotionVector, kBlockRefCount> mv;
  std::array<int, kBlockRefCount> ref_frame_index;
};

enum class EncodeRefMode {
  kRegular,
  kOverlay,
  kShowExisting,
};

struct GopFrame {
  // basic info
  bool is_valid;
  int order_idx;   // Index in display order in a GOP
  int coding_idx;  // Index in coding order in a GOP

  int global_order_idx;   // Index in display order in the whole video chunk
  int global_coding_idx;  // Index in coding order in the whole video chunk

  bool is_key_frame;     // If this is key frame, reset reference buffers are
                         // required
  bool is_arf_frame;     // Is this a forward frame, a frame with order_idx
                         // higher than the current display order
  bool is_show_frame;    // Is this frame a show frame after coding
  bool is_golden_frame;  // Is this a high quality frame

  // reference frame info
  EncodeRefMode encode_ref_mode;
  int colocated_ref_idx;  // colocated_ref_idx == -1 when encode_ref_mode ==
                          // EncodeRefMode::kRegular
  int update_ref_idx;  // The reference index that this frame should be updated
                       // to. update_ref_idx == -1 when this frame will not
                       // serve as a reference frame
  std::vector<int>
      ref_idx_list;     // The indices of reference frames.
                        // The size should be less or equal to max_ref_frames.
  int layer_depth;      // Layer depth in the GOP structure
  int primary_ref_idx;  // We will use the primary reference to update current
                        // frame's initial probability model
};

struct GopStruct {
  int show_frame_count;
  std::vector<GopFrame> gop_frame_list;
};

using GopStructList = std::vector<GopStruct>;

struct FrameEncodeParameters {
  int q_index;
  int rdmult;
};

using FirstpassInfo = std::vector<FIRSTPASS_STATS>;
using RefFrameTable = std::array<GopFrame, kRefFrameTableSize>;

struct GopEncodeInfo {
  std::vector<FrameEncodeParameters> param_list;
  RefFrameTable final_snapshot;  // RefFrameTable snapshot after coding this GOP
};

struct TplFrameStats {
  int min_block_size;
  int frame_width;
  int frame_height;
  std::vector<TplBlockStats> block_stats_list;
};

struct TplGopStats {
  std::vector<TplFrameStats> frame_stats_list;
};

class AV1RateControlQModeInterface {
 public:
  AV1RateControlQModeInterface();
  virtual ~AV1RateControlQModeInterface();

  virtual void SetRcParam(const RateControlParam &rc_param) = 0;
  virtual GopStructList DetermineGopInfo(
      const FirstpassInfo &firstpass_stats_list) = 0;
  // Accept firstpass and tpl info from the encoder and return q index and
  // rdmult. This needs to be called with consecutive GOPs as returned by
  // DetermineGopInfo.
  virtual GopEncodeInfo GetGopEncodeInfo(
      const GopStruct &gop_struct, const TplGopStats &tpl_gop_stats,
      const RefFrameTable &ref_frame_table_snapshot_init) = 0;
};  // class AV1RateCtrlQMode
}  // namespace aom

#endif  // AOM_AV1_RATECTRL_QMODE_INTERFACE_H_
