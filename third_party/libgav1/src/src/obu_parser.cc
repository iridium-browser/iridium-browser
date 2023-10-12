// Copyright 2019 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/obu_parser.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include "src/buffer_pool.h"
#include "src/decoder_impl.h"
#include "src/motion_vector.h"
#include "src/utils/common.h"
#include "src/utils/logging.h"

namespace libgav1 {
namespace {

// 5.9.16.
// Find the smallest value of k such that block_size << k is greater than or
// equal to target.
//
// NOTE: TileLog2(block_size, target) is equal to
//   CeilLog2(ceil((double)target / block_size))
// where the division is a floating-point number division. (This equality holds
// even when |target| is equal to 0.) In the special case of block_size == 1,
// TileLog2(1, target) is equal to CeilLog2(target).
int TileLog2(int block_size, int target) {
  int k = 0;
  for (; (block_size << k) < target; ++k) {
  }
  return k;
}

void ParseBitStreamLevel(BitStreamLevel* const level, uint8_t level_bits) {
  level->major = kMinimumMajorBitstreamLevel + (level_bits >> 2);
  level->minor = level_bits & 3;
}

// This function assumes loop_filter is zero-initialized, so only it needs to
// set the nonzero default values.
void SetDefaultRefDeltas(LoopFilter* const loop_filter) {
  loop_filter->ref_deltas[kReferenceFrameIntra] = 1;
  loop_filter->ref_deltas[kReferenceFrameGolden] = -1;
  loop_filter->ref_deltas[kReferenceFrameAlternate] = -1;
  loop_filter->ref_deltas[kReferenceFrameAlternate2] = -1;
}

bool InTemporalLayer(int operating_point_idc, int temporal_id) {
  return ((operating_point_idc >> temporal_id) & 1) != 0;
}

bool InSpatialLayer(int operating_point_idc, int spatial_id) {
  return ((operating_point_idc >> (spatial_id + 8)) & 1) != 0;
}

// Returns the index of the last nonzero byte in the |data| buffer of |size|
// bytes. If there is no nonzero byte in the |data| buffer, returns -1.
int GetLastNonzeroByteIndex(const uint8_t* data, size_t size) {
  // Scan backward for a nonzero byte.
  if (size > INT_MAX) return -1;
  int i = static_cast<int>(size) - 1;
  while (i >= 0 && data[i] == 0) {
    --i;
  }
  return i;
}

// A cleanup helper class that releases the frame buffer reference held in
// |frame| in the destructor.
class RefCountedBufferPtrCleanup {
 public:
  explicit RefCountedBufferPtrCleanup(RefCountedBufferPtr* frame)
      : frame_(*frame) {}

  // Not copyable or movable.
  RefCountedBufferPtrCleanup(const RefCountedBufferPtrCleanup&) = delete;
  RefCountedBufferPtrCleanup& operator=(const RefCountedBufferPtrCleanup&) =
      delete;

  ~RefCountedBufferPtrCleanup() { frame_ = nullptr; }

 private:
  RefCountedBufferPtr& frame_;
};

}  // namespace

bool ObuSequenceHeader::ParametersChanged(const ObuSequenceHeader& old) const {
  // Note that the operating_parameters field is not compared per Section 7.5:
  //   Within a particular coded video sequence, the contents of
  //   sequence_header_obu must be bit-identical each time the sequence header
  //   appears except for the contents of operating_parameters_info.
  return memcmp(this, &old,
                offsetof(ObuSequenceHeader, operating_parameters)) != 0;
}

// Macros to avoid repeated error checks in the parser code.
#define OBU_LOG_AND_RETURN_FALSE                                            \
  do {                                                                      \
    LIBGAV1_DLOG(ERROR, "%s:%d (%s): Not enough bits.", __FILE__, __LINE__, \
                 __func__);                                                 \
    return false;                                                           \
  } while (false)
#define OBU_PARSER_FAIL         \
  do {                          \
    if (scratch == -1) {        \
      OBU_LOG_AND_RETURN_FALSE; \
    }                           \
  } while (false)
#define OBU_READ_BIT_OR_FAIL        \
  scratch = bit_reader_->ReadBit(); \
  OBU_PARSER_FAIL
#define OBU_READ_LITERAL_OR_FAIL(n)      \
  scratch = bit_reader_->ReadLiteral(n); \
  OBU_PARSER_FAIL
#define OBU_READ_UVLC_OR_FAIL(x)        \
  do {                                  \
    if (!bit_reader_->ReadUvlc(&(x))) { \
      OBU_LOG_AND_RETURN_FALSE;         \
    }                                   \
  } while (false)

bool ObuParser::ParseColorConfig(ObuSequenceHeader* sequence_header) {
  int64_t scratch;
  ColorConfig* const color_config = &sequence_header->color_config;
  OBU_READ_BIT_OR_FAIL;
  const bool high_bitdepth = scratch != 0;
  if (sequence_header->profile == kProfile2 && high_bitdepth) {
    OBU_READ_BIT_OR_FAIL;
    const bool is_twelve_bit = scratch != 0;
    color_config->bitdepth = is_twelve_bit ? 12 : 10;
  } else {
    color_config->bitdepth = high_bitdepth ? 10 : 8;
  }
  if (sequence_header->profile == kProfile1) {
    color_config->is_monochrome = false;
  } else {
    OBU_READ_BIT_OR_FAIL;
    color_config->is_monochrome = scratch != 0;
  }
  OBU_READ_BIT_OR_FAIL;
  const bool color_description_present_flag = scratch != 0;
  if (color_description_present_flag) {
    OBU_READ_LITERAL_OR_FAIL(8);
    color_config->color_primary = static_cast<ColorPrimary>(scratch);
    OBU_READ_LITERAL_OR_FAIL(8);
    color_config->transfer_characteristics =
        static_cast<TransferCharacteristics>(scratch);
    OBU_READ_LITERAL_OR_FAIL(8);
    color_config->matrix_coefficients =
        static_cast<MatrixCoefficients>(scratch);
  } else {
    color_config->color_primary = kColorPrimaryUnspecified;
    color_config->transfer_characteristics =
        kTransferCharacteristicsUnspecified;
    color_config->matrix_coefficients = kMatrixCoefficientsUnspecified;
  }
  if (color_config->is_monochrome) {
    OBU_READ_BIT_OR_FAIL;
    color_config->color_range = static_cast<ColorRange>(scratch);
    // Set subsampling_x and subsampling_y to 1 for monochrome. This makes it
    // easy to allow monochrome to be supported in profile 0. Profile 0
    // requires subsampling_x and subsampling_y to be 1.
    color_config->subsampling_x = 1;
    color_config->subsampling_y = 1;
    color_config->chroma_sample_position = kChromaSamplePositionUnknown;
  } else {
    if (color_config->color_primary == kColorPrimaryBt709 &&
        color_config->transfer_characteristics ==
            kTransferCharacteristicsSrgb &&
        color_config->matrix_coefficients == kMatrixCoefficientsIdentity) {
      color_config->color_range = kColorRangeFull;
      color_config->subsampling_x = 0;
      color_config->subsampling_y = 0;
      // YUV 4:4:4 is only allowed in profile 1, or profile 2 with bit depth 12.
      // See the table at the beginning of Section 6.4.1.
      if (sequence_header->profile != kProfile1 &&
          (sequence_header->profile != kProfile2 ||
           color_config->bitdepth != 12)) {
        LIBGAV1_DLOG(ERROR,
                     "YUV 4:4:4 is not allowed in profile %d for bitdepth %d.",
                     sequence_header->profile, color_config->bitdepth);
        return false;
      }
    } else {
      OBU_READ_BIT_OR_FAIL;
      color_config->color_range = static_cast<ColorRange>(scratch);
      if (sequence_header->profile == kProfile0) {
        color_config->subsampling_x = 1;
        color_config->subsampling_y = 1;
      } else if (sequence_header->profile == kProfile1) {
        color_config->subsampling_x = 0;
        color_config->subsampling_y = 0;
      } else {
        if (color_config->bitdepth == 12) {
          OBU_READ_BIT_OR_FAIL;
          color_config->subsampling_x = scratch;
          if (color_config->subsampling_x == 1) {
            OBU_READ_BIT_OR_FAIL;
            color_config->subsampling_y = scratch;
          } else {
            color_config->subsampling_y = 0;
          }
        } else {
          color_config->subsampling_x = 1;
          color_config->subsampling_y = 0;
        }
      }
      if (color_config->subsampling_x == 1 &&
          color_config->subsampling_y == 1) {
        OBU_READ_LITERAL_OR_FAIL(2);
        color_config->chroma_sample_position =
            static_cast<ChromaSamplePosition>(scratch);
      }
    }
    OBU_READ_BIT_OR_FAIL;
    color_config->separate_uv_delta_q = scratch != 0;
  }
  if (color_config->matrix_coefficients == kMatrixCoefficientsIdentity &&
      (color_config->subsampling_x != 0 || color_config->subsampling_y != 0)) {
    LIBGAV1_DLOG(ERROR,
                 "matrix_coefficients is MC_IDENTITY, but subsampling_x (%d) "
                 "and subsampling_y (%d) are not both 0.",
                 color_config->subsampling_x, color_config->subsampling_y);
    return false;
  }
  return true;
}

bool ObuParser::ParseTimingInfo(ObuSequenceHeader* sequence_header) {
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  sequence_header->timing_info_present_flag = scratch != 0;
  if (!sequence_header->timing_info_present_flag) return true;
  TimingInfo* const info = &sequence_header->timing_info;
  OBU_READ_LITERAL_OR_FAIL(32);
  info->num_units_in_tick = static_cast<uint32_t>(scratch);
  if (info->num_units_in_tick == 0) {
    LIBGAV1_DLOG(ERROR, "num_units_in_tick is 0.");
    return false;
  }
  OBU_READ_LITERAL_OR_FAIL(32);
  info->time_scale = static_cast<uint32_t>(scratch);
  if (info->time_scale == 0) {
    LIBGAV1_DLOG(ERROR, "time_scale is 0.");
    return false;
  }
  OBU_READ_BIT_OR_FAIL;
  info->equal_picture_interval = scratch != 0;
  if (info->equal_picture_interval) {
    OBU_READ_UVLC_OR_FAIL(info->num_ticks_per_picture);
    ++info->num_ticks_per_picture;
  }
  return true;
}

bool ObuParser::ParseDecoderModelInfo(ObuSequenceHeader* sequence_header) {
  if (!sequence_header->timing_info_present_flag) return true;
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  sequence_header->decoder_model_info_present_flag = scratch != 0;
  if (!sequence_header->decoder_model_info_present_flag) return true;
  DecoderModelInfo* const info = &sequence_header->decoder_model_info;
  OBU_READ_LITERAL_OR_FAIL(5);
  info->encoder_decoder_buffer_delay_length = 1 + scratch;
  OBU_READ_LITERAL_OR_FAIL(32);
  info->num_units_in_decoding_tick = static_cast<uint32_t>(scratch);
  OBU_READ_LITERAL_OR_FAIL(5);
  info->buffer_removal_time_length = 1 + scratch;
  OBU_READ_LITERAL_OR_FAIL(5);
  info->frame_presentation_time_length = 1 + scratch;
  return true;
}

bool ObuParser::ParseOperatingParameters(ObuSequenceHeader* sequence_header,
                                         int index) {
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  sequence_header->decoder_model_present_for_operating_point[index] =
      scratch != 0;
  if (!sequence_header->decoder_model_present_for_operating_point[index]) {
    return true;
  }
  OperatingParameters* const params = &sequence_header->operating_parameters;
  OBU_READ_LITERAL_OR_FAIL(
      sequence_header->decoder_model_info.encoder_decoder_buffer_delay_length);
  params->decoder_buffer_delay[index] = static_cast<uint32_t>(scratch);
  OBU_READ_LITERAL_OR_FAIL(
      sequence_header->decoder_model_info.encoder_decoder_buffer_delay_length);
  params->encoder_buffer_delay[index] = static_cast<uint32_t>(scratch);
  OBU_READ_BIT_OR_FAIL;
  params->low_delay_mode_flag[index] = scratch != 0;
  return true;
}

bool ObuParser::ParseSequenceHeader(bool seen_frame_header) {
  ObuSequenceHeader sequence_header = {};
  int64_t scratch;
  OBU_READ_LITERAL_OR_FAIL(3);
  if (scratch >= kMaxProfiles) {
    LIBGAV1_DLOG(ERROR, "Invalid profile: %d.", static_cast<int>(scratch));
    return false;
  }
  sequence_header.profile = static_cast<BitstreamProfile>(scratch);
  OBU_READ_BIT_OR_FAIL;
  sequence_header.still_picture = scratch != 0;
  OBU_READ_BIT_OR_FAIL;
  sequence_header.reduced_still_picture_header = scratch != 0;
  if (sequence_header.reduced_still_picture_header) {
    if (!sequence_header.still_picture) {
      LIBGAV1_DLOG(
          ERROR, "reduced_still_picture_header is 1, but still_picture is 0.");
      return false;
    }
    sequence_header.operating_points = 1;
    sequence_header.operating_point_idc[0] = 0;
    OBU_READ_LITERAL_OR_FAIL(5);
    ParseBitStreamLevel(&sequence_header.level[0], scratch);
  } else {
    if (!ParseTimingInfo(&sequence_header) ||
        !ParseDecoderModelInfo(&sequence_header)) {
      return false;
    }
    OBU_READ_BIT_OR_FAIL;
    const bool initial_display_delay_present_flag = scratch != 0;
    OBU_READ_LITERAL_OR_FAIL(5);
    sequence_header.operating_points = static_cast<int>(1 + scratch);
    if (operating_point_ >= sequence_header.operating_points) {
      LIBGAV1_DLOG(
          ERROR,
          "Invalid operating point: %d (valid range is [0,%d] inclusive).",
          operating_point_, sequence_header.operating_points - 1);
      return false;
    }
    for (int i = 0; i < sequence_header.operating_points; ++i) {
      OBU_READ_LITERAL_OR_FAIL(12);
      sequence_header.operating_point_idc[i] = static_cast<int>(scratch);
      for (int j = 0; j < i; ++j) {
        if (sequence_header.operating_point_idc[i] ==
            sequence_header.operating_point_idc[j]) {
          LIBGAV1_DLOG(ERROR,
                       "operating_point_idc[%d] (%d) is equal to "
                       "operating_point_idc[%d] (%d).",
                       i, sequence_header.operating_point_idc[i], j,
                       sequence_header.operating_point_idc[j]);
          return false;
        }
      }
      OBU_READ_LITERAL_OR_FAIL(5);
      ParseBitStreamLevel(&sequence_header.level[i], scratch);
      if (sequence_header.level[i].major > 3) {
        OBU_READ_BIT_OR_FAIL;
        sequence_header.tier[i] = scratch;
      }
      if (sequence_header.decoder_model_info_present_flag &&
          !ParseOperatingParameters(&sequence_header, i)) {
        return false;
      }
      if (initial_display_delay_present_flag) {
        OBU_READ_BIT_OR_FAIL;
        if (scratch != 0) {
          OBU_READ_LITERAL_OR_FAIL(4);
          sequence_header.initial_display_delay[i] = 1 + scratch;
        }
      }
    }
  }
  OBU_READ_LITERAL_OR_FAIL(4);
  sequence_header.frame_width_bits = 1 + scratch;
  OBU_READ_LITERAL_OR_FAIL(4);
  sequence_header.frame_height_bits = 1 + scratch;
  OBU_READ_LITERAL_OR_FAIL(sequence_header.frame_width_bits);
  sequence_header.max_frame_width = static_cast<int32_t>(1 + scratch);
  OBU_READ_LITERAL_OR_FAIL(sequence_header.frame_height_bits);
  sequence_header.max_frame_height = static_cast<int32_t>(1 + scratch);
  if (!sequence_header.reduced_still_picture_header) {
    OBU_READ_BIT_OR_FAIL;
    sequence_header.frame_id_numbers_present = scratch != 0;
  }
  if (sequence_header.frame_id_numbers_present) {
    OBU_READ_LITERAL_OR_FAIL(4);
    sequence_header.delta_frame_id_length_bits = 2 + scratch;
    OBU_READ_LITERAL_OR_FAIL(3);
    sequence_header.frame_id_length_bits =
        sequence_header.delta_frame_id_length_bits + 1 + scratch;
    // Section 6.8.2: It is a requirement of bitstream conformance that the
    // number of bits needed to read display_frame_id does not exceed 16. This
    // is equivalent to the constraint that idLen <= 16.
    if (sequence_header.frame_id_length_bits > 16) {
      LIBGAV1_DLOG(ERROR, "Invalid frame_id_length_bits: %d.",
                   sequence_header.frame_id_length_bits);
      return false;
    }
  }
  OBU_READ_BIT_OR_FAIL;
  sequence_header.use_128x128_superblock = scratch != 0;
  OBU_READ_BIT_OR_FAIL;
  sequence_header.enable_filter_intra = scratch != 0;
  OBU_READ_BIT_OR_FAIL;
  sequence_header.enable_intra_edge_filter = scratch != 0;
  if (sequence_header.reduced_still_picture_header) {
    sequence_header.force_screen_content_tools = kSelectScreenContentTools;
    sequence_header.force_integer_mv = kSelectIntegerMv;
  } else {
    OBU_READ_BIT_OR_FAIL;
    sequence_header.enable_interintra_compound = scratch != 0;
    OBU_READ_BIT_OR_FAIL;
    sequence_header.enable_masked_compound = scratch != 0;
    OBU_READ_BIT_OR_FAIL;
    sequence_header.enable_warped_motion = scratch != 0;
    OBU_READ_BIT_OR_FAIL;
    sequence_header.enable_dual_filter = scratch != 0;
    OBU_READ_BIT_OR_FAIL;
    sequence_header.enable_order_hint = scratch != 0;
    if (sequence_header.enable_order_hint) {
      OBU_READ_BIT_OR_FAIL;
      sequence_header.enable_jnt_comp = scratch != 0;
      OBU_READ_BIT_OR_FAIL;
      sequence_header.enable_ref_frame_mvs = scratch != 0;
    }
    OBU_READ_BIT_OR_FAIL;
    sequence_header.choose_screen_content_tools = scratch != 0;
    if (sequence_header.choose_screen_content_tools) {
      sequence_header.force_screen_content_tools = kSelectScreenContentTools;
    } else {
      OBU_READ_BIT_OR_FAIL;
      sequence_header.force_screen_content_tools = scratch;
    }
    if (sequence_header.force_screen_content_tools > 0) {
      OBU_READ_BIT_OR_FAIL;
      sequence_header.choose_integer_mv = scratch != 0;
      if (sequence_header.choose_integer_mv) {
        sequence_header.force_integer_mv = kSelectIntegerMv;
      } else {
        OBU_READ_BIT_OR_FAIL;
        sequence_header.force_integer_mv = scratch;
      }
    } else {
      sequence_header.force_integer_mv = kSelectIntegerMv;
    }
    if (sequence_header.enable_order_hint) {
      OBU_READ_LITERAL_OR_FAIL(3);
      sequence_header.order_hint_bits = 1 + scratch;
      sequence_header.order_hint_shift_bits =
          Mod32(32 - sequence_header.order_hint_bits);
    }
  }
  OBU_READ_BIT_OR_FAIL;
  sequence_header.enable_superres = scratch != 0;
  OBU_READ_BIT_OR_FAIL;
  sequence_header.enable_cdef = scratch != 0;
  OBU_READ_BIT_OR_FAIL;
  sequence_header.enable_restoration = scratch != 0;
  if (!ParseColorConfig(&sequence_header)) return false;
  OBU_READ_BIT_OR_FAIL;
  sequence_header.film_grain_params_present = scratch != 0;
  // Compare new sequence header with old sequence header.
  if (has_sequence_header_ &&
      sequence_header.ParametersChanged(sequence_header_)) {
    // Between the frame header OBU and the last tile group OBU of the frame,
    // do not allow the sequence header to change.
    if (seen_frame_header) {
      LIBGAV1_DLOG(ERROR, "Sequence header changed in the middle of a frame.");
      return false;
    }
    sequence_header_changed_ = true;
    decoder_state_.ClearReferenceFrames();
  }
  sequence_header_ = sequence_header;
  if (!has_sequence_header_) {
    sequence_header_changed_ = true;
  }
  has_sequence_header_ = true;
  // Section 6.4.1: It is a requirement of bitstream conformance that if
  // OperatingPointIdc is equal to 0, then obu_extension_flag is equal to 0 for
  // all OBUs that follow this sequence header until the next sequence header.
  extension_disallowed_ =
      (sequence_header_.operating_point_idc[operating_point_] == 0);
  return true;
}

// Marks reference frames as invalid for referencing when they are too far in
// the past to be referenced by the frame id mechanism.
void ObuParser::MarkInvalidReferenceFrames() {
  // The current lower bound of the frame ids for reference frames.
  int lower_bound = decoder_state_.current_frame_id -
                    (1 << sequence_header_.delta_frame_id_length_bits);
  // True if lower_bound is smaller than current_frame_id. False if lower_bound
  // wraps around (in modular arithmetic) to the other side of current_frame_id.
  bool lower_bound_is_smaller = true;
  if (lower_bound <= 0) {
    lower_bound += 1 << sequence_header_.frame_id_length_bits;
    lower_bound_is_smaller = false;
  }
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    const uint16_t reference_frame_id = decoder_state_.reference_frame_id[i];
    if (lower_bound_is_smaller) {
      if (reference_frame_id > decoder_state_.current_frame_id ||
          reference_frame_id < lower_bound) {
        decoder_state_.reference_frame[i] = nullptr;
      }
    } else {
      if (reference_frame_id > decoder_state_.current_frame_id &&
          reference_frame_id < lower_bound) {
        decoder_state_.reference_frame[i] = nullptr;
      }
    }
  }
}

