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

#ifndef AOM_AV1_QMODE_RC_RATECTRL_QMODE_INTERFACE_H_
#define AOM_AV1_QMODE_RC_RATECTRL_QMODE_INTERFACE_H_

#include <array>
#include <string>
#include <vector>

#include "aom/aom_codec.h"
#include "av1/encoder/firstpass.h"

namespace aom {

constexpr int kBlockRefCount = 2;

struct MotionVector {
  int row;  // subpel row
  int col;  // subpel col
  // TODO(b/241589513): Move this to TplFrameStats; it's wasteful to code it
  // separately for each block.
  int subpel_bits;  // number of fractional bits used by row/col
};

enum class TplPassCount {
  kOneTplPass = 1,
  kTwoTplPasses = 2,
};

struct RateControlParam {
  // Range of allowed GOP sizes (number of displayed frames).
  int max_gop_show_frame_count;
  int min_gop_show_frame_count;
  // Number of reference frame buffers, i.e., size of the DPB.
  int ref_frame_table_size;
  // Maximum number of references a single frame may use.
  int max_ref_frames;

  int base_q_index;

  // If greater than 1, enables per-superblock q_index, and limits the number of
  // unique q_index values which may be used in a frame (each of which will have
  // its own unique rdmult value).
  int max_distinct_q_indices_per_frame;

  // If per-superblock q_index is enabled and this is greater than 1, enables
  // additional per-superblock scaling of lambda, and limits the number of
  // unique lambda scale values which may be used in a frame.
  int max_distinct_lambda_scales_per_frame;

  int frame_width;
  int frame_height;

  // Total number of TPL passes.
  TplPassCount tpl_pass_count = TplPassCount::kOneTplPass;
  // Current TPL pass number, 0 or 1 (for GetTplPassGopEncodeInfo).
  int tpl_pass_index = 0;
};

struct TplBlockStats {
  int16_t height;      // Pixel height.
  int16_t width;       // Pixel width.
  int16_t row;         // Pixel row of the top left corner.
  int16_t col;         // Pixel col of the top left corner.
  int64_t intra_cost;  // Rd cost of the best intra mode.
  int64_t inter_cost;  // Rd cost of the best inter mode.

  // Valid only if TplFrameStats::rate_dist_present is true:
  int64_t recrf_rate;      // Bits when using recon as reference.
  int64_t recrf_dist;      // Distortion when using recon as reference.
  int64_t intra_pred_err;  // Prediction residual of the intra mode.
  int64_t inter_pred_err;  // Prediction residual of the inter mode.

  std::array<MotionVector, kBlockRefCount> mv;
  std::array<int, kBlockRefCount> ref_frame_index;
};

// gop frame type used for facilitate setting up GopFrame
// TODO(angiebird): Define names for forward key frame and
// key frame with overlay
enum class GopFrameType {
  kRegularKey,     // High quality key frame without overlay
  kRegularLeaf,    // Regular leaf frame
  kRegularGolden,  // Regular golden frame
  kRegularArf,  // High quality arf with strong filtering followed by an overlay
                // later
  kOverlay,     // Overlay frame
  kIntermediateOverlay,  // Intermediate overlay frame
  kIntermediateArf,  // Good quality arf with weak or no filtering followed by a
                     // show_existing later
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

struct Status {
  aom_codec_err_t code;
  std::string message;  // Empty if code == AOM_CODEC_OK.
  bool ok() const { return code == AOM_CODEC_OK; }
};

// A very simple imitation of absl::StatusOr, this is conceptually a union of a
// Status struct and an object of type T. It models an object that is either a
// usable object, or an error explaining why such an object is not present. A
// StatusOr<T> may never hold a status with a code of AOM_CODEC_OK.
template <typename T>
class StatusOr {
 public:
  StatusOr(const T &value) : value_(value) {}
  StatusOr(T &&value) : value_(std::move(value)) {}
  StatusOr(Status status) : status_(std::move(status)) {
    assert(status_.code != AOM_CODEC_OK);
  }

  const Status &status() const { return status_; }
  bool ok() const { return status().ok(); }

  // operator* returns the value; it should only be called after checking that
  // ok() returns true.
  const T &operator*() const & { return value_; }
  T &operator*() & { return value_; }
  const T &&operator*() const && { return value_; }
  T &&operator*() && { return std::move(value_); }

  // sor->field is equivalent to (*sor).field.
  const T *operator->() const & { return &value_; }
  T *operator->() & { return &value_; }

  // value() is equivalent to operator*, but asserts that ok() is true.
  const T &value() const & {
    assert(ok());
    return value_;
  }
  T &value() & {
    assert(ok());
    return value_;
  }
  const T &&value() const && {
    assert(ok());
    return value_;
  }
  T &&value() && {
    assert(ok());
    return std::move(value_);
  }

 private:
  T value_;  // This could be std::optional<T> if it were available.
  Status status_ = { AOM_CODEC_OK, "" };
};

struct ReferenceFrame {
  int index;  // Index of reference slot containing the reference frame
  ReferenceName name;
};

struct GopFrame {
  // basic info
  bool is_valid;
  int order_idx;    // Index in display order in a GOP
  int coding_idx;   // Index in coding order in a GOP
  int display_idx;  // The number of displayed frames preceding this frame in
                    // a GOP

