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
constexpr int kRefFrameTableSize = 7;

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
  int frame_width;
  int frame_height;
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

enum class ReferenceName {
  kNoneFrame = -1,
  kIntraFrame = 0,
  kLastFrame = 1,
  kLast2Frame = 2,
  kLast3Frame = 3,
  kGoldenFrame = 4,
  kBwdrefFrame = 5,
  kAltref2Frame = 6,
  kAltrefFrame = 7,
};

struct ReferenceFrame {
  int index;  // Index of reference slot containing the reference frame
  ReferenceName name;
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
  int update_ref_idx;     // The reference index that this frame should be
                          // updated to. update_ref_idx == -1 when this frame
                          // will not serve as a reference frame
  std::vector<ReferenceFrame>
      ref_frame_list;  // A list of available reference frames in priority order
                       // for the current to-be-coded frame. The list size
                       // should be less or equal to kRefFrameTableSize. The
                       // reference frames with smaller indices are more likely
                       // to be a good reference frame. Therefore, they should
                       // be prioritized when the reference frame count is
                       // limited. For example, if we plan to use 3 reference
                       // frames, we should choose ref_frame_list[0],
                       // ref_frame_list[1] and ref_frame_list[2].
  int layer_depth;     // Layer depth in the GOP structure
  ReferenceFrame primary_ref_frame;  // We will use the primary reference frame
                                     // to update current frame's initial
                                     // probability model
};

struct GopStruct {
  int show_frame_count;
  int global_coding_idx_offset;
  int global_order_idx_offset;
  std::vector<GopFrame> gop_frame_list;
};

using GopStructList = std::vector<GopStruct>;

struct FrameEncodeParameters {
  int q_index;
  int rdmult;
};

struct FirstpassInfo {
  int num_mbs_16x16;  // Count of 16x16 unit blocks in each frame.
                      // FIRSTPASS_STATS's unit block size is 16x16
  std::vector<FIRSTPASS_STATS> stats_list;
};

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
      const FirstpassInfo &firstpass_info) = 0;
  // Accept firstpass and tpl info from the encoder and return q index and
  // rdmult. This needs to be called with consecutive GOPs as returned by
  // DetermineGopInfo.
  virtual GopEncodeInfo GetGopEncodeInfo(
      const GopStruct &gop_struct, const TplGopStats &tpl_gop_stats,
      const RefFrameTable &ref_frame_table_snapshot_init) = 0;
};  // class AV1RateCtrlQMode
}  // namespace aom

#endif  // AOM_AV1_RATECTRL_QMODE_INTERFACE_H_