bool ObuParser::ParseFrameSizeAndRenderSize() {
  int64_t scratch;
  // Frame Size.
  if (frame_header_.frame_size_override_flag) {
    OBU_READ_LITERAL_OR_FAIL(sequence_header_.frame_width_bits);
    frame_header_.width = static_cast<int32_t>(1 + scratch);
    OBU_READ_LITERAL_OR_FAIL(sequence_header_.frame_height_bits);
    frame_header_.height = static_cast<int32_t>(1 + scratch);
    if (frame_header_.width > sequence_header_.max_frame_width ||
        frame_header_.height > sequence_header_.max_frame_height) {
      LIBGAV1_DLOG(ERROR,
                   "Frame dimensions are larger than the maximum values");
      return false;
    }
  } else {
    frame_header_.width = sequence_header_.max_frame_width;
    frame_header_.height = sequence_header_.max_frame_height;
  }
  if (!ParseSuperResParametersAndComputeImageSize()) return false;

  // Render Size.
  OBU_READ_BIT_OR_FAIL;
  frame_header_.render_and_frame_size_different = scratch != 0;
  if (frame_header_.render_and_frame_size_different) {
    OBU_READ_LITERAL_OR_FAIL(16);
    frame_header_.render_width = static_cast<int32_t>(1 + scratch);
    OBU_READ_LITERAL_OR_FAIL(16);
    frame_header_.render_height = static_cast<int32_t>(1 + scratch);
  } else {
    frame_header_.render_width = frame_header_.upscaled_width;
    frame_header_.render_height = frame_header_.height;
  }

  return true;
}

bool ObuParser::ParseSuperResParametersAndComputeImageSize() {
  int64_t scratch;
  // SuperRes.
  frame_header_.upscaled_width = frame_header_.width;
  frame_header_.use_superres = false;
  if (sequence_header_.enable_superres) {
    OBU_READ_BIT_OR_FAIL;
    frame_header_.use_superres = scratch != 0;
  }
  if (frame_header_.use_superres) {
    OBU_READ_LITERAL_OR_FAIL(3);
    // 9 is the smallest value for the denominator.
    frame_header_.superres_scale_denominator = scratch + 9;
    frame_header_.width =
        (frame_header_.upscaled_width * kSuperResScaleNumerator +
         (frame_header_.superres_scale_denominator / 2)) /
        frame_header_.superres_scale_denominator;
  } else {
    frame_header_.superres_scale_denominator = kSuperResScaleNumerator;
  }
  assert(frame_header_.width != 0);
  assert(frame_header_.height != 0);
  // Check if multiplying upscaled_width by height would overflow.
  assert(frame_header_.upscaled_width >= frame_header_.width);
  if (frame_header_.upscaled_width > INT32_MAX / frame_header_.height) {
    LIBGAV1_DLOG(ERROR, "Frame dimensions too big: width=%d height=%d.",
                 frame_header_.width, frame_header_.height);
    return false;
  }
  frame_header_.columns4x4 = ((frame_header_.width + 7) >> 3) << 1;
  frame_header_.rows4x4 = ((frame_header_.height + 7) >> 3) << 1;
  return true;
}

bool ObuParser::ValidateInterFrameSize() const {
  for (int index : frame_header_.reference_frame_index) {
    const RefCountedBuffer* reference_frame =
        decoder_state_.reference_frame[index].get();
    if (2 * frame_header_.width < reference_frame->upscaled_width() ||
        2 * frame_header_.height < reference_frame->frame_height() ||
        frame_header_.width > 16 * reference_frame->upscaled_width() ||
        frame_header_.height > 16 * reference_frame->frame_height()) {
      LIBGAV1_DLOG(ERROR,
                   "Invalid inter frame size: width=%d, height=%d. Reference "
                   "frame: index=%d, upscaled width=%d, height=%d.",
                   frame_header_.width, frame_header_.height, index,
                   reference_frame->upscaled_width(),
                   reference_frame->frame_height());
      return false;
    }
  }
  return true;
}

bool ObuParser::ParseReferenceOrderHint() {
  if (!frame_header_.error_resilient_mode ||
      !sequence_header_.enable_order_hint) {
    return true;
  }
  int64_t scratch;
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    OBU_READ_LITERAL_OR_FAIL(sequence_header_.order_hint_bits);
    frame_header_.reference_order_hint[i] = scratch;
    if (frame_header_.reference_order_hint[i] !=
        decoder_state_.reference_order_hint[i]) {
      decoder_state_.reference_frame[i] = nullptr;
    }
  }
  return true;
}

// static
int ObuParser::FindLatestBackwardReference(
    const int current_frame_hint,
    const std::array<int, kNumReferenceFrameTypes>& shifted_order_hints,
    const std::array<bool, kNumReferenceFrameTypes>& used_frame) {
  int ref = -1;
  int latest_order_hint = INT_MIN;
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    const int hint = shifted_order_hints[i];
    if (!used_frame[i] && hint >= current_frame_hint &&
        hint >= latest_order_hint) {
      ref = i;
      latest_order_hint = hint;
    }
  }
  return ref;
}

// static
int ObuParser::FindEarliestBackwardReference(
    const int current_frame_hint,
    const std::array<int, kNumReferenceFrameTypes>& shifted_order_hints,
    const std::array<bool, kNumReferenceFrameTypes>& used_frame) {
  int ref = -1;
  int earliest_order_hint = INT_MAX;
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    const int hint = shifted_order_hints[i];
    if (!used_frame[i] && hint >= current_frame_hint &&
        hint < earliest_order_hint) {
      ref = i;
      earliest_order_hint = hint;
    }
  }
  return ref;
}

// static
int ObuParser::FindLatestForwardReference(
    const int current_frame_hint,
    const std::array<int, kNumReferenceFrameTypes>& shifted_order_hints,
    const std::array<bool, kNumReferenceFrameTypes>& used_frame) {
  int ref = -1;
  int latest_order_hint = INT_MIN;
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    const int hint = shifted_order_hints[i];
    if (!used_frame[i] && hint < current_frame_hint &&
        hint >= latest_order_hint) {
      ref = i;
      latest_order_hint = hint;
    }
  }
  return ref;
}

// static
int ObuParser::FindReferenceWithSmallestOutputOrder(
    const std::array<int, kNumReferenceFrameTypes>& shifted_order_hints) {
  int ref = -1;
  int earliest_order_hint = INT_MAX;
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    const int hint = shifted_order_hints[i];
    if (hint < earliest_order_hint) {
      ref = i;
      earliest_order_hint = hint;
    }
  }
  return ref;
}

// Computes the elements in the frame_header_.reference_frame_index array
// based on:
// * the syntax elements last_frame_idx and gold_frame_idx, and
// * the values stored within the decoder_state_.reference_order_hint array
//   (these values represent the least significant bits of the expected output
//   order of the frames).
//
// Frame type: {
//       libgav1_name              spec_name              int
//   kReferenceFrameLast,          LAST_FRAME              1
//   kReferenceFrameLast2,         LAST2_FRAME             2
//   kReferenceFrameLast3,         LAST3_FRAME             3
//   kReferenceFrameGolden,        GOLDEN_FRAME            4
//   kReferenceFrameBackward,      BWDREF_FRAME            5
//   kReferenceFrameAlternate2,    ALTREF2_FRAME           6
//   kReferenceFrameAlternate,     ALTREF_FRAME            7
// }
//
// A typical case of a group of pictures (frames) in display order:
// (However, more complex cases are possibly allowed in terms of
// bitstream conformance.)
//
// |         |         |         |         |         |         |         |
// |         |         |         |         |         |         |         |
// |         |         |         |         |         |         |         |
// |         |         |         |         |         |         |         |
//
// 4         3         2         1   current_frame   5         6         7
//
bool ObuParser::SetFrameReferences(const int8_t last_frame_idx,
                                   const int8_t gold_frame_idx) {
  // Set the ref_frame_idx entries for kReferenceFrameLast and
  // kReferenceFrameGolden to last_frame_idx and gold_frame_idx. Initialize
  // the other entries to -1.
  for (int8_t& reference_frame_index : frame_header_.reference_frame_index) {
    reference_frame_index = -1;
  }
  frame_header_
      .reference_frame_index[kReferenceFrameLast - kReferenceFrameLast] =
      last_frame_idx;
  frame_header_
      .reference_frame_index[kReferenceFrameGolden - kReferenceFrameLast] =
      gold_frame_idx;

  // used_frame records which reference frames have been used.
  std::array<bool, kNumReferenceFrameTypes> used_frame;
  used_frame.fill(false);
  used_frame[last_frame_idx] = true;
  used_frame[gold_frame_idx] = true;

  assert(sequence_header_.order_hint_bits >= 1);
  const int current_frame_hint = 1 << (sequence_header_.order_hint_bits - 1);
  // shifted_order_hints contains the expected output order shifted such that
  // the current frame has hint equal to current_frame_hint.
  std::array<int, kNumReferenceFrameTypes> shifted_order_hints;
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    const int relative_distance = GetRelativeDistance(
        decoder_state_.reference_order_hint[i], frame_header_.order_hint,
        sequence_header_.order_hint_shift_bits);
    shifted_order_hints[i] = current_frame_hint + relative_distance;
  }

  // The expected output orders for kReferenceFrameLast and
  // kReferenceFrameGolden.
  const int last_order_hint = shifted_order_hints[last_frame_idx];
  const int gold_order_hint = shifted_order_hints[gold_frame_idx];

  // Section 7.8: It is a requirement of bitstream conformance that
  // lastOrderHint and goldOrderHint are strictly less than curFrameHint.
  if (last_order_hint >= current_frame_hint ||
      gold_order_hint >= current_frame_hint) {
    return false;
  }

  // Find a backward reference to the frame with highest output order. If
  // found, set the kReferenceFrameAlternate reference to that backward
  // reference.
  int ref = FindLatestBackwardReference(current_frame_hint, shifted_order_hints,
                                        used_frame);
  if (ref >= 0) {
    frame_header_
        .reference_frame_index[kReferenceFrameAlternate - kReferenceFrameLast] =
        ref;
    used_frame[ref] = true;
  }

  // Find a backward reference to the closest frame. If found, set the
  // kReferenceFrameBackward reference to that backward reference.
  ref = FindEarliestBackwardReference(current_frame_hint, shifted_order_hints,
                                      used_frame);
  if (ref >= 0) {
    frame_header_
        .reference_frame_index[kReferenceFrameBackward - kReferenceFrameLast] =
        ref;
    used_frame[ref] = true;
  }

  // Set the kReferenceFrameAlternate2 reference to the next closest backward
  // reference.
  ref = FindEarliestBackwardReference(current_frame_hint, shifted_order_hints,
                                      used_frame);
  if (ref >= 0) {
    frame_header_.reference_frame_index[kReferenceFrameAlternate2 -
                                        kReferenceFrameLast] = ref;
    used_frame[ref] = true;
  }

  // The remaining references are set to be forward references in
  // reverse chronological order.
  static constexpr ReferenceFrameType
      kRefFrameList[kNumInterReferenceFrameTypes - 2] = {
          kReferenceFrameLast2, kReferenceFrameLast3, kReferenceFrameBackward,
          kReferenceFrameAlternate2, kReferenceFrameAlternate};
  for (const ReferenceFrameType ref_frame : kRefFrameList) {
    if (frame_header_.reference_frame_index[ref_frame - kReferenceFrameLast] <
        0) {
      ref = FindLatestForwardReference(current_frame_hint, shifted_order_hints,
                                       used_frame);
      if (ref >= 0) {
        frame_header_.reference_frame_index[ref_frame - kReferenceFrameLast] =
            ref;
        used_frame[ref] = true;
      }
    }
  }

  // Finally, any remaining references are set to the reference frame with
  // smallest output order.
  ref = FindReferenceWithSmallestOutputOrder(shifted_order_hints);
  assert(ref >= 0);
  for (int8_t& reference_frame_index : frame_header_.reference_frame_index) {
    if (reference_frame_index < 0) {
      reference_frame_index = ref;
    }
  }

  return true;
}