  int global_order_idx;   // Index in display order in the whole video chunk
  int global_coding_idx;  // Index in coding order in the whole video chunk

  bool is_key_frame;     // If this is key frame, reset reference buffers are
                         // required
  bool is_arf_frame;     // Is this a forward frame, a frame with order_idx
                         // higher than the current display order
  bool is_show_frame;    // Is this frame a show frame after coding
  bool is_golden_frame;  // Is this a high quality frame

  GopFrameType update_type;  // This is a redundant field. It is only used for
                             // easy conversion in SW integration.

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
                       // should be less or equal to ref_frame_table_size. The
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
  // TODO(jingning): This can be removed once the framework is up running.
  int display_tracker;  // Track the number of frames displayed proceeding a
                        // current coding frame.
  double base_q_ratio;  // The adjustment ratio, based on which the base QP
                        // index of this Gop will be adjusted from
                        // RateControlParam::base_q_index.
  std::vector<GopFrame> gop_frame_list;
};

using GopStructList = std::vector<GopStruct>;

struct SuperblockEncodeParameters {
  int q_index;
  int rdmult;
};

struct FrameEncodeParameters {
  // Base q_index for the frame.
  int q_index;

  // Frame level Lagrangian multiplier.
  int rdmult;

  // If max_distinct_q_indices_per_frame <= 1, this will be empty.
  // Otherwise:
  // - There must be one entry per 64x64 superblock, in row-major order
  // - There may be no more than max_distinct_q_indices_per_frame unique q_index
  //   values
  // - All entries with the same q_index must have the same rdmult
  // (If it's desired to use different rdmult values with the same q_index, this
  // must be done with superblock_lambda_scales.)
  std::vector<SuperblockEncodeParameters> superblock_encode_params;

  // If max_distinct_q_indices_per_frame <= 1 or
  // max_distinct_lambda_scales_per_frame <= 1, this will be empty. Otherwise,
  // it will have one entry per 64x64 superblock, in row-major order, with no
  // more than max_distinct_lambda_scales_per_frame unique values. Each entry
  // should be multiplied by the rdmult in the corresponding superblock's entry
  // in superblock_encode_params.
  std::vector<float> superblock_lambda_scales;
};

struct FirstpassInfo {
  int num_mbs_16x16;  // Count of 16x16 unit blocks in each frame.
                      // FIRSTPASS_STATS's unit block size is 16x16
  std::vector<FIRSTPASS_STATS> stats_list;
};

// In general, the number of elements in RefFrameTable must always equal
// ref_frame_table_size (as specified in RateControlParam), but see
// GetGopEncodeInfo for the one exception.
using RefFrameTable = std::vector<GopFrame>;

struct GopEncodeInfo {
  std::vector<FrameEncodeParameters> param_list;
  RefFrameTable final_snapshot;  // RefFrameTable snapshot after coding this GOP
};

struct TplFrameStats {
  int min_block_size;
  int frame_width;
  int frame_height;
  bool rate_dist_present;  // True if recrf_rate and recrf_dist are populated.
  std::vector<TplBlockStats> block_stats_list;
  // Optional stats computed with different settings, should be empty unless
  // tpl_pass_count == kTwoTplPasses.
  std::vector<TplBlockStats> alternate_block_stats_list;
};

struct TplGopStats {
  std::vector<TplFrameStats> frame_stats_list;
};

// Structure and TPL stats for a single GOP, to be used for lookahead.
struct LookaheadStats {
  const GopStruct *gop_struct;       // Not owned, may not be nullptr.
  const TplGopStats *tpl_gop_stats;  // Not owned, may not be nullptr.
};

class AV1RateControlQModeInterface {
 public:
  AV1RateControlQModeInterface();
  virtual ~AV1RateControlQModeInterface();

  virtual Status SetRcParam(const RateControlParam &rc_param) = 0;
  virtual StatusOr<GopStructList> DetermineGopInfo(
      const FirstpassInfo &firstpass_info) = 0;

  // Accepts GOP structure, TPL info, and first pass stats from the encoder and
  // returns per-frame (and optionally per-superblock) q index and rdmult. This
  // should be called with consecutive GOPs as returned by DetermineGopInfo.
  //
  // GOP structure and TPL info from zero or more subsequent GOPs may optionally
  // be passed in lookahead_stats.
  //
  // Stats starting at the first frame of the GOP and extending at least through
  // any lookup GOPs should be passed in firstpass_info.
  //
  // For the first GOP, a default-constructed RefFrameTable may be passed in as
  // ref_frame_table_snapshot_init; for subsequent GOPs, it should be the
  // final_snapshot returned on the previous call.
  virtual StatusOr<GopEncodeInfo> GetGopEncodeInfo(
      const GopStruct &gop_struct, const TplGopStats &tpl_gop_stats,
      const std::vector<LookaheadStats> &lookahead_stats,
      const FirstpassInfo &firstpass_info,
      const RefFrameTable &ref_frame_table_snapshot) = 0;
  // Similar to GetGopEncodeInfo but for the TPL pass. The returned encode info
  // is only per-frame level, never per-superblock.
  virtual StatusOr<GopEncodeInfo> GetTplPassGopEncodeInfo(
      const GopStruct &gop_struct, const FirstpassInfo &firstpass_info) = 0;
};
}  // namespace aom

#endif  // AOM_AV1_QMODE_RC_RATECTRL_QMODE_INTERFACE_H_