bool ObuParser::ParseLoopFilterParameters() {
  LoopFilter* const loop_filter = &frame_header_.loop_filter;
  if (frame_header_.coded_lossless || frame_header_.allow_intrabc) {
    SetDefaultRefDeltas(loop_filter);
    return true;
  }
  // IsIntraFrame implies kPrimaryReferenceNone.
  assert(!IsIntraFrame(frame_header_.frame_type) ||
         frame_header_.primary_reference_frame == kPrimaryReferenceNone);
  if (frame_header_.primary_reference_frame == kPrimaryReferenceNone) {
    // Part of the setup_past_independence() function in the spec. It is not
    // necessary to set loop_filter->delta_enabled to true. See
    // https://crbug.com/aomedia/2305.
    SetDefaultRefDeltas(loop_filter);
  } else {
    // Part of the load_previous() function in the spec.
    const int prev_frame_index =
        frame_header_
            .reference_frame_index[frame_header_.primary_reference_frame];
    const RefCountedBuffer* prev_frame =
        decoder_state_.reference_frame[prev_frame_index].get();
    loop_filter->ref_deltas = prev_frame->loop_filter_ref_deltas();
    loop_filter->mode_deltas = prev_frame->loop_filter_mode_deltas();
  }
  int64_t scratch;
  for (int i = 0; i < 2; ++i) {
    OBU_READ_LITERAL_OR_FAIL(6);
    loop_filter->level[i] = scratch;
  }
  if (!sequence_header_.color_config.is_monochrome &&
      (loop_filter->level[0] != 0 || loop_filter->level[1] != 0)) {
    for (int i = 2; i < 4; ++i) {
      OBU_READ_LITERAL_OR_FAIL(6);
      loop_filter->level[i] = scratch;
    }
  }
  OBU_READ_LITERAL_OR_FAIL(3);
  loop_filter->sharpness = scratch;
  OBU_READ_BIT_OR_FAIL;
  loop_filter->delta_enabled = scratch != 0;
  if (loop_filter->delta_enabled) {
    OBU_READ_BIT_OR_FAIL;
    loop_filter->delta_update = scratch != 0;
    if (loop_filter->delta_update) {
      for (auto& ref_delta : loop_filter->ref_deltas) {
        OBU_READ_BIT_OR_FAIL;
        const bool update_ref_delta = scratch != 0;
        if (update_ref_delta) {
          int scratch_int;
          if (!bit_reader_->ReadInverseSignedLiteral(6, &scratch_int)) {
            LIBGAV1_DLOG(ERROR, "Not enough bits.");
            return false;
          }
          ref_delta = scratch_int;
        }
      }
      for (auto& mode_delta : loop_filter->mode_deltas) {
        OBU_READ_BIT_OR_FAIL;
        const bool update_mode_delta = scratch != 0;
        if (update_mode_delta) {
          int scratch_int;
          if (!bit_reader_->ReadInverseSignedLiteral(6, &scratch_int)) {
            LIBGAV1_DLOG(ERROR, "Not enough bits.");
            return false;
          }
          mode_delta = scratch_int;
        }
      }
    }
  } else {
    loop_filter->delta_update = false;
  }
  return true;
}

bool ObuParser::ParseDeltaQuantizer(int8_t* const delta) {
  int64_t scratch;
  *delta = 0;
  OBU_READ_BIT_OR_FAIL;
  const bool delta_coded = scratch != 0;
  if (delta_coded) {
    int scratch_int;
    if (!bit_reader_->ReadInverseSignedLiteral(6, &scratch_int)) {
      LIBGAV1_DLOG(ERROR, "Not enough bits.");
      return false;
    }
    *delta = scratch_int;
  }
  return true;
}

bool ObuParser::ParseQuantizerParameters() {
  int64_t scratch;
  QuantizerParameters* const quantizer = &frame_header_.quantizer;
  OBU_READ_LITERAL_OR_FAIL(8);
  quantizer->base_index = scratch;
  if (!ParseDeltaQuantizer(&quantizer->delta_dc[kPlaneY])) return false;
  if (!sequence_header_.color_config.is_monochrome) {
    bool diff_uv_delta = false;
    if (sequence_header_.color_config.separate_uv_delta_q) {
      OBU_READ_BIT_OR_FAIL;
      diff_uv_delta = scratch != 0;
    }
    if (!ParseDeltaQuantizer(&quantizer->delta_dc[kPlaneU]) ||
        !ParseDeltaQuantizer(&quantizer->delta_ac[kPlaneU])) {
      return false;
    }
    if (diff_uv_delta) {
      if (!ParseDeltaQuantizer(&quantizer->delta_dc[kPlaneV]) ||
          !ParseDeltaQuantizer(&quantizer->delta_ac[kPlaneV])) {
        return false;
      }
    } else {
      quantizer->delta_dc[kPlaneV] = quantizer->delta_dc[kPlaneU];
      quantizer->delta_ac[kPlaneV] = quantizer->delta_ac[kPlaneU];
    }
  }
  OBU_READ_BIT_OR_FAIL;
  quantizer->use_matrix = scratch != 0;
  if (quantizer->use_matrix) {
    OBU_READ_LITERAL_OR_FAIL(4);
    quantizer->matrix_level[kPlaneY] = scratch;
    OBU_READ_LITERAL_OR_FAIL(4);
    quantizer->matrix_level[kPlaneU] = scratch;
    if (sequence_header_.color_config.separate_uv_delta_q) {
      OBU_READ_LITERAL_OR_FAIL(4);
      quantizer->matrix_level[kPlaneV] = scratch;
    } else {
      quantizer->matrix_level[kPlaneV] = quantizer->matrix_level[kPlaneU];
    }
  }
  return true;
}

// This method implements the following functions in the spec:
// - segmentation_params()
// - part of setup_past_independence(): Set the FeatureData and FeatureEnabled
//   arrays to all 0.
// - part of load_previous(): Call load_segmentation_params().
//
// A careful analysis of the spec shows the part of setup_past_independence()
// can be optimized away and the part of load_previous() only needs to be
// invoked under a specific condition. Although the logic looks different from
// the spec, it is equivalent and more efficient.
bool ObuParser::ParseSegmentationParameters() {
  int64_t scratch;
  Segmentation* const segmentation = &frame_header_.segmentation;
  OBU_READ_BIT_OR_FAIL;
  segmentation->enabled = scratch != 0;
  if (!segmentation->enabled) return true;
  if (frame_header_.primary_reference_frame == kPrimaryReferenceNone) {
    segmentation->update_map = true;
    segmentation->update_data = true;
  } else {
    OBU_READ_BIT_OR_FAIL;
    segmentation->update_map = scratch != 0;
    if (segmentation->update_map) {
      OBU_READ_BIT_OR_FAIL;
      segmentation->temporal_update = scratch != 0;
    }
    OBU_READ_BIT_OR_FAIL;
    segmentation->update_data = scratch != 0;
    if (!segmentation->update_data) {
      // Part of the load_previous() function in the spec.
      const int prev_frame_index =
          frame_header_
              .reference_frame_index[frame_header_.primary_reference_frame];
      decoder_state_.reference_frame[prev_frame_index]
          ->GetSegmentationParameters(segmentation);
      return true;
    }
  }
  for (int8_t i = 0; i < kMaxSegments; ++i) {
    for (int8_t j = 0; j < kSegmentFeatureMax; ++j) {
      OBU_READ_BIT_OR_FAIL;
      segmentation->feature_enabled[i][j] = scratch != 0;
      if (segmentation->feature_enabled[i][j]) {
        if (Segmentation::FeatureSigned(static_cast<SegmentFeature>(j))) {
          int scratch_int;
          if (!bit_reader_->ReadInverseSignedLiteral(
                  kSegmentationFeatureBits[j], &scratch_int)) {
            LIBGAV1_DLOG(ERROR, "Not enough bits.");
            return false;
          }
          segmentation->feature_data[i][j] =
              Clip3(scratch_int, -kSegmentationFeatureMaxValues[j],
                    kSegmentationFeatureMaxValues[j]);
        } else {
          if (kSegmentationFeatureBits[j] > 0) {
            OBU_READ_LITERAL_OR_FAIL(kSegmentationFeatureBits[j]);
            segmentation->feature_data[i][j] = Clip3(
                static_cast<int>(scratch), 0, kSegmentationFeatureMaxValues[j]);
          } else {
            segmentation->feature_data[i][j] = 0;
          }
        }
        segmentation->last_active_segment_id = i;
        if (j >= kSegmentFeatureReferenceFrame) {
          segmentation->segment_id_pre_skip = true;
        }
      }
    }
  }
  return true;
}

bool ObuParser::ParseQuantizerIndexDeltaParameters() {
  int64_t scratch;
  if (frame_header_.quantizer.base_index > 0) {
    OBU_READ_BIT_OR_FAIL;
    frame_header_.delta_q.present = scratch != 0;
    if (frame_header_.delta_q.present) {
      OBU_READ_LITERAL_OR_FAIL(2);
      frame_header_.delta_q.scale = scratch;
    }
  }
  return true;
}

bool ObuParser::ParseLoopFilterDeltaParameters() {
  int64_t scratch;
  if (frame_header_.delta_q.present) {
    if (!frame_header_.allow_intrabc) {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.delta_lf.present = scratch != 0;
    }
    if (frame_header_.delta_lf.present) {
      OBU_READ_LITERAL_OR_FAIL(2);
      frame_header_.delta_lf.scale = scratch;
      OBU_READ_BIT_OR_FAIL;
      frame_header_.delta_lf.multi = scratch != 0;
    }
  }
  return true;
}

void ObuParser::ComputeSegmentLosslessAndQIndex() {
  frame_header_.coded_lossless = true;
  Segmentation* const segmentation = &frame_header_.segmentation;
  const QuantizerParameters* const quantizer = &frame_header_.quantizer;
  for (int i = 0; i < kMaxSegments; ++i) {
    segmentation->qindex[i] =
        GetQIndex(*segmentation, i, quantizer->base_index);
    segmentation->lossless[i] =
        segmentation->qindex[i] == 0 && quantizer->delta_dc[kPlaneY] == 0 &&
        quantizer->delta_dc[kPlaneU] == 0 &&
        quantizer->delta_ac[kPlaneU] == 0 &&
        quantizer->delta_dc[kPlaneV] == 0 && quantizer->delta_ac[kPlaneV] == 0;
    if (!segmentation->lossless[i]) frame_header_.coded_lossless = false;
    // The spec calls for setting up a two-dimensional SegQMLevel array here.
    // We avoid the SegQMLevel array by using segmentation->lossless[i] and
    // quantizer->matrix_level[plane] directly in the reconstruct process of
    // Section 7.12.3.
  }
  frame_header_.upscaled_lossless =
      frame_header_.coded_lossless &&
      frame_header_.width == frame_header_.upscaled_width;
}

bool ObuParser::ParseCdefParameters() {
  const int coeff_shift = sequence_header_.color_config.bitdepth - 8;
  if (frame_header_.coded_lossless || frame_header_.allow_intrabc ||
      !sequence_header_.enable_cdef) {
    frame_header_.cdef.damping = 3 + coeff_shift;
    return true;
  }
  Cdef* const cdef = &frame_header_.cdef;
  int64_t scratch;
  OBU_READ_LITERAL_OR_FAIL(2);
  cdef->damping = scratch + 3 + coeff_shift;
  OBU_READ_LITERAL_OR_FAIL(2);
  cdef->bits = scratch;
  for (int i = 0; i < (1 << cdef->bits); ++i) {
    OBU_READ_LITERAL_OR_FAIL(4);
    cdef->y_primary_strength[i] = scratch << coeff_shift;
    OBU_READ_LITERAL_OR_FAIL(2);
    cdef->y_secondary_strength[i] = scratch;
    if (cdef->y_secondary_strength[i] == 3) ++cdef->y_secondary_strength[i];
    cdef->y_secondary_strength[i] <<= coeff_shift;
    if (sequence_header_.color_config.is_monochrome) continue;
    OBU_READ_LITERAL_OR_FAIL(4);
    cdef->uv_primary_strength[i] = scratch << coeff_shift;
    OBU_READ_LITERAL_OR_FAIL(2);
    cdef->uv_secondary_strength[i] = scratch;
    if (cdef->uv_secondary_strength[i] == 3) ++cdef->uv_secondary_strength[i];
    cdef->uv_secondary_strength[i] <<= coeff_shift;
  }
  return true;
}

bool ObuParser::ParseLoopRestorationParameters() {
  if (frame_header_.upscaled_lossless || frame_header_.allow_intrabc ||
      !sequence_header_.enable_restoration) {
    return true;
  }
  int64_t scratch;
  bool uses_loop_restoration = false;
  bool uses_chroma_loop_restoration = false;
  LoopRestoration* const loop_restoration = &frame_header_.loop_restoration;
  const int num_planes = sequence_header_.color_config.is_monochrome
                             ? kMaxPlanesMonochrome
                             : kMaxPlanes;
  for (int i = 0; i < num_planes; ++i) {
    OBU_READ_LITERAL_OR_FAIL(2);
    loop_restoration->type[i] = static_cast<LoopRestorationType>(scratch);
    if (loop_restoration->type[i] != kLoopRestorationTypeNone) {
      uses_loop_restoration = true;
      if (i > 0) uses_chroma_loop_restoration = true;
    }
  }
  if (uses_loop_restoration) {
    uint8_t unit_shift;
    if (sequence_header_.use_128x128_superblock) {
      OBU_READ_BIT_OR_FAIL;
      unit_shift = scratch + 1;
    } else {
      OBU_READ_BIT_OR_FAIL;
      unit_shift = scratch;
      if (unit_shift != 0) {
        OBU_READ_BIT_OR_FAIL;
        const uint8_t unit_extra_shift = scratch;
        unit_shift += unit_extra_shift;
      }
    }
    loop_restoration->unit_size_log2[kPlaneY] = 6 + unit_shift;
    uint8_t uv_shift = 0;
    if (sequence_header_.color_config.subsampling_x != 0 &&
        sequence_header_.color_config.subsampling_y != 0 &&
        uses_chroma_loop_restoration) {
      OBU_READ_BIT_OR_FAIL;
      uv_shift = scratch;
    }
    loop_restoration->unit_size_log2[kPlaneU] =
        loop_restoration->unit_size_log2[kPlaneV] =
            loop_restoration->unit_size_log2[0] - uv_shift;
  }
  return true;
}

bool ObuParser::ParseTxModeSyntax() {
  if (frame_header_.coded_lossless) {
    frame_header_.tx_mode = kTxModeOnly4x4;
    return true;
  }
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  frame_header_.tx_mode = (scratch == 1) ? kTxModeSelect : kTxModeLargest;
  return true;
}

bool ObuParser::ParseFrameReferenceModeSyntax() {
  int64_t scratch;
  if (!IsIntraFrame(frame_header_.frame_type)) {
    OBU_READ_BIT_OR_FAIL;
    frame_header_.reference_mode_select = scratch != 0;
  }
  return true;
}

bool ObuParser::IsSkipModeAllowed() {
  if (IsIntraFrame(frame_header_.frame_type) ||
      !frame_header_.reference_mode_select ||
      !sequence_header_.enable_order_hint) {
    return false;
  }
  // Identify the nearest forward and backward references.
  int forward_index = -1;
  int backward_index = -1;
  int forward_hint = -1;
  int backward_hint = -1;
  for (int i = 0; i < kNumInterReferenceFrameTypes; ++i) {
    const unsigned int reference_hint =
        decoder_state_
            .reference_order_hint[frame_header_.reference_frame_index[i]];
    // TODO(linfengz): |relative_distance| equals
    // current_frame_->reference_info()->
    //     relative_distance_from[i + kReferenceFrameLast];
    // However, the unit test ObuParserTest.SkipModeParameters() would fail.
    // Will figure out how to initialize |current_frame_.reference_info_| in the
    // RefCountedBuffer later.
    const int relative_distance =
        GetRelativeDistance(reference_hint, frame_header_.order_hint,
                            sequence_header_.order_hint_shift_bits);
    if (relative_distance < 0) {
      if (forward_index < 0 ||
          GetRelativeDistance(reference_hint, forward_hint,
                              sequence_header_.order_hint_shift_bits) > 0) {
        forward_index = i;
        forward_hint = reference_hint;
      }
    } else if (relative_distance > 0) {
      if (backward_index < 0 ||
          GetRelativeDistance(reference_hint, backward_hint,
                              sequence_header_.order_hint_shift_bits) < 0) {
        backward_index = i;
        backward_hint = reference_hint;
      }
    }
  }
  if (forward_index < 0) return false;
  if (backward_index >= 0) {
    // Bidirectional prediction.
    frame_header_.skip_mode_frame[0] = static_cast<ReferenceFrameType>(
        kReferenceFrameLast + std::min(forward_index, backward_index));
    frame_header_.skip_mode_frame[1] = static_cast<ReferenceFrameType>(
        kReferenceFrameLast + std::max(forward_index, backward_index));
    return true;
  }
  // Forward prediction only. Identify the second nearest forward reference.
  int second_forward_index = -1;
  int second_forward_hint = -1;
  for (int i = 0; i < kNumInterReferenceFrameTypes; ++i) {
    const unsigned int reference_hint =
        decoder_state_
            .reference_order_hint[frame_header_.reference_frame_index[i]];
    if (GetRelativeDistance(reference_hint, forward_hint,
                            sequence_header_.order_hint_shift_bits) < 0) {
      if (second_forward_index < 0 ||
          GetRelativeDistance(reference_hint, second_forward_hint,
                              sequence_header_.order_hint_shift_bits) > 0) {
        second_forward_index = i;
        second_forward_hint = reference_hint;
      }
    }
  }
  if (second_forward_index < 0) return false;
  frame_header_.skip_mode_frame[0] = static_cast<ReferenceFrameType>(
      kReferenceFrameLast + std::min(forward_index, second_forward_index));
  frame_header_.skip_mode_frame[1] = static_cast<ReferenceFrameType>(
      kReferenceFrameLast + std::max(forward_index, second_forward_index));
  return true;
}

bool ObuParser::ParseSkipModeParameters() {
  if (!IsSkipModeAllowed()) return true;
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  frame_header_.skip_mode_present = scratch != 0;
  return true;
}

// Sets frame_header_.global_motion[ref].params[index].
bool ObuParser::ParseGlobalParamSyntax(
    int ref, int index,
    const std::array<GlobalMotion, kNumReferenceFrameTypes>&
        prev_global_motions) {
  GlobalMotion* const global_motion = &frame_header_.global_motion[ref];
  const GlobalMotion* const prev_global_motion = &prev_global_motions[ref];
  int abs_bits = kGlobalMotionAlphaBits;
  int precision_bits = kGlobalMotionAlphaPrecisionBits;
  if (index < 2) {
    if (global_motion->type == kGlobalMotionTransformationTypeTranslation) {
      const auto high_precision_mv_factor =
          static_cast<int>(!frame_header_.allow_high_precision_mv);
      abs_bits = kGlobalMotionTranslationOnlyBits - high_precision_mv_factor;
      precision_bits =
          kGlobalMotionTranslationOnlyPrecisionBits - high_precision_mv_factor;
    } else {
      abs_bits = kGlobalMotionTranslationBits;
      precision_bits = kGlobalMotionTranslationPrecisionBits;
    }
  }
  const int precision_diff = kWarpedModelPrecisionBits - precision_bits;
  const int round = (index % 3 == 2) ? 1 << kWarpedModelPrecisionBits : 0;
  const int sub = (index % 3 == 2) ? 1 << precision_bits : 0;
  const int mx = 1 << abs_bits;
  const int reference =
      (prev_global_motion->params[index] >> precision_diff) - sub;
  int scratch;
  if (!bit_reader_->DecodeSignedSubexpWithReference(
          -mx, mx + 1, reference, kGlobalMotionReadControl, &scratch)) {
    LIBGAV1_DLOG(ERROR, "Not enough bits.");
    return false;
  }
  global_motion->params[index] = LeftShift(scratch, precision_diff) + round;
  return true;
}

bool ObuParser::ParseGlobalMotionParameters() {
  for (int ref = kReferenceFrameLast; ref <= kReferenceFrameAlternate; ++ref) {
    frame_header_.global_motion[ref].type =
        kGlobalMotionTransformationTypeIdentity;
    for (int i = 0; i < 6; ++i) {
      frame_header_.global_motion[ref].params[i] =
          (i % 3 == 2) ? 1 << kWarpedModelPrecisionBits : 0;
    }
  }
  if (IsIntraFrame(frame_header_.frame_type)) return true;
  const std::array<GlobalMotion, kNumReferenceFrameTypes>* prev_global_motions =
      nullptr;
  if (frame_header_.primary_reference_frame == kPrimaryReferenceNone) {
    // Part of the setup_past_independence() function in the spec. The value
    // that the spec says PrevGmParams[ref][i] should be set to is exactly
    // the value frame_header_.global_motion[ref].params[i] is set to by the
    // for loop above. Therefore prev_global_motions can simply point to
    // frame_header_.global_motion.
    prev_global_motions = &frame_header_.global_motion;
  } else {
    // Part of the load_previous() function in the spec.
    const int prev_frame_index =
        frame_header_
            .reference_frame_index[frame_header_.primary_reference_frame];
    prev_global_motions =
        &decoder_state_.reference_frame[prev_frame_index]->GlobalMotions();
  }
  for (int ref = kReferenceFrameLast; ref <= kReferenceFrameAlternate; ++ref) {
    GlobalMotion* const global_motion = &frame_header_.global_motion[ref];
    int64_t scratch;
    OBU_READ_BIT_OR_FAIL;
    const bool is_global = scratch != 0;
    if (is_global) {
      OBU_READ_BIT_OR_FAIL;
      const bool is_rot_zoom = scratch != 0;
      if (is_rot_zoom) {
        global_motion->type = kGlobalMotionTransformationTypeRotZoom;
      } else {
        OBU_READ_BIT_OR_FAIL;
        const bool is_translation = scratch != 0;
        global_motion->type = is_translation
                                  ? kGlobalMotionTransformationTypeTranslation
                                  : kGlobalMotionTransformationTypeAffine;
      }
    } else {
      global_motion->type = kGlobalMotionTransformationTypeIdentity;
    }
    if (global_motion->type >= kGlobalMotionTransformationTypeRotZoom) {
      if (!ParseGlobalParamSyntax(ref, 2, *prev_global_motions) ||
          !ParseGlobalParamSyntax(ref, 3, *prev_global_motions)) {
        return false;
      }
      if (global_motion->type == kGlobalMotionTransformationTypeAffine) {
        if (!ParseGlobalParamSyntax(ref, 4, *prev_global_motions) ||
            !ParseGlobalParamSyntax(ref, 5, *prev_global_motions)) {
          return false;
        }
      } else {
        global_motion->params[4] = -global_motion->params[3];
        global_motion->params[5] = global_motion->params[2];
      }
    }
    if (global_motion->type >= kGlobalMotionTransformationTypeTranslation) {
      if (!ParseGlobalParamSyntax(ref, 0, *prev_global_motions) ||
          !ParseGlobalParamSyntax(ref, 1, *prev_global_motions)) {
        return false;
      }
    }
  }
  return true;
}

bool ObuParser::ParseFilmGrainParameters() {
  if (!sequence_header_.film_grain_params_present ||
      (!frame_header_.show_frame && !frame_header_.showable_frame)) {
    // frame_header_.film_grain_params is already zero-initialized.
    return true;
  }

  FilmGrainParams& film_grain_params = frame_header_.film_grain_params;
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  film_grain_params.apply_grain = scratch != 0;
  if (!film_grain_params.apply_grain) {
    // film_grain_params is already zero-initialized.
    return true;
  }

  OBU_READ_LITERAL_OR_FAIL(16);
  film_grain_params.grain_seed = static_cast<int>(scratch);
  film_grain_params.update_grain = true;
  if (frame_header_.frame_type == kFrameInter) {
    OBU_READ_BIT_OR_FAIL;
    film_grain_params.update_grain = scratch != 0;
  }
  if (!film_grain_params.update_grain) {
    OBU_READ_LITERAL_OR_FAIL(3);
    film_grain_params.reference_index = static_cast<int>(scratch);
    bool found = false;
    for (const auto index : frame_header_.reference_frame_index) {
      if (film_grain_params.reference_index == index) {
        found = true;
        break;
      }
    }
    if (!found) {
      static_assert(sizeof(frame_header_.reference_frame_index) /
                            sizeof(frame_header_.reference_frame_index[0]) ==
                        7,
                    "");
      LIBGAV1_DLOG(ERROR,
                   "Invalid value for film_grain_params_ref_idx (%d). "
                   "ref_frame_idx = {%d, %d, %d, %d, %d, %d, %d}",
                   film_grain_params.reference_index,
                   frame_header_.reference_frame_index[0],
                   frame_header_.reference_frame_index[1],
                   frame_header_.reference_frame_index[2],
                   frame_header_.reference_frame_index[3],
                   frame_header_.reference_frame_index[4],
                   frame_header_.reference_frame_index[5],
                   frame_header_.reference_frame_index[6]);
      return false;
    }
    const RefCountedBuffer* grain_params_reference_frame =
        decoder_state_.reference_frame[film_grain_params.reference_index].get();
    if (grain_params_reference_frame == nullptr) {
      LIBGAV1_DLOG(ERROR, "Buffer %d does not contain a decoded frame",
                   film_grain_params.reference_index);
      return false;
    }
    const int temp_grain_seed = film_grain_params.grain_seed;
    const bool temp_update_grain = film_grain_params.update_grain;
    const int temp_reference_index = film_grain_params.reference_index;
    film_grain_params = grain_params_reference_frame->film_grain_params();
    film_grain_params.grain_seed = temp_grain_seed;
    film_grain_params.update_grain = temp_update_grain;
    film_grain_params.reference_index = temp_reference_index;
    return true;
  }

  OBU_READ_LITERAL_OR_FAIL(4);
  film_grain_params.num_y_points = scratch;
  if (film_grain_params.num_y_points > 14) {
    LIBGAV1_DLOG(ERROR, "Invalid value for num_y_points (%d).",
                 film_grain_params.num_y_points);
    return false;
  }
  for (int i = 0; i < film_grain_params.num_y_points; ++i) {
    OBU_READ_LITERAL_OR_FAIL(8);
    film_grain_params.point_y_value[i] = scratch;
    if (i != 0 && film_grain_params.point_y_value[i - 1] >=
                      film_grain_params.point_y_value[i]) {
      LIBGAV1_DLOG(ERROR, "point_y_value[%d] (%d) >= point_y_value[%d] (%d).",
                   i - 1, film_grain_params.point_y_value[i - 1], i,
                   film_grain_params.point_y_value[i]);
      return false;
    }
    OBU_READ_LITERAL_OR_FAIL(8);
    film_grain_params.point_y_scaling[i] = scratch;
  }
  if (sequence_header_.color_config.is_monochrome) {
    film_grain_params.chroma_scaling_from_luma = false;
  } else {
    OBU_READ_BIT_OR_FAIL;
    film_grain_params.chroma_scaling_from_luma = scratch != 0;
  }
  if (sequence_header_.color_config.is_monochrome ||
      film_grain_params.chroma_scaling_from_luma ||
      (sequence_header_.color_config.subsampling_x == 1 &&
       sequence_header_.color_config.subsampling_y == 1 &&
       film_grain_params.num_y_points == 0)) {
    film_grain_params.num_u_points = 0;
    film_grain_params.num_v_points = 0;
  } else {
    OBU_READ_LITERAL_OR_FAIL(4);
    film_grain_params.num_u_points = scratch;
    if (film_grain_params.num_u_points > 10) {
      LIBGAV1_DLOG(ERROR, "Invalid value for num_u_points (%d).",
                   film_grain_params.num_u_points);
      return false;
    }
    for (int i = 0; i < film_grain_params.num_u_points; ++i) {
      OBU_READ_LITERAL_OR_FAIL(8);
      film_grain_params.point_u_value[i] = scratch;
      if (i != 0 && film_grain_params.point_u_value[i - 1] >=
                        film_grain_params.point_u_value[i]) {
        LIBGAV1_DLOG(ERROR, "point_u_value[%d] (%d) >= point_u_value[%d] (%d).",
                     i - 1, film_grain_params.point_u_value[i - 1], i,
                     film_grain_params.point_u_value[i]);
        return false;
      }
      OBU_READ_LITERAL_OR_FAIL(8);
      film_grain_params.point_u_scaling[i] = scratch;
    }
    OBU_READ_LITERAL_OR_FAIL(4);
    film_grain_params.num_v_points = scratch;
    if (film_grain_params.num_v_points > 10) {
      LIBGAV1_DLOG(ERROR, "Invalid value for num_v_points (%d).",
                   film_grain_params.num_v_points);
      return false;
    }
    if (sequence_header_.color_config.subsampling_x == 1 &&
        sequence_header_.color_config.subsampling_y == 1 &&
        (film_grain_params.num_u_points == 0) !=
            (film_grain_params.num_v_points == 0)) {
      LIBGAV1_DLOG(ERROR,
                   "Invalid values for num_u_points (%d) and num_v_points (%d) "
                   "for 4:2:0 chroma subsampling.",
                   film_grain_params.num_u_points,
                   film_grain_params.num_v_points);
      return false;
    }
    for (int i = 0; i < film_grain_params.num_v_points; ++i) {
      OBU_READ_LITERAL_OR_FAIL(8);
      film_grain_params.point_v_value[i] = scratch;
      if (i != 0 && film_grain_params.point_v_value[i - 1] >=
                        film_grain_params.point_v_value[i]) {
        LIBGAV1_DLOG(ERROR, "point_v_value[%d] (%d) >= point_v_value[%d] (%d).",
                     i - 1, film_grain_params.point_v_value[i - 1], i,
                     film_grain_params.point_v_value[i]);
        return false;
      }
      OBU_READ_LITERAL_OR_FAIL(8);
      film_grain_params.point_v_scaling[i] = scratch;
    }
  }
  OBU_READ_LITERAL_OR_FAIL(2);
  film_grain_params.chroma_scaling = scratch + 8;
  OBU_READ_LITERAL_OR_FAIL(2);
  film_grain_params.auto_regression_coeff_lag = scratch;

  const int num_pos_y =
      MultiplyBy2(film_grain_params.auto_regression_coeff_lag) *
      (film_grain_params.auto_regression_coeff_lag + 1);
  int num_pos_uv = num_pos_y;
  if (film_grain_params.num_y_points > 0) {
    ++num_pos_uv;
    for (int i = 0; i < num_pos_y; ++i) {
      OBU_READ_LITERAL_OR_FAIL(8);
      film_grain_params.auto_regression_coeff_y[i] =
          static_cast<int8_t>(scratch - 128);
    }
  }
  if (film_grain_params.chroma_scaling_from_luma ||
      film_grain_params.num_u_points > 0) {
    for (int i = 0; i < num_pos_uv; ++i) {
      OBU_READ_LITERAL_OR_FAIL(8);
      film_grain_params.auto_regression_coeff_u[i] =
          static_cast<int8_t>(scratch - 128);
    }
  }
  if (film_grain_params.chroma_scaling_from_luma ||
      film_grain_params.num_v_points > 0) {
    for (int i = 0; i < num_pos_uv; ++i) {
      OBU_READ_LITERAL_OR_FAIL(8);
      film_grain_params.auto_regression_coeff_v[i] =
          static_cast<int8_t>(scratch - 128);
    }
  }
  OBU_READ_LITERAL_OR_FAIL(2);
  film_grain_params.auto_regression_shift = static_cast<uint8_t>(scratch + 6);
  OBU_READ_LITERAL_OR_FAIL(2);
  film_grain_params.grain_scale_shift = static_cast<int>(scratch);
  if (film_grain_params.num_u_points > 0) {
    OBU_READ_LITERAL_OR_FAIL(8);
    film_grain_params.u_multiplier = static_cast<int8_t>(scratch - 128);
    OBU_READ_LITERAL_OR_FAIL(8);
    film_grain_params.u_luma_multiplier = static_cast<int8_t>(scratch - 128);
    OBU_READ_LITERAL_OR_FAIL(9);
    film_grain_params.u_offset = static_cast<int16_t>(scratch - 256);
  }
  if (film_grain_params.num_v_points > 0) {
    OBU_READ_LITERAL_OR_FAIL(8);
    film_grain_params.v_multiplier = static_cast<int8_t>(scratch - 128);
    OBU_READ_LITERAL_OR_FAIL(8);
    film_grain_params.v_luma_multiplier = static_cast<int8_t>(scratch - 128);
    OBU_READ_LITERAL_OR_FAIL(9);
    film_grain_params.v_offset = static_cast<int16_t>(scratch - 256);
  }
  OBU_READ_BIT_OR_FAIL;
  film_grain_params.overlap_flag = scratch != 0;
  OBU_READ_BIT_OR_FAIL;
  film_grain_params.clip_to_restricted_range = scratch != 0;
  return true;
}

bool ObuParser::ParseTileInfoSyntax() {
  TileInfo* const tile_info = &frame_header_.tile_info;
  const int sb_columns = sequence_header_.use_128x128_superblock
                             ? ((frame_header_.columns4x4 + 31) >> 5)
                             : ((frame_header_.columns4x4 + 15) >> 4);
  const int sb_rows = sequence_header_.use_128x128_superblock
                          ? ((frame_header_.rows4x4 + 31) >> 5)
                          : ((frame_header_.rows4x4 + 15) >> 4);
  tile_info->sb_columns = sb_columns;
  tile_info->sb_rows = sb_rows;
  const int sb_shift = sequence_header_.use_128x128_superblock ? 5 : 4;
  const int sb_size = 2 + sb_shift;
  const int sb_max_tile_width = kMaxTileWidth >> sb_size;
  const int sb_max_tile_area = kMaxTileArea >> MultiplyBy2(sb_size);
  const int minlog2_tile_columns = TileLog2(sb_max_tile_width, sb_columns);
  const int maxlog2_tile_columns =
      CeilLog2(std::min(sb_columns, static_cast<int>(kMaxTileColumns)));
  const int maxlog2_tile_rows =
      CeilLog2(std::min(sb_rows, static_cast<int>(kMaxTileRows)));
  const int min_log2_tiles = std::max(
      minlog2_tile_columns, TileLog2(sb_max_tile_area, sb_rows * sb_columns));
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  tile_info->uniform_spacing = scratch != 0;
  if (tile_info->uniform_spacing) {
    // Read tile columns.
    tile_info->tile_columns_log2 = minlog2_tile_columns;
    while (tile_info->tile_columns_log2 < maxlog2_tile_columns) {
      OBU_READ_BIT_OR_FAIL;
      if (scratch == 0) break;
      ++tile_info->tile_columns_log2;
    }

    // Compute tile column starts.
    const int sb_tile_width =
        (sb_columns + (1 << tile_info->tile_columns_log2) - 1) >>
        tile_info->tile_columns_log2;
    if (sb_tile_width <= 0) return false;
    int i = 0;
    for (int sb_start = 0; sb_start < sb_columns; sb_start += sb_tile_width) {
      if (i >= kMaxTileColumns) {
        LIBGAV1_DLOG(ERROR,
                     "tile_columns would be greater than kMaxTileColumns.");
        return false;
      }
      tile_info->tile_column_start[i++] = sb_start << sb_shift;
    }
    tile_info->tile_column_start[i] = frame_header_.columns4x4;
    tile_info->tile_columns = i;

    // Read tile rows.
    const int minlog2_tile_rows =
        std::max(min_log2_tiles - tile_info->tile_columns_log2, 0);
    tile_info->tile_rows_log2 = minlog2_tile_rows;
    while (tile_info->tile_rows_log2 < maxlog2_tile_rows) {
      OBU_READ_BIT_OR_FAIL;
      if (scratch == 0) break;
      ++tile_info->tile_rows_log2;
    }

    // Compute tile row starts.
    const int sb_tile_height =
        (sb_rows + (1 << tile_info->tile_rows_log2) - 1) >>
        tile_info->tile_rows_log2;
    if (sb_tile_height <= 0) return false;
    i = 0;
    for (int sb_start = 0; sb_start < sb_rows; sb_start += sb_tile_height) {
      if (i >= kMaxTileRows) {
        LIBGAV1_DLOG(ERROR, "tile_rows would be greater than kMaxTileRows.");
        return false;
      }
      tile_info->tile_row_start[i++] = sb_start << sb_shift;
    }
    tile_info->tile_row_start[i] = frame_header_.rows4x4;
    tile_info->tile_rows = i;
  } else {
    int widest_tile_sb = 1;
    int i = 0;
    for (int sb_start = 0; sb_start < sb_columns; ++i) {
      if (i >= kMaxTileColumns) {
        LIBGAV1_DLOG(ERROR,
                     "tile_columns would be greater than kMaxTileColumns.");
        return false;
      }
      tile_info->tile_column_start[i] = sb_start << sb_shift;
      const int max_width =
          std::min(sb_columns - sb_start, static_cast<int>(sb_max_tile_width));
      if (!bit_reader_->DecodeUniform(
              max_width, &tile_info->tile_column_width_in_superblocks[i])) {
        LIBGAV1_DLOG(ERROR, "Not enough bits.");
        return false;
      }
      ++tile_info->tile_column_width_in_superblocks[i];
      widest_tile_sb = std::max(tile_info->tile_column_width_in_superblocks[i],
                                widest_tile_sb);
      sb_start += tile_info->tile_column_width_in_superblocks[i];
    }
    tile_info->tile_column_start[i] = frame_header_.columns4x4;
    tile_info->tile_columns = i;
    tile_info->tile_columns_log2 = CeilLog2(tile_info->tile_columns);

    int max_tile_area_sb = sb_rows * sb_columns;
    if (min_log2_tiles > 0) max_tile_area_sb >>= min_log2_tiles + 1;
    const int max_tile_height_sb =
        std::max(max_tile_area_sb / widest_tile_sb, 1);

    i = 0;
    for (int sb_start = 0; sb_start < sb_rows; ++i) {
      if (i >= kMaxTileRows) {
        LIBGAV1_DLOG(ERROR, "tile_rows would be greater than kMaxTileRows.");
        return false;
      }
      tile_info->tile_row_start[i] = sb_start << sb_shift;
      const int max_height = std::min(sb_rows - sb_start, max_tile_height_sb);
      if (!bit_reader_->DecodeUniform(
              max_height, &tile_info->tile_row_height_in_superblocks[i])) {
        LIBGAV1_DLOG(ERROR, "Not enough bits.");
        return false;
      }
      ++tile_info->tile_row_height_in_superblocks[i];
      sb_start += tile_info->tile_row_height_in_superblocks[i];
    }
    tile_info->tile_row_start[i] = frame_header_.rows4x4;
    tile_info->tile_rows = i;
    tile_info->tile_rows_log2 = CeilLog2(tile_info->tile_rows);
  }
  tile_info->tile_count = tile_info->tile_rows * tile_info->tile_columns;
  if (!tile_buffers_.reserve(tile_info->tile_count)) {
    LIBGAV1_DLOG(ERROR, "Unable to allocate memory for tile_buffers_.");
    return false;
  }
  tile_info->context_update_id = 0;
  const int tile_bits =
      tile_info->tile_columns_log2 + tile_info->tile_rows_log2;
  if (tile_bits != 0) {
    OBU_READ_LITERAL_OR_FAIL(tile_bits);
    tile_info->context_update_id = static_cast<int16_t>(scratch);
    if (tile_info->context_update_id >= tile_info->tile_count) {
      LIBGAV1_DLOG(ERROR, "Invalid context_update_tile_id (%d) >= %d.",
                   tile_info->context_update_id, tile_info->tile_count);
      return false;
    }
    OBU_READ_LITERAL_OR_FAIL(2);
    tile_info->tile_size_bytes = 1 + scratch;
  }
  return true;
}

bool ObuParser::ReadAllowWarpedMotion() {
  if (IsIntraFrame(frame_header_.frame_type) ||
      frame_header_.error_resilient_mode ||
      !sequence_header_.enable_warped_motion) {
    return true;
  }
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  frame_header_.allow_warped_motion = scratch != 0;
  return true;
}

bool ObuParser::ParseFrameParameters() {
  int64_t scratch;
  if (sequence_header_.reduced_still_picture_header) {
    frame_header_.show_frame = true;
    if (!EnsureCurrentFrameIsNotNull()) return false;
  } else {
    OBU_READ_BIT_OR_FAIL;
    frame_header_.show_existing_frame = scratch != 0;
    if (frame_header_.show_existing_frame) {
      OBU_READ_LITERAL_OR_FAIL(3);
      frame_header_.frame_to_show = scratch;
      if (sequence_header_.decoder_model_info_present_flag &&
          !sequence_header_.timing_info.equal_picture_interval) {
        OBU_READ_LITERAL_OR_FAIL(
            sequence_header_.decoder_model_info.frame_presentation_time_length);
        frame_header_.frame_presentation_time = static_cast<uint32_t>(scratch);
      }
      if (sequence_header_.frame_id_numbers_present) {
        OBU_READ_LITERAL_OR_FAIL(sequence_header_.frame_id_length_bits);
        frame_header_.display_frame_id = static_cast<uint16_t>(scratch);
        // Section 6.8.2: It is a requirement of bitstream conformance that
        // whenever display_frame_id is read, the value matches
        // RefFrameId[ frame_to_show_map_idx ] ..., and that
        // RefValid[ frame_to_show_map_idx ] is equal to 1.
        //
        // The current_frame_ == nullptr check below is equivalent to checking
        // if RefValid[ frame_to_show_map_idx ] is equal to 1.
        if (frame_header_.display_frame_id !=
            decoder_state_.reference_frame_id[frame_header_.frame_to_show]) {
          LIBGAV1_DLOG(ERROR,
                       "Reference buffer %d has a frame id number mismatch.",
                       frame_header_.frame_to_show);
          return false;
        }
      }
      // Section 7.18.2. Note: This is also needed for Section 7.21 if
      // frame_type is kFrameKey.
      current_frame_ =
          decoder_state_.reference_frame[frame_header_.frame_to_show];
      if (current_frame_ == nullptr) {
        LIBGAV1_DLOG(ERROR, "Buffer %d does not contain a decoded frame",
                     frame_header_.frame_to_show);
        return false;
      }
      // Section 6.8.2: It is a requirement of bitstream conformance that
      // when show_existing_frame is used to show a previous frame, that the
      // value of showable_frame for the previous frame was equal to 1.
      if (!current_frame_->showable_frame()) {
        LIBGAV1_DLOG(ERROR, "Buffer %d does not contain a showable frame",
                     frame_header_.frame_to_show);
        return false;
      }
      if (current_frame_->frame_type() == kFrameKey) {
        frame_header_.refresh_frame_flags = 0xff;
        // Section 6.8.2: It is a requirement of bitstream conformance that
        // when show_existing_frame is used to show a previous frame with
        // RefFrameType[ frame_to_show_map_idx ] equal to KEY_FRAME, that
        // the frame is output via the show_existing_frame mechanism at most
        // once.
        current_frame_->set_showable_frame(false);

        // Section 7.21. Note: decoder_state_.current_frame_id must be set
        // only when frame_type is kFrameKey per the spec. Among all the
        // variables set in Section 7.21, current_frame_id is the only one
        // whose value lives across frames. (PrevFrameID is set equal to the
        // current_frame_id value for the previous frame.)
        decoder_state_.current_frame_id =
            decoder_state_.reference_frame_id[frame_header_.frame_to_show];
        decoder_state_.order_hint =
            decoder_state_.reference_order_hint[frame_header_.frame_to_show];
      }
      return true;
    }
    if (!EnsureCurrentFrameIsNotNull()) return false;
    OBU_READ_LITERAL_OR_FAIL(2);
    frame_header_.frame_type = static_cast<FrameType>(scratch);
    current_frame_->set_frame_type(frame_header_.frame_type);
    OBU_READ_BIT_OR_FAIL;
    frame_header_.show_frame = scratch != 0;
    if (frame_header_.show_frame &&
        sequence_header_.decoder_model_info_present_flag &&
        !sequence_header_.timing_info.equal_picture_interval) {
      OBU_READ_LITERAL_OR_FAIL(
          sequence_header_.decoder_model_info.frame_presentation_time_length);
      frame_header_.frame_presentation_time = static_cast<uint32_t>(scratch);
    }
    if (frame_header_.show_frame) {
      frame_header_.showable_frame = (frame_header_.frame_type != kFrameKey);
    } else {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.showable_frame = scratch != 0;
    }
    current_frame_->set_showable_frame(frame_header_.showable_frame);
    if (frame_header_.frame_type == kFrameSwitch ||
        (frame_header_.frame_type == kFrameKey && frame_header_.show_frame)) {
      frame_header_.error_resilient_mode = true;
    } else {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.error_resilient_mode = scratch != 0;
    }
  }
  if (frame_header_.frame_type == kFrameKey && frame_header_.show_frame) {
    decoder_state_.reference_order_hint.fill(0);
    decoder_state_.reference_frame.fill(nullptr);
  }
  OBU_READ_BIT_OR_FAIL;
  frame_header_.enable_cdf_update = scratch == 0;
  if (sequence_header_.force_screen_content_tools ==
      kSelectScreenContentTools) {
    OBU_READ_BIT_OR_FAIL;
    frame_header_.allow_screen_content_tools = scratch != 0;
  } else {
    frame_header_.allow_screen_content_tools =
        sequence_header_.force_screen_content_tools != 0;
  }
  if (frame_header_.allow_screen_content_tools) {
    if (sequence_header_.force_integer_mv == kSelectIntegerMv) {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.force_integer_mv = scratch;
    } else {
      frame_header_.force_integer_mv = sequence_header_.force_integer_mv;
    }
  } else {
    frame_header_.force_integer_mv = 0;
  }
  if (IsIntraFrame(frame_header_.frame_type)) {
    frame_header_.force_integer_mv = 1;
  }
  if (sequence_header_.frame_id_numbers_present) {
    OBU_READ_LITERAL_OR_FAIL(sequence_header_.frame_id_length_bits);
    frame_header_.current_frame_id = static_cast<uint16_t>(scratch);
    const int previous_frame_id = decoder_state_.current_frame_id;
    decoder_state_.current_frame_id = frame_header_.current_frame_id;
    if (frame_header_.frame_type != kFrameKey || !frame_header_.show_frame) {
      if (previous_frame_id >= 0) {
        // Section 6.8.2: ..., it is a requirement of bitstream conformance
        // that all of the following conditions are true:
        //   * current_frame_id is not equal to PrevFrameID,
        //   * DiffFrameID is less than 1 << ( idLen - 1 )
        int diff_frame_id = decoder_state_.current_frame_id - previous_frame_id;
        const int id_length_max_value =
            1 << sequence_header_.frame_id_length_bits;
        if (diff_frame_id <= 0) {
          diff_frame_id += id_length_max_value;
        }
        if (diff_frame_id >= DivideBy2(id_length_max_value)) {
          LIBGAV1_DLOG(ERROR,
                       "current_frame_id (%d) equals or differs too much from "
                       "previous_frame_id (%d).",
                       decoder_state_.current_frame_id, previous_frame_id);
          return false;
        }
      }
      MarkInvalidReferenceFrames();
    }
  } else {
    frame_header_.current_frame_id = 0;
    decoder_state_.current_frame_id = frame_header_.current_frame_id;
  }
  if (frame_header_.frame_type == kFrameSwitch) {
    frame_header_.frame_size_override_flag = true;
  } else if (!sequence_header_.reduced_still_picture_header) {
    OBU_READ_BIT_OR_FAIL;
    frame_header_.frame_size_override_flag = scratch != 0;
  }
  if (sequence_header_.order_hint_bits > 0) {
    OBU_READ_LITERAL_OR_FAIL(sequence_header_.order_hint_bits);
    frame_header_.order_hint = scratch;
  }
  decoder_state_.order_hint = frame_header_.order_hint;
  if (IsIntraFrame(frame_header_.frame_type) ||
      frame_header_.error_resilient_mode) {
    frame_header_.primary_reference_frame = kPrimaryReferenceNone;
  } else {
    OBU_READ_LITERAL_OR_FAIL(3);
    frame_header_.primary_reference_frame = scratch;
  }
  if (sequence_header_.decoder_model_info_present_flag) {
    OBU_READ_BIT_OR_FAIL;
    const bool buffer_removal_time_present = scratch != 0;
    if (buffer_removal_time_present) {
      for (int i = 0; i < sequence_header_.operating_points; ++i) {
        if (!sequence_header_.decoder_model_present_for_operating_point[i]) {
          continue;
        }
        const int index = sequence_header_.operating_point_idc[i];
        if (index == 0 ||
            (InTemporalLayer(index, obu_headers_.back().temporal_id) &&
             InSpatialLayer(index, obu_headers_.back().spatial_id))) {
          OBU_READ_LITERAL_OR_FAIL(
              sequence_header_.decoder_model_info.buffer_removal_time_length);
          frame_header_.buffer_removal_time[i] = static_cast<uint32_t>(scratch);
        }
      }
    }
  }
  if (frame_header_.frame_type == kFrameSwitch ||
      (frame_header_.frame_type == kFrameKey && frame_header_.show_frame)) {
    frame_header_.refresh_frame_flags = 0xff;
  } else {
    OBU_READ_LITERAL_OR_FAIL(8);
    frame_header_.refresh_frame_flags = scratch;
    // Section 6.8.2: If frame_type is equal to INTRA_ONLY_FRAME, it is a
    // requirement of bitstream conformance that refresh_frame_flags is not
    // equal to 0xff.
    if (frame_header_.frame_type == kFrameIntraOnly &&
        frame_header_.refresh_frame_flags == 0xff) {
      LIBGAV1_DLOG(ERROR, "Intra only frames cannot have refresh flags 0xFF.");
      return false;
    }
  }
  if ((!IsIntraFrame(frame_header_.frame_type) ||
       frame_header_.refresh_frame_flags != 0xff) &&
      !ParseReferenceOrderHint()) {
    return false;
  }
  if (IsIntraFrame(frame_header_.frame_type)) {
    if (!ParseFrameSizeAndRenderSize()) return false;
    if (frame_header_.allow_screen_content_tools &&
        frame_header_.width == frame_header_.upscaled_width) {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.allow_intrabc = scratch != 0;
    }
  } else {
    if (!sequence_header_.enable_order_hint) {
      frame_header_.frame_refs_short_signaling = false;
    } else {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.frame_refs_short_signaling = scratch != 0;
      if (frame_header_.frame_refs_short_signaling) {
        OBU_READ_LITERAL_OR_FAIL(3);
        const int8_t last_frame_idx = scratch;
        OBU_READ_LITERAL_OR_FAIL(3);
        const int8_t gold_frame_idx = scratch;
        if (!SetFrameReferences(last_frame_idx, gold_frame_idx)) {
          return false;
        }
      }
    }
    for (int i = 0; i < kNumInterReferenceFrameTypes; ++i) {
      if (!frame_header_.frame_refs_short_signaling) {
        OBU_READ_LITERAL_OR_FAIL(3);
        frame_header_.reference_frame_index[i] = scratch;
      }
      const int reference_frame_index = frame_header_.reference_frame_index[i];
      assert(reference_frame_index >= 0);
      // Section 6.8.2: It is a requirement of bitstream conformance that
      // RefValid[ ref_frame_idx[ i ] ] is equal to 1 ...
      // The remainder of the statement is handled by ParseSequenceHeader().
      // Note if support for Annex C: Error resilience behavior is added this
      // check should be omitted per C.5 Decoder consequences of processable
      // frames.
      if (decoder_state_.reference_frame[reference_frame_index] == nullptr) {
        LIBGAV1_DLOG(ERROR, "ref_frame_idx[%d] (%d) is not valid.", i,
                     reference_frame_index);
        return false;
      }
      if (sequence_header_.frame_id_numbers_present) {
        OBU_READ_LITERAL_OR_FAIL(sequence_header_.delta_frame_id_length_bits);
        const int delta_frame_id = static_cast<int>(1 + scratch);
        const int id_length_max_value =
            1 << sequence_header_.frame_id_length_bits;
        frame_header_.expected_frame_id[i] =
            (frame_header_.current_frame_id + id_length_max_value -
             delta_frame_id) %
            id_length_max_value;
        // Section 6.8.2: It is a requirement of bitstream conformance that
        // whenever expectedFrameId[ i ] is calculated, the value matches
        // RefFrameId[ ref_frame_idx[ i ] ] ...
        if (frame_header_.expected_frame_id[i] !=
            decoder_state_.reference_frame_id[reference_frame_index]) {
          LIBGAV1_DLOG(ERROR,
                       "Reference buffer %d has a frame id number mismatch.",
                       reference_frame_index);
          return false;
        }
      }
    }
    if (frame_header_.frame_size_override_flag &&
        !frame_header_.error_resilient_mode) {
      // Section 5.9.7.
      for (int index : frame_header_.reference_frame_index) {
        OBU_READ_BIT_OR_FAIL;
        frame_header_.found_reference = scratch != 0;
        if (frame_header_.found_reference) {
          const RefCountedBuffer* reference_frame =
              decoder_state_.reference_frame[index].get();
          // frame_header_.upscaled_width will be set in the
          // ParseSuperResParametersAndComputeImageSize() call below.
          frame_header_.width = reference_frame->upscaled_width();
          frame_header_.height = reference_frame->frame_height();
          frame_header_.render_width = reference_frame->render_width();
          frame_header_.render_height = reference_frame->render_height();
          if (!ParseSuperResParametersAndComputeImageSize()) return false;
          break;
        }
      }
      if (!frame_header_.found_reference && !ParseFrameSizeAndRenderSize()) {
        return false;
      }
    } else {
      if (!ParseFrameSizeAndRenderSize()) return false;
    }
    if (!ValidateInterFrameSize()) return false;
    if (frame_header_.force_integer_mv != 0) {
      frame_header_.allow_high_precision_mv = false;
    } else {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.allow_high_precision_mv = scratch != 0;
    }
    OBU_READ_BIT_OR_FAIL;
    const bool is_filter_switchable = scratch != 0;
    if (is_filter_switchable) {
      frame_header_.interpolation_filter = kInterpolationFilterSwitchable;
    } else {
      OBU_READ_LITERAL_OR_FAIL(2);
      frame_header_.interpolation_filter =
          static_cast<InterpolationFilter>(scratch);
    }
    OBU_READ_BIT_OR_FAIL;
    frame_header_.is_motion_mode_switchable = scratch != 0;
    if (frame_header_.error_resilient_mode ||
        !sequence_header_.enable_ref_frame_mvs) {
      frame_header_.use_ref_frame_mvs = false;
    } else {
      OBU_READ_BIT_OR_FAIL;
      frame_header_.use_ref_frame_mvs = scratch != 0;
    }
  }
  // At this point, we have parsed the frame and render sizes and computed
  // the image size, whether it's an intra or inter frame. So we can save
  // the sizes in the current frame now.
  if (!current_frame_->SetFrameDimensions(frame_header_)) {
    LIBGAV1_DLOG(ERROR, "Setting current frame dimensions failed.");
    return false;
  }
  if (!IsIntraFrame(frame_header_.frame_type)) {
    // Initialize the kReferenceFrameIntra type reference frame information to
    // simplify the frame type validation in motion field projection.
    // Set the kReferenceFrameIntra type |order_hint_| to
    // |frame_header_.order_hint|. This guarantees that in SIMD implementations,
    // the other reference frame information of the kReferenceFrameIntra type
    // could be correctly initialized using the following loop with
    // |frame_header_.order_hint| being the |hint|.
    ReferenceInfo* const reference_info = current_frame_->reference_info();
    reference_info->order_hint[kReferenceFrameIntra] = frame_header_.order_hint;
    reference_info->relative_distance_from[kReferenceFrameIntra] = 0;
    reference_info->relative_distance_to[kReferenceFrameIntra] = 0;
    reference_info->skip_references[kReferenceFrameIntra] = true;
    reference_info->projection_divisions[kReferenceFrameIntra] = 0;

    for (int i = kReferenceFrameLast; i <= kNumInterReferenceFrameTypes; ++i) {
      const auto reference_frame = static_cast<ReferenceFrameType>(i);
      const uint8_t hint =
          decoder_state_.reference_order_hint
              [frame_header_.reference_frame_index[i - kReferenceFrameLast]];
      reference_info->order_hint[reference_frame] = hint;
      const int relative_distance_from =
          GetRelativeDistance(hint, frame_header_.order_hint,
                              sequence_header_.order_hint_shift_bits);
      const int relative_distance_to =
          GetRelativeDistance(frame_header_.order_hint, hint,
                              sequence_header_.order_hint_shift_bits);
      reference_info->relative_distance_from[reference_frame] =
          relative_distance_from;
      reference_info->relative_distance_to[reference_frame] =
          relative_distance_to;
      reference_info->skip_references[reference_frame] =
          relative_distance_to > kMaxFrameDistance || relative_distance_to <= 0;
      reference_info->projection_divisions[reference_frame] =
          reference_info->skip_references[reference_frame]
              ? 0
              : kProjectionMvDivisionLookup[relative_distance_to];
      decoder_state_.reference_frame_sign_bias[reference_frame] =
          relative_distance_from > 0;
    }
  }
  if (frame_header_.enable_cdf_update &&
      !sequence_header_.reduced_still_picture_header) {
    OBU_READ_BIT_OR_FAIL;
    frame_header_.enable_frame_end_update_cdf = scratch == 0;
  } else {
    frame_header_.enable_frame_end_update_cdf = false;
  }
  return true;
}

bool ObuParser::ParseFrameHeader() {
  // Section 6.8.1: It is a requirement of bitstream conformance that a
  // sequence header OBU has been received before a frame header OBU.
  if (!has_sequence_header_) return false;
  if (!ParseFrameParameters()) return false;
  if (frame_header_.show_existing_frame) return true;
  assert(!obu_headers_.empty());
  current_frame_->set_spatial_id(obu_headers_.back().spatial_id);
  current_frame_->set_temporal_id(obu_headers_.back().temporal_id);
  bool status = ParseTileInfoSyntax() && ParseQuantizerParameters() &&
                ParseSegmentationParameters();
  if (!status) return false;
  current_frame_->SetSegmentationParameters(frame_header_.segmentation);
  status =
      ParseQuantizerIndexDeltaParameters() && ParseLoopFilterDeltaParameters();
  if (!status) return false;
  ComputeSegmentLosslessAndQIndex();
  // Section 6.8.2: It is a requirement of bitstream conformance that
  // delta_q_present is equal to 0 when CodedLossless is equal to 1.
  if (frame_header_.coded_lossless && frame_header_.delta_q.present) {
    return false;
  }
  status = ParseLoopFilterParameters();
  if (!status) return false;
  current_frame_->SetLoopFilterDeltas(frame_header_.loop_filter);
  status = ParseCdefParameters() && ParseLoopRestorationParameters() &&
           ParseTxModeSyntax() && ParseFrameReferenceModeSyntax() &&
           ParseSkipModeParameters() && ReadAllowWarpedMotion();
  if (!status) return false;
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  frame_header_.reduced_tx_set = scratch != 0;
  status = ParseGlobalMotionParameters();
  if (!status) return false;
  current_frame_->SetGlobalMotions(frame_header_.global_motion);
  status = ParseFilmGrainParameters();
  if (!status) return false;
  if (sequence_header_.film_grain_params_present) {
    current_frame_->set_film_grain_params(frame_header_.film_grain_params);
  }
  return true;
}

bool ObuParser::ParsePadding(const uint8_t* data, size_t size) {
  // The spec allows a padding OBU to be header-only (i.e., |size| = 0). So
  // check trailing bits only if |size| > 0.
  if (size == 0) return true;
  // The payload of a padding OBU is byte aligned. Therefore the first
  // trailing byte should be 0x80. See https://crbug.com/aomedia/2393.
  const int i = GetLastNonzeroByteIndex(data, size);
  if (i < 0) {
    LIBGAV1_DLOG(ERROR, "Trailing bit is missing.");
    return false;
  }
  if (data[i] != 0x80) {
    LIBGAV1_DLOG(
        ERROR,
        "The last nonzero byte of the payload data is 0x%x, should be 0x80.",
        data[i]);
    return false;
  }
  // Skip all bits before the trailing bit.
  bit_reader_->SkipBytes(i);
  return true;
}

bool ObuParser::ParseMetadataScalability() {
  int64_t scratch;
  // scalability_mode_idc
  OBU_READ_LITERAL_OR_FAIL(8);
  const auto scalability_mode_idc = static_cast<int>(scratch);
  if (scalability_mode_idc == kScalabilitySS) {
    // Parse scalability_structure().
    // spatial_layers_cnt_minus_1
    OBU_READ_LITERAL_OR_FAIL(2);
    const auto spatial_layers_count = static_cast<int>(scratch) + 1;
    // spatial_layer_dimensions_present_flag
    OBU_READ_BIT_OR_FAIL;
    const auto spatial_layer_dimensions_present_flag = scratch != 0;
    // spatial_layer_description_present_flag
    OBU_READ_BIT_OR_FAIL;
    const auto spatial_layer_description_present_flag = scratch != 0;
    // temporal_group_description_present_flag
    OBU_READ_BIT_OR_FAIL;
    const auto temporal_group_description_present_flag = scratch != 0;
    // scalability_structure_reserved_3bits
    OBU_READ_LITERAL_OR_FAIL(3);
    if (scratch != 0) {
      LIBGAV1_DLOG(WARNING,
                   "scalability_structure_reserved_3bits is not zero.");
    }
    if (spatial_layer_dimensions_present_flag) {
      for (int i = 0; i < spatial_layers_count; ++i) {
        // spatial_layer_max_width[i]
        OBU_READ_LITERAL_OR_FAIL(16);
        // spatial_layer_max_height[i]
        OBU_READ_LITERAL_OR_FAIL(16);
      }
    }
    if (spatial_layer_description_present_flag) {
      for (int i = 0; i < spatial_layers_count; ++i) {
        // spatial_layer_ref_id[i]
        OBU_READ_LITERAL_OR_FAIL(8);
      }
    }
    if (temporal_group_description_present_flag) {
      // temporal_group_size
      OBU_READ_LITERAL_OR_FAIL(8);
      const auto temporal_group_size = static_cast<int>(scratch);
      for (int i = 0; i < temporal_group_size; ++i) {
        // temporal_group_temporal_id[i]
        OBU_READ_LITERAL_OR_FAIL(3);
        // temporal_group_temporal_switching_up_point_flag[i]
        OBU_READ_BIT_OR_FAIL;
        // temporal_group_spatial_switching_up_point_flag[i]
        OBU_READ_BIT_OR_FAIL;
        // temporal_group_ref_cnt[i]
        OBU_READ_LITERAL_OR_FAIL(3);
        const auto temporal_group_ref_count = static_cast<int>(scratch);
        for (int j = 0; j < temporal_group_ref_count; ++j) {
          // temporal_group_ref_pic_diff[i][j]
          OBU_READ_LITERAL_OR_FAIL(8);
        }
      }
    }
  }
  return true;
}

bool ObuParser::ParseMetadataTimecode() {
  int64_t scratch;
  // counting_type: should be the same for all pictures in the coded video
  // sequence. 7..31 are reserved.
  OBU_READ_LITERAL_OR_FAIL(5);
  // full_timestamp_flag
  OBU_READ_BIT_OR_FAIL;
  const bool full_timestamp_flag = scratch != 0;
  // discontinuity_flag
  OBU_READ_BIT_OR_FAIL;
  // cnt_dropped_flag
  OBU_READ_BIT_OR_FAIL;
  // n_frames
  OBU_READ_LITERAL_OR_FAIL(9);
  if (full_timestamp_flag) {
    // seconds_value
    OBU_READ_LITERAL_OR_FAIL(6);
    const auto seconds_value = static_cast<int>(scratch);
    if (seconds_value > 59) {
      LIBGAV1_DLOG(ERROR, "Invalid seconds_value %d.", seconds_value);
      return false;
    }
    // minutes_value
    OBU_READ_LITERAL_OR_FAIL(6);
    const auto minutes_value = static_cast<int>(scratch);
    if (minutes_value > 59) {
      LIBGAV1_DLOG(ERROR, "Invalid minutes_value %d.", minutes_value);
      return false;
    }
    // hours_value
    OBU_READ_LITERAL_OR_FAIL(5);
    const auto hours_value = static_cast<int>(scratch);
    if (hours_value > 23) {
      LIBGAV1_DLOG(ERROR, "Invalid hours_value %d.", hours_value);
      return false;
    }
  } else {
    // seconds_flag
    OBU_READ_BIT_OR_FAIL;
    const bool seconds_flag = scratch != 0;
    if (seconds_flag) {
      // seconds_value
      OBU_READ_LITERAL_OR_FAIL(6);
      const auto seconds_value = static_cast<int>(scratch);
      if (seconds_value > 59) {
        LIBGAV1_DLOG(ERROR, "Invalid seconds_value %d.", seconds_value);
        return false;
      }
      // minutes_flag
      OBU_READ_BIT_OR_FAIL;
      const bool minutes_flag = scratch != 0;
      if (minutes_flag) {
        // minutes_value
        OBU_READ_LITERAL_OR_FAIL(6);
        const auto minutes_value = static_cast<int>(scratch);
        if (minutes_value > 59) {
          LIBGAV1_DLOG(ERROR, "Invalid minutes_value %d.", minutes_value);
          return false;
        }
        // hours_flag
        OBU_READ_BIT_OR_FAIL;
        const bool hours_flag = scratch != 0;
        if (hours_flag) {
          // hours_value
          OBU_READ_LITERAL_OR_FAIL(5);
          const auto hours_value = static_cast<int>(scratch);
          if (hours_value > 23) {
            LIBGAV1_DLOG(ERROR, "Invalid hours_value %d.", hours_value);
            return false;
          }
        }
      }
    }
  }
  // time_offset_length: should be the same for all pictures in the coded
  // video sequence.
  OBU_READ_LITERAL_OR_FAIL(5);
  const auto time_offset_length = static_cast<int>(scratch);
  if (time_offset_length > 0) {
    // time_offset_value
    OBU_READ_LITERAL_OR_FAIL(time_offset_length);
  }
  // Compute clockTimestamp. Section 6.7.7:
  //   When timing_info_present_flag is equal to 1 and discontinuity_flag is
  //   equal to 0, the value of clockTimestamp shall be greater than or equal
  //   to the value of clockTimestamp for the previous set of clock timestamp
  //   syntax elements in output order.
  return true;
}

bool ObuParser::ParseMetadata(const uint8_t* data, size_t size) {
  const size_t start_offset = bit_reader_->byte_offset();
  size_t metadata_type;
  if (!bit_reader_->ReadUnsignedLeb128(&metadata_type)) {
    LIBGAV1_DLOG(ERROR, "Could not read metadata_type.");
    return false;
  }
  const size_t metadata_type_size = bit_reader_->byte_offset() - start_offset;
  if (size < metadata_type_size) {
    LIBGAV1_DLOG(
        ERROR, "metadata_type is longer than metadata OBU payload %zu vs %zu.",
        metadata_type_size, size);
    return false;
  }
  data += metadata_type_size;
  size -= metadata_type_size;
  int64_t scratch;
  switch (metadata_type) {
    case kMetadataTypeHdrContentLightLevel: {
      ObuMetadataHdrCll hdr_cll;
      OBU_READ_LITERAL_OR_FAIL(16);
      hdr_cll.max_cll = scratch;
      OBU_READ_LITERAL_OR_FAIL(16);
      hdr_cll.max_fall = scratch;
      if (!EnsureCurrentFrameIsNotNull()) return false;
      current_frame_->set_hdr_cll(hdr_cll);
      break;
    }
    case kMetadataTypeHdrMasteringDisplayColorVolume: {
      ObuMetadataHdrMdcv hdr_mdcv;
      for (int i = 0; i < 3; ++i) {
        OBU_READ_LITERAL_OR_FAIL(16);
        hdr_mdcv.primary_chromaticity_x[i] = scratch;
        OBU_READ_LITERAL_OR_FAIL(16);
        hdr_mdcv.primary_chromaticity_y[i] = scratch;
      }
      OBU_READ_LITERAL_OR_FAIL(16);
      hdr_mdcv.white_point_chromaticity_x = scratch;
      OBU_READ_LITERAL_OR_FAIL(16);
      hdr_mdcv.white_point_chromaticity_y = scratch;
      OBU_READ_LITERAL_OR_FAIL(32);
      hdr_mdcv.luminance_max = static_cast<uint32_t>(scratch);
      OBU_READ_LITERAL_OR_FAIL(32);
      hdr_mdcv.luminance_min = static_cast<uint32_t>(scratch);
      if (!EnsureCurrentFrameIsNotNull()) return false;
      current_frame_->set_hdr_mdcv(hdr_mdcv);
      break;
    }
    case kMetadataTypeScalability:
      if (!ParseMetadataScalability()) return false;
      break;
    case kMetadataTypeItutT35: {
      ObuMetadataItutT35 itut_t35;
      OBU_READ_LITERAL_OR_FAIL(8);
      itut_t35.country_code = static_cast<uint8_t>(scratch);
      ++data;
      --size;
      if (itut_t35.country_code == 0xFF) {
        OBU_READ_LITERAL_OR_FAIL(8);
        itut_t35.country_code_extension_byte = static_cast<uint8_t>(scratch);
        ++data;
        --size;
      }
      // Read itut_t35.payload_bytes. Section 6.7.2 of the spec says:
      //   itut_t35.payload_bytes shall be bytes containing data registered as
      //   specified in Recommendation ITU-T T.35.
      // Therefore itut_t35.payload_bytes is byte aligned and the first trailing
      // byte should be 0x80. Since the exact syntax of itut_t35.payload_bytes
      // is not defined in the AV1 spec, identify the end of
      // itut_t35.payload_bytes by searching for the trailing bit.
      const int i = GetLastNonzeroByteIndex(data, size);
      if (i < 0) {
        LIBGAV1_DLOG(ERROR, "Trailing bit is missing.");
        return false;
      }
      if (data[i] != 0x80) {
        LIBGAV1_DLOG(
            ERROR,
            "itut_t35.payload_bytes is not byte aligned. The last nonzero byte "
            "of the payload data is 0x%x, should be 0x80.",
            data[i]);
        return false;
      }
      itut_t35.payload_size = i;
      if (!EnsureCurrentFrameIsNotNull() ||
          !current_frame_->set_itut_t35(itut_t35, data)) {
        return false;
      }
      // Skip all bits before the trailing bit.
      bit_reader_->SkipBytes(i);
      break;
    }
    case kMetadataTypeTimecode:
      if (!ParseMetadataTimecode()) return false;
      break;
    default: {
      // metadata_type is equal to a value reserved for future use or a user
      // private value.
      //
      // The Note in Section 5.8.1 says "Decoders should ignore the entire OBU
      // if they do not understand the metadata_type." Find the trailing bit
      // and skip all bits before the trailing bit.
      const int i = GetLastNonzeroByteIndex(data, size);
      if (i >= 0) {
        // The last 1 bit in the last nonzero byte is the trailing bit. Skip
        // all bits before the trailing bit.
        const int n = CountTrailingZeros(data[i]);
        bit_reader_->SkipBits(i * 8 + 7 - n);
      }
      break;
    }
  }
  return true;
}

bool ObuParser::AddTileBuffers(int start, int end, size_t total_size,
                               size_t tg_header_size,
                               size_t bytes_consumed_so_far) {
  // Validate that the tile group start and end are within the allowed range.
  if (start != next_tile_group_start_ || start > end ||
      end >= frame_header_.tile_info.tile_count) {
    LIBGAV1_DLOG(ERROR,
                 "Invalid tile group start %d or end %d: expected tile group "
                 "start %d, tile_count %d.",
                 start, end, next_tile_group_start_,
                 frame_header_.tile_info.tile_count);
    return false;
  }
  next_tile_group_start_ = end + 1;

  if (total_size < tg_header_size) {
    LIBGAV1_DLOG(ERROR, "total_size (%zu) is less than tg_header_size (%zu).)",
                 total_size, tg_header_size);
    return false;
  }
  size_t bytes_left = total_size - tg_header_size;
  const uint8_t* data = data_ + bytes_consumed_so_far + tg_header_size;
  for (int tile_number = start; tile_number <= end; ++tile_number) {
    size_t tile_size = 0;
    if (tile_number != end) {
      RawBitReader bit_reader(data, bytes_left);
      if (!bit_reader.ReadLittleEndian(frame_header_.tile_info.tile_size_bytes,
                                       &tile_size)) {
        LIBGAV1_DLOG(ERROR, "Could not read tile size for tile #%d",
                     tile_number);
        return false;
      }
      ++tile_size;
      data += frame_header_.tile_info.tile_size_bytes;
      bytes_left -= frame_header_.tile_info.tile_size_bytes;
      if (tile_size > bytes_left) {
        LIBGAV1_DLOG(ERROR, "Invalid tile size %zu for tile #%d", tile_size,
                     tile_number);
        return false;
      }
    } else {
      tile_size = bytes_left;
      if (tile_size == 0) {
        LIBGAV1_DLOG(ERROR, "Invalid tile size %zu for tile #%d", tile_size,
                     tile_number);
        return false;
      }
    }
    // The memory for this has been allocated in ParseTileInfoSyntax(). So it is
    // safe to use push_back_unchecked here.
    tile_buffers_.push_back_unchecked({data, tile_size});
    data += tile_size;
    bytes_left -= tile_size;
  }
  bit_reader_->SkipBytes(total_size - tg_header_size);
  return true;
}

bool ObuParser::ParseTileGroup(size_t size, size_t bytes_consumed_so_far) {
  const TileInfo* const tile_info = &frame_header_.tile_info;
  const size_t start_offset = bit_reader_->byte_offset();
  const int tile_bits =
      tile_info->tile_columns_log2 + tile_info->tile_rows_log2;
  if (tile_bits == 0) {
    return AddTileBuffers(0, 0, size, 0, bytes_consumed_so_far);
  }
  int64_t scratch;
  OBU_READ_BIT_OR_FAIL;
  const bool tile_start_and_end_present_flag = scratch != 0;
  if (!tile_start_and_end_present_flag) {
    if (!bit_reader_->AlignToNextByte()) {
      LIBGAV1_DLOG(ERROR, "Byte alignment has non zero bits.");
      return false;
    }
    return AddTileBuffers(0, tile_info->tile_count - 1, size, 1,
                          bytes_consumed_so_far);
  }
  if (obu_headers_.back().type == kObuFrame) {
    // 6.10.1: If obu_type is equal to OBU_FRAME, it is a requirement of
    // bitstream conformance that the value of tile_start_and_end_present_flag
    // is equal to 0.
    LIBGAV1_DLOG(ERROR,
                 "tile_start_and_end_present_flag must be 0 in Frame OBU");
    return false;
  }
  OBU_READ_LITERAL_OR_FAIL(tile_bits);
  const int start = static_cast<int>(scratch);
  OBU_READ_LITERAL_OR_FAIL(tile_bits);
  const int end = static_cast<int>(scratch);
  if (!bit_reader_->AlignToNextByte()) {
    LIBGAV1_DLOG(ERROR, "Byte alignment has non zero bits.");
    return false;
  }
  const size_t tg_header_size = bit_reader_->byte_offset() - start_offset;
  return AddTileBuffers(start, end, size, tg_header_size,
                        bytes_consumed_so_far);
}

bool ObuParser::ParseHeader() {
  ObuHeader obu_header;
  int64_t scratch = bit_reader_->ReadBit();
  if (scratch != 0) {
    LIBGAV1_DLOG(ERROR, "forbidden_bit is not zero.");
    return false;
  }
  OBU_READ_LITERAL_OR_FAIL(4);
  obu_header.type = static_cast<libgav1::ObuType>(scratch);
  OBU_READ_BIT_OR_FAIL;
  const bool extension_flag = scratch != 0;
  OBU_READ_BIT_OR_FAIL;
  obu_header.has_size_field = scratch != 0;
  OBU_READ_BIT_OR_FAIL;  // reserved.
  if (scratch != 0) {
    LIBGAV1_DLOG(WARNING, "obu_reserved_1bit is not zero.");
  }
  obu_header.has_extension = extension_flag;
  if (extension_flag) {
    if (extension_disallowed_) {
      LIBGAV1_DLOG(ERROR,
                   "OperatingPointIdc is 0, but obu_extension_flag is 1.");
      return false;
    }
    OBU_READ_LITERAL_OR_FAIL(3);
    obu_header.temporal_id = scratch;
    OBU_READ_LITERAL_OR_FAIL(2);
    obu_header.spatial_id = scratch;
    OBU_READ_LITERAL_OR_FAIL(3);  // reserved.
    if (scratch != 0) {
      LIBGAV1_DLOG(WARNING, "extension_header_reserved_3bits is not zero.");
    }
  } else {
    obu_header.temporal_id = 0;
    obu_header.spatial_id = 0;
  }
  return obu_headers_.push_back(obu_header);
}

#undef OBU_READ_UVLC_OR_FAIL
#undef OBU_READ_LITERAL_OR_FAIL
#undef OBU_READ_BIT_OR_FAIL
#undef OBU_PARSER_FAIL
#undef OBU_LOG_AND_RETURN_FALSE

bool ObuParser::InitBitReader(const uint8_t* const data, size_t size) {
  bit_reader_.reset(new (std::nothrow) RawBitReader(data, size));
  return bit_reader_ != nullptr;
}

bool ObuParser::EnsureCurrentFrameIsNotNull() {
  if (current_frame_ != nullptr) return true;
  current_frame_ = buffer_pool_->GetFreeBuffer();
  if (current_frame_ == nullptr) {
    LIBGAV1_DLOG(ERROR, "Could not get current_frame from the buffer pool.");
    return false;
  }
  return true;
}

bool ObuParser::HasData() const { return size_ > 0; }

StatusCode ObuParser::ParseOneFrame(RefCountedBufferPtr* const current_frame) {
  if (data_ == nullptr || size_ == 0) return kStatusInvalidArgument;

  assert(current_frame_ == nullptr);
  // This is used to release any references held in case of parsing failure.
  RefCountedBufferPtrCleanup current_frame_cleanup(&current_frame_);

  const uint8_t* data = data_;
  size_t size = size_;

  // Clear everything except the sequence header.
  obu_headers_.clear();
  frame_header_ = {};
  tile_buffers_.clear();
  next_tile_group_start_ = 0;
  sequence_header_changed_ = false;

  bool parsed_one_full_frame = false;
  bool seen_frame_header = false;
  const uint8_t* frame_header = nullptr;
  size_t frame_header_size_in_bits = 0;
  while (size > 0 && !parsed_one_full_frame) {
    if (!InitBitReader(data, size)) {
      LIBGAV1_DLOG(ERROR, "Failed to initialize bit reader.");
      return kStatusOutOfMemory;
    }
    if (!ParseHeader()) {
      LIBGAV1_DLOG(ERROR, "Failed to parse OBU Header.");
      return kStatusBitstreamError;
    }
    const ObuHeader& obu_header = obu_headers_.back();
    if (!obu_header.has_size_field) {
      LIBGAV1_DLOG(
          ERROR,
          "has_size_field is zero. libgav1 does not support such streams.");
      return kStatusUnimplemented;
    }
    const size_t obu_header_size = bit_reader_->byte_offset();
    size_t obu_size;
    if (!bit_reader_->ReadUnsignedLeb128(&obu_size)) {
      LIBGAV1_DLOG(ERROR, "Could not read OBU size.");
      return kStatusBitstreamError;
    }
    const size_t obu_length_size = bit_reader_->byte_offset() - obu_header_size;
    if (size - bit_reader_->byte_offset() < obu_size) {
      LIBGAV1_DLOG(ERROR, "Not enough bits left to parse OBU %zu vs %zu.",
                   size - bit_reader_->bit_offset(), obu_size);
      return kStatusBitstreamError;
    }

    const ObuType obu_type = obu_header.type;
    if (obu_type != kObuSequenceHeader && obu_type != kObuTemporalDelimiter &&
        has_sequence_header_ &&
        sequence_header_.operating_point_idc[operating_point_] != 0 &&
        obu_header.has_extension &&
        (!InTemporalLayer(
             sequence_header_.operating_point_idc[operating_point_],
             obu_header.temporal_id) ||
         !InSpatialLayer(sequence_header_.operating_point_idc[operating_point_],
                         obu_header.spatial_id))) {
      obu_headers_.pop_back();
      bit_reader_->SkipBytes(obu_size);
      data += bit_reader_->byte_offset();
      size -= bit_reader_->byte_offset();
      continue;
    }

    const size_t obu_start_position = bit_reader_->bit_offset();
    // The bit_reader_ is byte aligned after reading obu_header and obu_size.
    // Therefore the byte offset can be computed as obu_start_position >> 3
    // below.
    assert((obu_start_position & 7) == 0);
    bool obu_skipped = false;
    switch (obu_type) {
      case kObuTemporalDelimiter:
        break;
      case kObuSequenceHeader:
        if (!ParseSequenceHeader(seen_frame_header)) {
          LIBGAV1_DLOG(ERROR, "Failed to parse SequenceHeader OBU.");
          return kStatusBitstreamError;
        }
        if (sequence_header_.color_config.bitdepth > LIBGAV1_MAX_BITDEPTH) {
          LIBGAV1_DLOG(
              ERROR,
              "Bitdepth %d is not supported. The maximum bitdepth is %d.",
              sequence_header_.color_config.bitdepth, LIBGAV1_MAX_BITDEPTH);
          return kStatusUnimplemented;
        }
        break;
      case kObuFrameHeader:
        if (seen_frame_header) {
          LIBGAV1_DLOG(ERROR,
                       "Frame header found but frame header was already seen.");
          return kStatusBitstreamError;
        }
        if (!ParseFrameHeader()) {
          LIBGAV1_DLOG(ERROR, "Failed to parse FrameHeader OBU.");
          return kStatusBitstreamError;
        }
        frame_header = &data[obu_start_position >> 3];
        frame_header_size_in_bits =
            bit_reader_->bit_offset() - obu_start_position;
        seen_frame_header = true;
        parsed_one_full_frame = frame_header_.show_existing_frame;
        break;
      case kObuRedundantFrameHeader: {
        if (!seen_frame_header) {
          LIBGAV1_DLOG(ERROR,
                       "Redundant frame header found but frame header was not "
                       "yet seen.");
          return kStatusBitstreamError;
        }
        const size_t fh_size = (frame_header_size_in_bits + 7) >> 3;
        if (obu_size < fh_size ||
            memcmp(frame_header, &data[obu_start_position >> 3], fh_size) !=
                0) {
          LIBGAV1_DLOG(ERROR,
                       "Redundant frame header differs from frame header.");
          return kStatusBitstreamError;
        }
        bit_reader_->SkipBits(frame_header_size_in_bits);
        break;
      }
      case kObuFrame: {
        const size_t fh_start_offset = bit_reader_->byte_offset();
        if (seen_frame_header) {
          LIBGAV1_DLOG(ERROR,
                       "Frame header found but frame header was already seen.");
          return kStatusBitstreamError;
        }
        if (!ParseFrameHeader()) {
          LIBGAV1_DLOG(ERROR, "Failed to parse FrameHeader in Frame OBU.");
          return kStatusBitstreamError;
        }
        // Section 6.8.2: If obu_type is equal to OBU_FRAME, it is a
        // requirement of bitstream conformance that show_existing_frame is
        // equal to 0.
        if (frame_header_.show_existing_frame) {
          LIBGAV1_DLOG(ERROR, "Frame OBU cannot set show_existing_frame to 1.");
          return kStatusBitstreamError;
        }
        if (!bit_reader_->AlignToNextByte()) {
          LIBGAV1_DLOG(ERROR, "Byte alignment has non zero bits.");
          return kStatusBitstreamError;
        }
        const size_t fh_size = bit_reader_->byte_offset() - fh_start_offset;
        if (fh_size >= obu_size) {
          LIBGAV1_DLOG(ERROR, "Frame header size (%zu) >= obu_size (%zu).",
                       fh_size, obu_size);
          return kStatusBitstreamError;
        }
        if (!ParseTileGroup(obu_size - fh_size,
                            size_ - size + bit_reader_->byte_offset())) {
          LIBGAV1_DLOG(ERROR, "Failed to parse TileGroup in Frame OBU.");
          return kStatusBitstreamError;
        }
        parsed_one_full_frame = true;
        break;
      }
      case kObuTileGroup:
        if (!ParseTileGroup(obu_size,
                            size_ - size + bit_reader_->byte_offset())) {
          LIBGAV1_DLOG(ERROR, "Failed to parse TileGroup OBU.");
          return kStatusBitstreamError;
        }
        parsed_one_full_frame =
            (next_tile_group_start_ == frame_header_.tile_info.tile_count);
        break;
      case kObuTileList:
        LIBGAV1_DLOG(ERROR, "Decoding of tile list OBUs is not supported.");
        return kStatusUnimplemented;
      case kObuPadding:
        if (!ParsePadding(&data[obu_start_position >> 3], obu_size)) {
          LIBGAV1_DLOG(ERROR, "Failed to parse Padding OBU.");
          return kStatusBitstreamError;
        }
        break;
      case kObuMetadata:
        if (!ParseMetadata(&data[obu_start_position >> 3], obu_size)) {
          LIBGAV1_DLOG(ERROR, "Failed to parse Metadata OBU.");
          return kStatusBitstreamError;
        }
        break;
      default:
        // Skip reserved OBUs. Section 6.2.2: Reserved units are for future use
        // and shall be ignored by AV1 decoder.
        bit_reader_->SkipBytes(obu_size);
        obu_skipped = true;
        break;
    }
    if (obu_size > 0 && !obu_skipped && obu_type != kObuFrame &&
        obu_type != kObuTileGroup) {
      const size_t parsed_obu_size_in_bits =
          bit_reader_->bit_offset() - obu_start_position;
      if (obu_size * 8 < parsed_obu_size_in_bits) {
        LIBGAV1_DLOG(
            ERROR,
            "Parsed OBU size (%zu bits) is greater than expected OBU size "
            "(%zu bytes) obu_type: %d.",
            parsed_obu_size_in_bits, obu_size, obu_type);
        return kStatusBitstreamError;
      }
      if (!bit_reader_->VerifyAndSkipTrailingBits(obu_size * 8 -
                                                  parsed_obu_size_in_bits)) {
        LIBGAV1_DLOG(ERROR,
                     "Error when verifying trailing bits for obu type: %d",
                     obu_type);
        return kStatusBitstreamError;
      }
    }
    const size_t bytes_consumed = bit_reader_->byte_offset();
    const size_t consumed_obu_size =
        bytes_consumed - obu_length_size - obu_header_size;
    if (consumed_obu_size != obu_size) {
      LIBGAV1_DLOG(ERROR,
                   "OBU size (%zu) and consumed size (%zu) does not match for "
                   "obu_type: %d.",
                   obu_size, consumed_obu_size, obu_type);
      return kStatusBitstreamError;
    }
    data += bytes_consumed;
    size -= bytes_consumed;
  }
  if (!parsed_one_full_frame && seen_frame_header) {
    LIBGAV1_DLOG(ERROR, "The last tile group in the frame was not received.");
    return kStatusBitstreamError;
  }
  data_ = data;
  size_ = size;
  *current_frame = std::move(current_frame_);
  return kStatusOk;
}

// AV1CodecConfigurationBox specification:
// https://aomediacodec.github.io/av1-isobmff/#av1codecconfigurationbox.
// static
std::unique_ptr<uint8_t[]> ObuParser::GetAV1CodecConfigurationBox(
    const uint8_t* data, size_t size, size_t* const av1c_size) {
  if (data == nullptr || av1c_size == nullptr) return nullptr;

  ObuSequenceHeader sequence_header;
  size_t sequence_header_offset;
  size_t sequence_header_size;
  const StatusCode status =
      ParseBasicStreamInfo(data, size, &sequence_header,
                           &sequence_header_offset, &sequence_header_size);
  if (status != kStatusOk) {
    *av1c_size = 0;
    return nullptr;
  }

  *av1c_size = 4 + sequence_header_size;
  std::unique_ptr<uint8_t[]> av1c_ptr(new (std::nothrow) uint8_t[*av1c_size]);
  if (av1c_ptr == nullptr) {
    *av1c_size = 0;
    return nullptr;
  }
  uint8_t* av1c = av1c_ptr.get();
  // unsigned int (1) marker = 1;
  // unsigned int (7) version = 1;
  av1c[0] = 0x81;

  // unsigned int (3) seq_profile;
  // unsigned int (5) seq_level_idx_0;
  const uint8_t seq_level_idx_0 = ((sequence_header.level[0].major - 2) << 2) |
                                  sequence_header.level[0].minor;
  av1c[1] = (sequence_header.profile << 5) | seq_level_idx_0;

  // unsigned int (1) seq_tier_0;
  // unsigned int (1) high_bitdepth;
  // unsigned int (1) twelve_bit;
  // unsigned int (1) monochrome;
  // unsigned int (1) chroma_subsampling_x;
  // unsigned int (1) chroma_subsampling_y;
  // unsigned int (2) chroma_sample_position;
  const auto high_bitdepth =
      static_cast<uint8_t>(sequence_header.color_config.bitdepth > 8);
  const auto twelve_bit =
      static_cast<uint8_t>(sequence_header.color_config.bitdepth == 12);
  av1c[2] =
      (sequence_header.tier[0] << 7) | (high_bitdepth << 6) |
      (twelve_bit << 5) |
      (static_cast<uint8_t>(sequence_header.color_config.is_monochrome) << 4) |
      (sequence_header.color_config.subsampling_x << 3) |
      (sequence_header.color_config.subsampling_y << 2) |
      sequence_header.color_config.chroma_sample_position;

  // unsigned int (3) reserved = 0;
  // unsigned int (1) initial_presentation_delay_present;
  // if (initial_presentation_delay_present) {
  //   unsigned int (4) initial_presentation_delay_minus_one;
  // } else {
  //   unsigned int (4) reserved = 0;
  // }
  av1c[3] = 0;

  // unsigned int (8) configOBUs[];
  memcpy(av1c + 4, data + sequence_header_offset, sequence_header_size);

  return av1c_ptr;
}

// static
StatusCode ObuParser::ParseBasicStreamInfo(const uint8_t* data, size_t size,
                                           ObuSequenceHeader* sequence_header,
                                           size_t* sequence_header_offset,
                                           size_t* sequence_header_size) {
  DecoderState state;
  ObuParser parser(nullptr, 0, 0, nullptr, &state);
  if (!parser.InitBitReader(data, size)) {
    LIBGAV1_DLOG(ERROR, "Failed to initialize bit reader.");
    return kStatusOutOfMemory;
  }
  while (!parser.bit_reader_->Finished()) {
    const size_t obu_start_offset = parser.bit_reader_->byte_offset();
    if (!parser.ParseHeader()) {
      LIBGAV1_DLOG(ERROR, "Failed to parse OBU Header.");
      return kStatusBitstreamError;
    }
    const ObuHeader& obu_header = parser.obu_headers_.back();
    if (!obu_header.has_size_field) {
      LIBGAV1_DLOG(
          ERROR,
          "has_size_field is zero. libgav1 does not support such streams.");
      return kStatusUnimplemented;
    }
    size_t obu_size;
    if (!parser.bit_reader_->ReadUnsignedLeb128(&obu_size)) {
      LIBGAV1_DLOG(ERROR, "Could not read OBU size.");
      return kStatusBitstreamError;
    }
    if (size - parser.bit_reader_->byte_offset() < obu_size) {
      LIBGAV1_DLOG(ERROR, "Not enough bits left to parse OBU %zu vs %zu.",
                   size - parser.bit_reader_->bit_offset(), obu_size);
      return kStatusBitstreamError;
    }
    if (obu_header.type != kObuSequenceHeader) {
      parser.obu_headers_.pop_back();
      parser.bit_reader_->SkipBytes(obu_size);
      continue;
    }
    const size_t obu_start_position = parser.bit_reader_->bit_offset();
    if (!parser.ParseSequenceHeader(false)) {
      LIBGAV1_DLOG(ERROR, "Failed to parse SequenceHeader OBU.");
      return kStatusBitstreamError;
    }
    const size_t parsed_obu_size_in_bits =
        parser.bit_reader_->bit_offset() - obu_start_position;
    const uint64_t obu_size_in_bits = static_cast<uint64_t>(obu_size) * 8;
    if (obu_size_in_bits < parsed_obu_size_in_bits) {
      LIBGAV1_DLOG(
          ERROR,
          "Parsed OBU size (%zu bits) is greater than expected OBU size "
          "(%zu bytes)..",
          parsed_obu_size_in_bits, obu_size);
      return kStatusBitstreamError;
    }
    if (!parser.bit_reader_->VerifyAndSkipTrailingBits(
            static_cast<size_t>(obu_size_in_bits - parsed_obu_size_in_bits))) {
      LIBGAV1_DLOG(
          ERROR, "Error when verifying trailing bits for the sequence header.");
      return kStatusBitstreamError;
    }
    *sequence_header = parser.sequence_header_;
    *sequence_header_offset = obu_start_offset;
    *sequence_header_size =
        parser.bit_reader_->byte_offset() - obu_start_offset;
    return kStatusOk;
  }
  // Sequence header was never found.
  return kStatusBitstreamError;
}

}  // namespace libgav1
