// Copyright 2021 The libgav1 Authors
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "src/buffer_pool.h"
#include "src/decoder_impl.h"
#include "src/decoder_state.h"
#include "src/gav1/decoder_buffer.h"
#include "src/gav1/status_code.h"
#include "src/utils/common.h"
#include "src/utils/constants.h"
#include "src/utils/dynamic_buffer.h"
#include "src/utils/segmentation.h"
#include "src/utils/types.h"
#include "src/utils/vector.h"
#include "tests/third_party/libvpx/acm_random.h"

// Note the following test classes access private functions/members of
// ObuParser. To be declared friends of ObuParser they must not have internal
// linkage (they must be outside the anonymous namespace).
namespace libgav1 {

// Helper class to manipulate individual bits and generate a byte string.
class BytesAndBits {
 public:
  // Append a bit to the end.
  void AppendBit(uint8_t bit) { bits_.push_back(bit != 0); }

  // Append a byte to the end.
  void AppendByte(uint8_t byte) {
    for (int i = 0; i < 8; ++i) {
      AppendBit(GetNthBit(byte, i, 8));
    }
  }

  // Append a literal of size |bits| to the end.
  void AppendLiteral(int bits, int value) {
    InsertLiteral(static_cast<int>(bits_.size()), bits, value);
  }

  // Append an inverse signed literal to the end. |bits + 1| bits are appended.
  void AppendInverseSignedLiteral(int bits, int value) {
    InsertInverseSignedLiteral(static_cast<int>(bits_.size()), bits, value);
  }

  // Append a sequence of bytes to the end.
  void AppendBytes(const std::vector<uint8_t>& bytes) {
    for (const auto& byte : bytes) {
      AppendByte(byte);
    }
  }

  // Insert |bit| in |offset|. Moves all other bits to the right by 1.
  void InsertBit(int offset, uint8_t bit) {
    auto iterator = bits_.begin();
    bits_.insert(iterator + offset, bit != 0);
  }

  // Insert |value| of size |bits| at offset |offset|. Moves all other bits to
  // the right by |bits|.
  void InsertLiteral(int offset, int bits, int value) {
    for (int i = 0; i < bits; ++i) {
      InsertBit(i + offset, GetNthBit(value, i, bits));
    }
  }

  // Insert |value| of size |bits| at offset |offset| as an inverse signed
  // literal. Move all other bits to the right by |bits + 1|.
  //
  // Note: This is denoted su(1+bits) in the spec.
  void InsertInverseSignedLiteral(int offset, int bits, int value) {
    InsertBit(offset, (value >= 0) ? 0 : 1);
    InsertLiteral(offset + 1, bits, value);
  }

  // Insert |value| at |offset| as an unsigned variable length code (uvlc).
  // Return the number of bits inserted.
  int InsertUvlc(int offset, int value) {
    int leading_zeros = 1;
    int shift_value = ++value;
    while ((shift_value >>= 1) != 0) leading_zeros += 2;
    int bits = 0;
    InsertLiteral(offset, leading_zeros >> 1, 0);
    bits += leading_zeros >> 1;
    InsertLiteral(offset + bits, (leading_zeros + 1) >> 1, value);
    bits += (leading_zeros + 1) >> 1;
    return bits;
  }

  // Set the bit at |offset| to |bit|. The bit should already exist.
  void SetBit(int offset, uint8_t bit) { bits_[offset] = bit != 0; }

  // Set |bits| starting at |offset| to |value|. The bits should already exist.
  void SetLiteral(int offset, int bits, int value) {
    for (int i = 0; i < bits; ++i) {
      SetBit(offset + i, GetNthBit(value, i, bits));
    }
  }

  // Remove a bit in |offset|. Moves over all the following bits to the left by
  // 1.
  void RemoveBit(int offset) { RemoveLiteral(offset, 1); }

  // Remove a literal of size |bits| from |offset|. Moves over all the
  // following bits to the left by |bits|.
  void RemoveLiteral(int offset, int bits) {
    bits_.erase(bits_.begin() + offset, bits_.begin() + offset + bits);
  }

  // Remove all bits after offset.
  void RemoveAllBitsAfter(int offset) {
    RemoveLiteral(offset, static_cast<int>(bits_.size()) - offset);
  }

  // Clear all the bits stored.
  void Clear() { bits_.clear(); }

  // Generate the data vector from the bits. Pads 0 to the end of the last byte
  // if necessary.
  const std::vector<uint8_t>& GenerateData() {
    data_.clear();
    for (size_t i = 0; i < bits_.size(); i += 8) {
      uint8_t byte = 0;
      for (int j = 0; j < 8; ++j) {
        const uint8_t bit =
            ((i + j) < bits_.size()) ? static_cast<uint8_t>(bits_[i + j]) : 0;
        byte |= bit << (7 - j);
      }
      data_.push_back(byte);
    }
    return data_;
  }

 private:
  // Get the |n|th MSB from |value| with the assumption that |value| has |size|
  // bits.
  static uint8_t GetNthBit(int value, int n, int size) {
    return (value >> (size - n - 1)) & 0x01;
  }

  std::vector<uint8_t> data_;
  std::vector<bool> bits_;
};

class ObuParserTest : public testing::Test {
 protected:
  // Constants for unit tests.
  static constexpr int kFrameWidthBits = 9;
  static constexpr int kFrameHeightBits = 8;
  static constexpr int kHeight = 240;
  static constexpr int kWidth = 426;
  static constexpr int kRows4x4 = 60;
  static constexpr int kColumns4x4 = 108;
  static constexpr int kFrameToShow = 2;
  static constexpr int kDisplayFrameId = 10;
  static constexpr int kFrameIdLengthBits = 15;
  static constexpr int kDeltaFrameIdLengthBits = 14;

  // Bit streams for testing. These may contain trailing bits and tests may have
  // to remove some of the trailing bits to keep the boundary alignment.
  const std::vector<uint8_t> kDefaultTemporalDelimiter = {0x12, 0x00};
  // Bits  Syntax element                  Value
  // 1     obu_forbidden_bit               0
  // 4     obu_type                        2 (OBU_TEMPORAL_DELIMITER)
  // 1     obu_extension_flag              1
  // 1     obu_has_size_field              1
  // 1     obu_reserved_1bit               0
  // 3     temporal_id                     6
  // 2     spatial_id                      2
  // 3     extension_header_reserved_3bits 0
  // 8     obu_size                        0
  const std::vector<uint8_t> kDefaultTemporalDelimiterWithExtension = {
      0x16, 0xd0, 0x00};
  const std::vector<uint8_t> kDefaultHeaderWithoutSizeField = {0x10};
  // Offset  Bits  Syntax element                     Value
  // 0       3     seq_profile                        0
  // 3       1     still_picture                      0
  // 4       1     reduced_still_picture_header       0
  // 5       1     timing_info_present_flag           0
  // 6       1     initial_display_delay_present_flag 0
  // 7       5     operating_points_cnt_minus_1       0
  // 12      12    operating_point_idc[ 0 ]           0
  // 24      5     seq_level_idx[ 0 ]                 0
  // 29      4     frame_width_bits_minus_1           8
  // 33      4     frame_height_bits_minus_1          7
  // 37      9     max_frame_width_minus_1            425
  // 46      8     max_frame_height_minus_1           239
  // 54      1     frame_id_numbers_present_flag      0
  // 55      1     use_128x128_superblock             1
  // 56      1     enable_filter_intra                1
  // 57      1     enable_intra_edge_filter           1
  // 58      1     enable_interintra_compound         1
  // 59      1     enable_masked_compound             1
  // 60      1     enable_warped_motion               0
  // 61      1     enable_dual_filter                 1
  // 62      1     enable_order_hint                  1
  // 63      1     enable_jnt_comp                    1
  // 64      1     enable_ref_frame_mvs               1
  // 65      1     seq_choose_screen_content_tools    1
  // 66      1     seq_choose_integer_mv              1
  // 67      3     order_hint_bits_minus_1            6
  // 70      1     enable_superres                    0
  // 71      1     enable_cdef                        1
  // 72      1     enable_restoration                 1
  // ...
  const std::vector<uint8_t> kDefaultSequenceHeader = {
      0x00, 0x00, 0x00, 0x04, 0x3e, 0xa7, 0xbd, 0xf7, 0xf9, 0x80, 0x40};
  const std::vector<uint8_t> kDefaultFrameHeaderKeyFrame = {0x10, 0x00};
  // Bits  Syntax element           Value
  // 1     show_existing_frame      0
  // 2     frame_type               2 (kFrameIntraOnly)
  // 1     show_frame               1
  // 1     error_resilient_mode     0
  // 1     disable_cdf_update       0
  // 1     frame_size_override_flag 0
  // 8     refresh_frame_flags      4
  // ...
  const std::vector<uint8_t> kDefaultFrameHeaderIntraOnlyFrame = {0x50, 0x08,
                                                                  0x00};
  // Bits  Syntax element           Value
  // 1     show_existing_frame      0
  // 2     frame_type               1 (kFrameInter)
  // 1     show_frame               1
  // 1     error_resilient_mode     0
  // 1     disable_cdf_update       0
  // 1     frame_size_override_flag 0
  // 3     primary_ref_frame        1
  // 8     refresh_frame_flags      4
  // 3     ref_frame_idx[0]         0
  // 3     ref_frame_idx[1]         1
  // 3     ref_frame_idx[2]         2
  // 3     ref_frame_idx[3]         3
  // 3     ref_frame_idx[4]         4
  // 3     ref_frame_idx[5]         5
  // 3     ref_frame_idx[6]         6
  // ...
  const std::vector<uint8_t> kDefaultFrameHeaderInterFrame = {0x30, 0x41, 0x01,
                                                              0x4e, 0x5c, 0x60};
  const std::vector<uint8_t> kDefaultGlobalMotionParametersRotZoom = {
      0xff, 0x50, 0x77, 0x7e, 0x1f, 0xcd};
  const std::vector<uint8_t> kDefaultGlobalMotionParametersAffine = {
      0x3f, 0x50, 0x77, 0x7b, 0xbf, 0xa8, 0x3e, 0x1f, 0xcd};

  void SetUp() override {
    buffer_pool_.reset(new (std::nothrow)
                           BufferPool(nullptr, nullptr, nullptr, nullptr));
    ASSERT_NE(buffer_pool_, nullptr);
  }

  bool Init() {
    obu_.reset(new (std::nothrow) ObuParser(nullptr, 0, 0, buffer_pool_.get(),
                                            &decoder_state_));
    if (obu_ == nullptr) return false;
    obu_headers_ = &obu_->obu_headers_;
    obu_frame_header_ = &obu_->frame_header_;
    obu_sequence_header_ = &obu_->sequence_header_;
    return true;
  }

  bool Init(const std::vector<uint8_t>& data, bool init_bit_reader = true) {
    obu_.reset(new (std::nothrow) ObuParser(
        data.data(), data.size(), 0, buffer_pool_.get(), &decoder_state_));
    if (obu_ == nullptr) return false;
    obu_headers_ = &obu_->obu_headers_;
    obu_frame_header_ = &obu_->frame_header_;
    obu_sequence_header_ = &obu_->sequence_header_;
    return init_bit_reader ? obu_->InitBitReader(data.data(), data.size())
                           : true;
  }

  bool Parse(const std::string& input,
             const ObuSequenceHeader* const sequence_header = nullptr) {
    std::vector<uint8_t> data(input.begin(), input.end());
    return Parse(data, sequence_header);
  }

  bool Parse(const std::vector<uint8_t>& data,
             const ObuSequenceHeader* const sequence_header = nullptr) {
    EXPECT_TRUE(Init(data, false));
    if (sequence_header != nullptr) obu_->set_sequence_header(*sequence_header);
    return obu_->ParseOneFrame(&current_frame_) == kStatusOk;
  }

  bool ParseSequenceHeader(const std::vector<uint8_t>& data) {
    EXPECT_TRUE(Init(data));
    return obu_->ParseSequenceHeader(/*seen_frame_header=*/false);
  }

  bool ParseFrameParameters(const std::vector<uint8_t>& data,
                            bool id_bits_present = false,
                            int force_screen_content_tools = 0,
                            int force_integer_mv = 0,
                            bool enable_superres = false) {
    EXPECT_TRUE(Init(data));
    if (id_bits_present) {
      obu_->sequence_header_.frame_id_numbers_present = true;
      obu_->sequence_header_.frame_id_length_bits = kFrameIdLengthBits;
      obu_->sequence_header_.delta_frame_id_length_bits =
          kDeltaFrameIdLengthBits;
    }
    obu_->sequence_header_.force_screen_content_tools =
        force_screen_content_tools;
    obu_->sequence_header_.force_integer_mv = force_integer_mv;
    obu_->sequence_header_.enable_superres = enable_superres;
    obu_->sequence_header_.frame_width_bits = kFrameWidthBits;
    obu_->sequence_header_.frame_height_bits = kFrameHeightBits;
    obu_->sequence_header_.max_frame_width = kWidth;
    obu_->sequence_header_.max_frame_height = kHeight;
    return obu_->ParseFrameParameters();
  }

  bool ParseSegmentationParameters(const std::vector<uint8_t>& data,
                                   int primary_reference_frame,
                                   int prev_frame_index) {
    EXPECT_TRUE(Init(data));
    obu_->frame_header_.primary_reference_frame = primary_reference_frame;
    if (primary_reference_frame != kPrimaryReferenceNone) {
      obu_->frame_header_.reference_frame_index[primary_reference_frame] =
          prev_frame_index;
    }
    return obu_->ParseSegmentationParameters();
  }

  bool ParseFrameReferenceModeSyntax(const std::vector<uint8_t>& data,
                                     FrameType frame_type) {
    EXPECT_TRUE(Init(data));
    obu_->frame_header_.frame_type = frame_type;
    return obu_->ParseFrameReferenceModeSyntax();
  }

  bool ParseGlobalMotionParameters(const std::vector<uint8_t>& data,
                                   FrameType frame_type) {
    EXPECT_TRUE(Init(data));
    obu_->frame_header_.frame_type = frame_type;
    obu_->frame_header_.primary_reference_frame = kPrimaryReferenceNone;
    return obu_->ParseGlobalMotionParameters();
  }

  bool ParseFilmGrainParameters(const std::vector<uint8_t>& data,
                                const ObuSequenceHeader& sequence_header,
                                const ObuFrameHeader& frame_header) {
    EXPECT_TRUE(Init(data));
    obu_->set_sequence_header(sequence_header);
    obu_->frame_header_ = frame_header;
    return obu_->ParseFilmGrainParameters();
  }

  bool ParseTileInfoSyntax(const std::vector<uint8_t>& data, int columns4x4,
                           int rows4x4, bool use_128x128_superblock) {
    EXPECT_TRUE(Init(data));
    obu_->frame_header_.columns4x4 = columns4x4;
    obu_->frame_header_.rows4x4 = rows4x4;
    obu_->sequence_header_.use_128x128_superblock = use_128x128_superblock;
    return obu_->ParseTileInfoSyntax();
  }

  bool ParseMetadata(const std::vector<uint8_t>& data) {
    EXPECT_TRUE(Init(data));
    return obu_->ParseMetadata(data.data(), data.size());
  }

  void DefaultSequenceHeader(ObuSequenceHeader* const gold) {
    memset(gold, 0, sizeof(*gold));
    gold->profile = kProfile0;
    gold->level[0].major = kMinimumMajorBitstreamLevel;
    gold->operating_points = 1;
    gold->max_frame_width = kWidth;
    gold->max_frame_height = kHeight;
    gold->frame_width_bits = kFrameWidthBits;
    gold->frame_height_bits = kFrameHeightBits;
    gold->use_128x128_superblock = true;
    gold->enable_filter_intra = true;
    gold->enable_intra_edge_filter = true;
    gold->enable_interintra_compound = true;
    gold->enable_masked_compound = true;
    gold->enable_dual_filter = true;
    gold->enable_order_hint = true;
    gold->enable_jnt_comp = true;
    gold->enable_ref_frame_mvs = true;
    gold->choose_screen_content_tools = true;
    gold->force_screen_content_tools = 2;
    gold->choose_integer_mv = true;
    gold->force_integer_mv = 2;
    gold->order_hint_bits = 7;
    gold->enable_cdef = true;
    gold->enable_restoration = true;
    gold->color_config.bitdepth = 8;
    gold->color_config.color_primary = kColorPrimaryUnspecified;
    gold->color_config.transfer_characteristics =
        kTransferCharacteristicsUnspecified;
    gold->color_config.matrix_coefficients = kMatrixCoefficientsUnspecified;
    gold->color_config.subsampling_x = 1;
    gold->color_config.subsampling_y = 1;
  }

  void DefaultFrameHeader(ObuFrameHeader* const gold, FrameType frame_type) {
    memset(gold, 0, sizeof(*gold));
    gold->frame_type = frame_type;
    gold->show_frame = true;
    gold->showable_frame = (frame_type != kFrameKey);
    gold->enable_cdf_update = true;
    gold->width = kWidth;
    gold->height = kHeight;
    gold->render_width = kWidth;
    gold->render_height = kHeight;
    gold->upscaled_width = kWidth;
    gold->primary_reference_frame = kPrimaryReferenceNone;
    gold->enable_frame_end_update_cdf = true;
    gold->rows4x4 = kRows4x4;
    gold->columns4x4 = kColumns4x4;
    if (frame_type == kFrameKey) {
      gold->refresh_frame_flags = 0xff;
      gold->error_resilient_mode = true;
      gold->force_integer_mv = 1;
    } else if (frame_type == kFrameIntraOnly) {
      gold->refresh_frame_flags = 4;
      gold->force_integer_mv = 1;
    } else if (frame_type == kFrameInter) {
      gold->refresh_frame_flags = 4;
      gold->primary_reference_frame = 1;
      for (int i = 0; i < kNumInterReferenceFrameTypes; ++i) {
        gold->reference_frame_index[i] = i;
      }
      gold->is_motion_mode_switchable = true;
    }
  }

  void OverrideFrameSize(BytesAndBits* const data, ObuFrameHeader* const gold,
                         int flag_offset, int size_offset) {
    data->SetBit(flag_offset, 1);  // frame_size_override_flag.
    data->InsertLiteral(size_offset, kFrameWidthBits,
                        kWidth - 2);  // frame_width_minus_1.
    data->InsertLiteral(size_offset + kFrameWidthBits, kFrameHeightBits,
                        kHeight - 2);  // frame_height_minus_1.
    gold->frame_size_override_flag = true;
    gold->width = kWidth - 1;
    gold->height = kHeight - 1;
    gold->render_width = gold->width;
    gold->render_height = gold->height;
    gold->upscaled_width = gold->width;
  }

  void OverrideRenderSize(BytesAndBits* const data, ObuFrameHeader* const gold,
                          int flag_offset) {
    data->SetBit(flag_offset, 1);  // render_and_frame_size_different.
    data->InsertLiteral(flag_offset + 1, 16,
                        kWidth - 10);  // render_width_minus_1.
    data->InsertLiteral(flag_offset + 17, 16,
                        kHeight - 10);  // render_height_minus_1.
    gold->render_width = kWidth - 9;
    gold->render_height = kHeight - 9;
    gold->render_and_frame_size_different = true;
  }

  void OverrideSegmentation(BytesAndBits* const data, Segmentation* const gold,
                            int offset) {
    gold->update_data = true;
    data->SetBit(offset++, static_cast<uint8_t>(gold->update_data));
    libvpx_test::ACMRandom rnd(libvpx_test::ACMRandom::DeterministicSeed());
    gold->segment_id_pre_skip = false;
    gold->last_active_segment_id = 0;
    for (int i = 0; i < kMaxSegments; ++i) {
      for (int j = 0; j < kSegmentFeatureMax; ++j) {
        gold->feature_enabled[i][j] = static_cast<bool>(rnd.Rand8() & 1);
        data->InsertBit(offset++,
                        static_cast<uint8_t>(gold->feature_enabled[i][j]));
        if (gold->feature_enabled[i][j]) {
          gold->feature_data[i][j] = rnd(1 << kSegmentationFeatureBits[j]);
          if (Segmentation::FeatureSigned(static_cast<SegmentFeature>(j))) {
            if (static_cast<bool>(rnd.Rand8() & 1)) {
              gold->feature_data[i][j] *= -1;
            }
            data->InsertInverseSignedLiteral(
                offset, kSegmentationFeatureBits[j], gold->feature_data[i][j]);
            offset += kSegmentationFeatureBits[j] + 1;
          } else {
            data->InsertLiteral(offset, kSegmentationFeatureBits[j],
                                gold->feature_data[i][j]);
            offset += kSegmentationFeatureBits[j];
          }
          gold->last_active_segment_id = i;
          if (j >= kSegmentFeatureReferenceFrame) {
            gold->segment_id_pre_skip = true;
          }
        }
      }
    }
  }

  void VerifyObuHeader(bool extension) {
    EXPECT_EQ(obu_->obu_headers().back().temporal_id, extension ? 6 : 0);
    EXPECT_EQ(obu_->obu_headers().back().spatial_id, extension ? 2 : 0);
  }

#define OBU_TEST_COMPARE(x) EXPECT_EQ(expected.x, actual.x)
  void VerifyFrameParameters(const ObuFrameHeader& expected,
                             bool id_bits_present = false) {
    const ObuFrameHeader& actual = obu_->frame_header();
    OBU_TEST_COMPARE(show_existing_frame);
    if (actual.show_existing_frame) {
      OBU_TEST_COMPARE(frame_to_show);
      OBU_TEST_COMPARE(frame_presentation_time);
      if (id_bits_present) {
        OBU_TEST_COMPARE(display_frame_id);
      }
      return;
    }
    OBU_TEST_COMPARE(frame_type);
    OBU_TEST_COMPARE(show_frame);
    OBU_TEST_COMPARE(frame_presentation_time);
    OBU_TEST_COMPARE(showable_frame);
    OBU_TEST_COMPARE(error_resilient_mode);
    OBU_TEST_COMPARE(enable_cdf_update);
    OBU_TEST_COMPARE(current_frame_id);
    OBU_TEST_COMPARE(frame_size_override_flag);
    OBU_TEST_COMPARE(order_hint);
    for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
      OBU_TEST_COMPARE(reference_order_hint[i]);
    }
    OBU_TEST_COMPARE(primary_reference_frame);
    OBU_TEST_COMPARE(width);
    OBU_TEST_COMPARE(height);
    OBU_TEST_COMPARE(render_and_frame_size_different);
    OBU_TEST_COMPARE(render_width);
    OBU_TEST_COMPARE(render_height);
    OBU_TEST_COMPARE(upscaled_width);
    OBU_TEST_COMPARE(coded_lossless);
    OBU_TEST_COMPARE(upscaled_lossless);
    OBU_TEST_COMPARE(allow_screen_content_tools);
    OBU_TEST_COMPARE(is_motion_mode_switchable);
    OBU_TEST_COMPARE(refresh_frame_flags);
    OBU_TEST_COMPARE(enable_frame_end_update_cdf);
    OBU_TEST_COMPARE(force_integer_mv);
    if (actual.frame_type == kFrameInter) {
      for (int i = 0; i < kNumInterReferenceFrameTypes; ++i) {
        OBU_TEST_COMPARE(reference_frame_index[i]);
      }
    }
    OBU_TEST_COMPARE(use_superres);
    OBU_TEST_COMPARE(rows4x4);
    OBU_TEST_COMPARE(columns4x4);
  }

  void VerifyLoopFilterParameters(const LoopFilter& expected) {
    const LoopFilter& actual = obu_->frame_header().loop_filter;
    for (int i = 0; i < 4; ++i) {
      OBU_TEST_COMPARE(level[i]);
    }
    OBU_TEST_COMPARE(sharpness);
    OBU_TEST_COMPARE(delta_enabled);
    OBU_TEST_COMPARE(delta_update);
    for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
      OBU_TEST_COMPARE(ref_deltas[i]);
    }
    for (int i = 0; i < kLoopFilterMaxModeDeltas; ++i) {
      OBU_TEST_COMPARE(mode_deltas[i]);
    }
  }

  void VerifyQuantizerParameters(const QuantizerParameters& expected) {
    const QuantizerParameters& actual = obu_->frame_header().quantizer;
    OBU_TEST_COMPARE(base_index);
    OBU_TEST_COMPARE(delta_dc[kPlaneY]);
    OBU_TEST_COMPARE(delta_dc[kPlaneU]);
    OBU_TEST_COMPARE(delta_dc[kPlaneV]);
    EXPECT_EQ(0, actual.delta_ac[kPlaneY]);
    OBU_TEST_COMPARE(delta_ac[kPlaneY]);
    OBU_TEST_COMPARE(delta_ac[kPlaneU]);
    OBU_TEST_COMPARE(delta_ac[kPlaneV]);
    OBU_TEST_COMPARE(use_matrix);
    OBU_TEST_COMPARE(matrix_level[kPlaneY]);
    OBU_TEST_COMPARE(matrix_level[kPlaneU]);
    OBU_TEST_COMPARE(matrix_level[kPlaneV]);
  }

  void VerifySegmentationParameters(const Segmentation& expected) {
    const Segmentation& actual = obu_->frame_header().segmentation;
    OBU_TEST_COMPARE(enabled);
    OBU_TEST_COMPARE(update_map);
    OBU_TEST_COMPARE(update_data);
    OBU_TEST_COMPARE(temporal_update);
    OBU_TEST_COMPARE(segment_id_pre_skip);
    OBU_TEST_COMPARE(last_active_segment_id);
    for (int i = 0; i < kMaxSegments; ++i) {
      for (int j = 0; j < kSegmentFeatureMax; ++j) {
        OBU_TEST_COMPARE(feature_enabled[i][j]);
        OBU_TEST_COMPARE(feature_data[i][j]);
      }
    }
  }

  void VerifyDeltaParameters(const Delta& expected, const Delta& actual) {
    OBU_TEST_COMPARE(present);
    OBU_TEST_COMPARE(scale);
    OBU_TEST_COMPARE(multi);
  }

  void VerifyCdefParameters(const Cdef& expected) {
    const Cdef& actual = obu_->frame_header().cdef;
    OBU_TEST_COMPARE(damping);
    OBU_TEST_COMPARE(bits);
    for (int i = 0; i < (1 << actual.bits); ++i) {
      OBU_TEST_COMPARE(y_primary_strength[i]);
      OBU_TEST_COMPARE(y_secondary_strength[i]);
      OBU_TEST_COMPARE(uv_primary_strength[i]);
      OBU_TEST_COMPARE(uv_secondary_strength[i]);
    }
  }

  void VerifyLoopRestorationParameters(const LoopRestoration& expected) {
    const LoopRestoration& actual = obu_->frame_header().loop_restoration;
    for (int i = 0; i < kMaxPlanes; ++i) {
      OBU_TEST_COMPARE(type[i]);
      OBU_TEST_COMPARE(unit_size_log2[i]);
    }
  }

  void VerifyGlobalMotionParameters(
      const std::array<GlobalMotion, kNumReferenceFrameTypes>& gold) {
    for (int i = kReferenceFrameLast; i <= kReferenceFrameAlternate; ++i) {
      const GlobalMotion& expected = gold[i];
      const GlobalMotion& actual = obu_->frame_header().global_motion[i];
      OBU_TEST_COMPARE(type) << " i: " << i;
      for (int j = 0; j < 6; ++j) {
        OBU_TEST_COMPARE(params[j]) << " i: " << i << " j: " << j;
      }
    }
  }

  void VerifyFilmGrainParameters(const FilmGrainParams& expected) {
    const FilmGrainParams& actual = obu_->frame_header().film_grain_params;
    OBU_TEST_COMPARE(apply_grain);
    OBU_TEST_COMPARE(update_grain);
    OBU_TEST_COMPARE(chroma_scaling_from_luma);
    OBU_TEST_COMPARE(overlap_flag);
    OBU_TEST_COMPARE(clip_to_restricted_range);
    OBU_TEST_COMPARE(num_y_points);
    OBU_TEST_COMPARE(num_u_points);
    OBU_TEST_COMPARE(num_v_points);
    for (int i = 0; i < 14; ++i) {
      OBU_TEST_COMPARE(point_y_value[i]);
      OBU_TEST_COMPARE(point_y_scaling[i]);
    }
    for (int i = 0; i < 10; ++i) {
      OBU_TEST_COMPARE(point_u_value[i]);
      OBU_TEST_COMPARE(point_u_scaling[i]);
    }
    for (int i = 0; i < 10; ++i) {
      OBU_TEST_COMPARE(point_v_value[i]);
      OBU_TEST_COMPARE(point_v_scaling[i]);
    }
    OBU_TEST_COMPARE(chroma_scaling);
    OBU_TEST_COMPARE(auto_regression_coeff_lag);
    for (int i = 0; i < 24; ++i) {
      OBU_TEST_COMPARE(auto_regression_coeff_y[i]);
    }
    for (int i = 0; i < 25; ++i) {
      OBU_TEST_COMPARE(auto_regression_coeff_u[i]);
    }
    for (int i = 0; i < 25; ++i) {
      OBU_TEST_COMPARE(auto_regression_coeff_v[i]);
    }
    OBU_TEST_COMPARE(auto_regression_shift);
    OBU_TEST_COMPARE(grain_seed);
    OBU_TEST_COMPARE(reference_index);
    OBU_TEST_COMPARE(grain_scale_shift);
    OBU_TEST_COMPARE(u_multiplier);
    OBU_TEST_COMPARE(u_luma_multiplier);
    OBU_TEST_COMPARE(u_offset);
    OBU_TEST_COMPARE(v_multiplier);
    OBU_TEST_COMPARE(v_luma_multiplier);
    OBU_TEST_COMPARE(v_offset);
  }

  void VerifyTileInfoParameters(const TileInfo& expected) {
    const TileInfo& actual = obu_->frame_header().tile_info;
    OBU_TEST_COMPARE(uniform_spacing);
    OBU_TEST_COMPARE(tile_columns_log2);
    OBU_TEST_COMPARE(tile_columns);
    for (int i = 0; i < kMaxTileColumns + 1; ++i) {
      OBU_TEST_COMPARE(tile_column_start[i]) << "tile_column: " << i;
      OBU_TEST_COMPARE(tile_column_width_in_superblocks[i])
          << "tile_column: " << i;
    }
    OBU_TEST_COMPARE(tile_rows_log2);
    OBU_TEST_COMPARE(tile_rows);
    for (int i = 0; i < kMaxTileRows + 1; ++i) {
      OBU_TEST_COMPARE(tile_row_start[i]) << "tile_row: " << i;
      OBU_TEST_COMPARE(tile_row_height_in_superblocks[i]) << "tile_rows: " << i;
    }
    OBU_TEST_COMPARE(tile_count);
    OBU_TEST_COMPARE(context_update_id);
    OBU_TEST_COMPARE(tile_size_bytes);
  }

  void VerifySequenceHeader(const ObuSequenceHeader& expected) {
    EXPECT_TRUE(obu_->sequence_header_changed());
    const ObuSequenceHeader& actual = obu_->sequence_header();
    OBU_TEST_COMPARE(profile);
    OBU_TEST_COMPARE(still_picture);
    OBU_TEST_COMPARE(reduced_still_picture_header);
    OBU_TEST_COMPARE(operating_points);
    for (int i = 0; i < actual.operating_points; ++i) {
      OBU_TEST_COMPARE(operating_point_idc[i]) << "i: " << i;
      OBU_TEST_COMPARE(level[i].major) << "i: " << i;
      OBU_TEST_COMPARE(level[i].minor) << "i: " << i;
      OBU_TEST_COMPARE(tier[i]) << "i: " << i;
    }
    OBU_TEST_COMPARE(frame_width_bits);
    OBU_TEST_COMPARE(frame_height_bits);
    OBU_TEST_COMPARE(max_frame_width);
    OBU_TEST_COMPARE(max_frame_height);
    OBU_TEST_COMPARE(frame_id_numbers_present);
    if (actual.frame_id_numbers_present) {
      OBU_TEST_COMPARE(frame_id_length_bits);
      OBU_TEST_COMPARE(delta_frame_id_length_bits);
    }
    OBU_TEST_COMPARE(use_128x128_superblock);
    OBU_TEST_COMPARE(enable_filter_intra);
    OBU_TEST_COMPARE(enable_intra_edge_filter);
    OBU_TEST_COMPARE(enable_interintra_compound);
    OBU_TEST_COMPARE(enable_masked_compound);
    OBU_TEST_COMPARE(enable_warped_motion);
    OBU_TEST_COMPARE(enable_dual_filter);
    OBU_TEST_COMPARE(enable_order_hint);
    OBU_TEST_COMPARE(enable_jnt_comp);
    OBU_TEST_COMPARE(enable_ref_frame_mvs);
    OBU_TEST_COMPARE(choose_screen_content_tools);
    OBU_TEST_COMPARE(force_screen_content_tools);
    OBU_TEST_COMPARE(choose_integer_mv);
    OBU_TEST_COMPARE(force_integer_mv);
    OBU_TEST_COMPARE(order_hint_bits);
    OBU_TEST_COMPARE(enable_superres);
    OBU_TEST_COMPARE(enable_cdef);
    OBU_TEST_COMPARE(enable_restoration);
    OBU_TEST_COMPARE(color_config.bitdepth);
    OBU_TEST_COMPARE(color_config.is_monochrome);
    OBU_TEST_COMPARE(color_config.color_range);
    OBU_TEST_COMPARE(color_config.subsampling_x);
    OBU_TEST_COMPARE(color_config.subsampling_y);
    OBU_TEST_COMPARE(color_config.chroma_sample_position);
    OBU_TEST_COMPARE(timing_info_present_flag);
    OBU_TEST_COMPARE(timing_info.num_units_in_tick);
    OBU_TEST_COMPARE(timing_info.time_scale);
    OBU_TEST_COMPARE(timing_info.equal_picture_interval);
    OBU_TEST_COMPARE(timing_info.num_ticks_per_picture);
    OBU_TEST_COMPARE(decoder_model_info_present_flag);
    OBU_TEST_COMPARE(decoder_model_info.encoder_decoder_buffer_delay_length);
    OBU_TEST_COMPARE(decoder_model_info.num_units_in_decoding_tick);
    OBU_TEST_COMPARE(decoder_model_info.buffer_removal_time_length);
    OBU_TEST_COMPARE(decoder_model_info.frame_presentation_time_length);
    for (int i = 0; i < actual.operating_points; ++i) {
      SCOPED_TRACE("i: " + std::to_string(i));
      OBU_TEST_COMPARE(operating_parameters.decoder_buffer_delay[i]);
      OBU_TEST_COMPARE(operating_parameters.encoder_buffer_delay[i]);
      OBU_TEST_COMPARE(operating_parameters.low_delay_mode_flag[i]);
      OBU_TEST_COMPARE(initial_display_delay[i]);
    }
    OBU_TEST_COMPARE(film_grain_params_present);
  }

  void VerifyMetadataHdrCll(const ObuMetadataHdrCll& expected) {
    EXPECT_TRUE(obu_->current_frame_->hdr_cll_set());
    const ObuMetadataHdrCll& actual = obu_->current_frame_->hdr_cll();
    OBU_TEST_COMPARE(max_cll);
    OBU_TEST_COMPARE(max_fall);
  }

  void VerifyMetadataHdrMdcv(const ObuMetadataHdrMdcv& expected) {
    EXPECT_TRUE(obu_->current_frame_->hdr_mdcv_set());
    const ObuMetadataHdrMdcv& actual = obu_->current_frame_->hdr_mdcv();
    for (int i = 0; i < 3; ++i) {
      OBU_TEST_COMPARE(primary_chromaticity_x[i]);
      OBU_TEST_COMPARE(primary_chromaticity_y[i]);
    }
    OBU_TEST_COMPARE(white_point_chromaticity_x);
    OBU_TEST_COMPARE(white_point_chromaticity_y);
    OBU_TEST_COMPARE(luminance_max);
    OBU_TEST_COMPARE(luminance_min);
  }

  void VerifyMetadataItutT35(const ObuMetadataItutT35& expected) {
    EXPECT_TRUE(obu_->current_frame_->itut_t35_set());
    const ObuMetadataItutT35& actual = obu_->current_frame_->itut_t35();
    OBU_TEST_COMPARE(country_code);
    if (actual.country_code == 0xFF) {
      OBU_TEST_COMPARE(country_code_extension_byte);
    }
    ASSERT_EQ(expected.payload_size, actual.payload_size);
    if (actual.payload_size != 0) {
      EXPECT_EQ(memcmp(expected.payload_bytes, actual.payload_bytes,
                       actual.payload_size),
                0);
    }
  }

#undef OBU_TEST_COMPARE

  // Accessors to private members of ObuParser. This avoids the need for a
  // dependency on a googletest header in the main library for FRIEND_TEST()
  // (or the need to duplicate the implementation).
  bool ObuParseFrameParameters() { return obu_->ParseFrameParameters(); }
  bool ObuParseLoopFilterParameters() {
    return obu_->ParseLoopFilterParameters();
  }
  bool ObuParseLoopFilterDeltaParameters() {
    return obu_->ParseLoopFilterDeltaParameters();
  }
  bool ObuParseQuantizerParameters() {
    return obu_->ParseQuantizerParameters();
  }
  bool ObuParseQuantizerIndexDeltaParameters() {
    return obu_->ParseQuantizerIndexDeltaParameters();
  }
  void ObuComputeSegmentLosslessAndQIndex() {
    obu_->ComputeSegmentLosslessAndQIndex();
  }
  bool ObuParseCdefParameters() { return obu_->ParseCdefParameters(); }
  bool ObuParseLoopRestorationParameters() {
    return obu_->ParseLoopRestorationParameters();
  }
  bool ObuParseTxModeSyntax() { return obu_->ParseTxModeSyntax(); }
  bool ObuIsSkipModeAllowed() { return obu_->IsSkipModeAllowed(); }
  bool ObuParseSkipModeParameters() { return obu_->ParseSkipModeParameters(); }
  bool ObuReadAllowWarpedMotion() { return obu_->ReadAllowWarpedMotion(); }
  bool ObuSetFrameReferences(int8_t last_frame_idx, int8_t gold_frame_idx) {
    return obu_->SetFrameReferences(last_frame_idx, gold_frame_idx);
  }

  std::unique_ptr<BufferPool> buffer_pool_;
  DecoderState decoder_state_;
  std::unique_ptr<ObuParser> obu_;
  // The following members are reset with each Init().
  Vector<ObuHeader>* obu_headers_;
  ObuFrameHeader* obu_frame_header_;
  ObuSequenceHeader* obu_sequence_header_;
  RefCountedBufferPtr current_frame_;
};

TEST_F(ObuParserTest, InvalidInputs) {
  obu_.reset(new (std::nothrow)
                 ObuParser(nullptr, 0, 0, buffer_pool_.get(), &decoder_state_));
  EXPECT_EQ(obu_->ParseOneFrame(&current_frame_), kStatusInvalidArgument);
  obu_.reset(new (std::nothrow) ObuParser(nullptr, 10, 0, buffer_pool_.get(),
                                          &decoder_state_));
  EXPECT_EQ(obu_->ParseOneFrame(&current_frame_), kStatusInvalidArgument);
  obu_.reset(new (std::nothrow)
                 ObuParser(kDefaultTemporalDelimiter.data(), 0, 0,
                           buffer_pool_.get(), &decoder_state_));
  EXPECT_EQ(obu_->ParseOneFrame(&current_frame_), kStatusInvalidArgument);
}

TEST_F(ObuParserTest, TemporalDelimiter) {
  BytesAndBits data;
  data.AppendBytes(kDefaultTemporalDelimiter);

  ASSERT_TRUE(Parse(data.GenerateData()));
  EXPECT_EQ(obu_->obu_headers().size(), 1);
  EXPECT_EQ(obu_->obu_headers().back().type, kObuTemporalDelimiter);
  VerifyObuHeader(false);

  // forbidden_bit is not zero.
  data.SetBit(0, 1);
  EXPECT_FALSE(Parse(data.GenerateData()));
}

TEST_F(ObuParserTest, HeaderExtensions) {
  BytesAndBits data;
  data.AppendBytes(kDefaultTemporalDelimiterWithExtension);

  ASSERT_TRUE(Parse(data.GenerateData()));
  EXPECT_EQ(obu_->obu_headers().size(), 1);
  EXPECT_EQ(obu_->obu_headers().back().type, kObuTemporalDelimiter);
  VerifyObuHeader(true);

  // extension flag is set but no extensions found.
  data.Clear();
  data.AppendByte(kDefaultTemporalDelimiterWithExtension[0]);
  EXPECT_FALSE(Parse(data.GenerateData()));
}

TEST_F(ObuParserTest, HeaderHasSizeFieldNotSet) {
  BytesAndBits data;
  data.AppendBytes(kDefaultHeaderWithoutSizeField);

  EXPECT_FALSE(Parse(data.GenerateData()));
}

TEST_F(ObuParserTest, SequenceHeader) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

TEST_F(ObuParserTest, SequenceHeaderLevel) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  // Set level to 1.
  gold.level[0].major = 2;
  gold.level[0].minor = 1;
  data.SetLiteral(24, 5, 1);  // level.

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);

  // Set operating_point_idc of operating point 1 to 0x101 (temporal layer 0
  // and spatial layer 0 should be decoded). Set level of operating point 1 to
  // 8 (4.0) and tier to 1.
  gold.operating_points = 2;
  gold.operating_point_idc[1] = (1 << 0) | (1 << (0 + 8));
  gold.level[1].major = 4;
  gold.level[1].minor = 0;
  gold.tier[1] = 1;
  data.SetLiteral(7, 5, gold.operating_points - 1);
  data.InsertLiteral(29, 12, 0x101);  // operating_point_idc.
  data.InsertLiteral(41, 5, 8);       // level.
  data.InsertBit(46, gold.tier[1]);

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

TEST_F(ObuParserTest, SequenceHeaderProfile) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  gold.still_picture = true;
  data.SetBit(3, static_cast<uint8_t>(gold.still_picture));

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);

  // profile 2; bitdepth 8;
  gold.profile = kProfile2;
  gold.color_config.bitdepth = 8;
  gold.color_config.subsampling_x = 1;
  gold.color_config.subsampling_y = 0;
  data.SetLiteral(0, 3, gold.profile);

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);

  // profile 2; bitdepth 10;
  gold.color_config.bitdepth = 10;
  data.SetBit(73, 1);     // high_bitdepth.
  data.InsertBit(74, 0);  // twelve_bit.

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);

  // profile 2; bitdepth 12;
  gold.color_config.bitdepth = 12;
  gold.color_config.subsampling_y = 1;
  data.SetBit(74, 1);     // twelve_bit.
  data.InsertBit(78, 1);  // subsampling_x.
  data.InsertBit(79, 1);  // subsampling_y.

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

TEST_F(ObuParserTest, SequenceHeaderIdLength) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  gold.frame_id_numbers_present = true;
  gold.delta_frame_id_length_bits = kDeltaFrameIdLengthBits;
  gold.frame_id_length_bits = kFrameIdLengthBits;
  data.SetBit(54, 1);  // frame_id_numbers_present.
  data.InsertLiteral(55, 4, kDeltaFrameIdLengthBits - 2);
  data.InsertLiteral(59, 3, kFrameIdLengthBits - kDeltaFrameIdLengthBits - 1);

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

// An idLen greater than 16 is invalid.
TEST_F(ObuParserTest, SequenceHeaderIdLengthInvalid) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);

  data.SetBit(54, 1);  // frame_id_numbers_present.
  data.InsertLiteral(55, 4, kDeltaFrameIdLengthBits - 2);
  data.InsertLiteral(59, 3, 17 - kDeltaFrameIdLengthBits - 1);  // idLen = 17.

  ASSERT_FALSE(ParseSequenceHeader(data.GenerateData()));
}

TEST_F(ObuParserTest, SequenceHeaderFlags) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  gold.enable_warped_motion = true;
  gold.enable_superres = true;
  data.SetBit(60, 1);  // enable_warped_motion.
  data.SetBit(70, 1);  // enable_superres.

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

TEST_F(ObuParserTest, SequenceHeaderForceScreenContentToolsEqualTo0) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  gold.choose_screen_content_tools = false;
  gold.force_screen_content_tools = 0;
  gold.choose_integer_mv = false;
  gold.force_integer_mv = 2;
  data.SetBit(65, 0);  // choose_screen_content_tools.
  data.SetBit(66, 0);  // force_screen_content_tools.

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

TEST_F(ObuParserTest, SequenceHeaderMonochrome) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  gold.color_config.is_monochrome = true;
  gold.color_config.color_range = kColorRangeFull;
  data.SetBit(74, 1);     // monochrome.
  data.InsertBit(76, 1);  // color_range.

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

// This tests TimingInfo, DecoderModelInfo and OperatingParameters. The test is
// kind of long but it is the simplest way to test all three since they are
// dependent on one another.
TEST_F(ObuParserTest, SequenceHeaderTimingInfo) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  gold.timing_info_present_flag = true;
  gold.timing_info.num_units_in_tick = 100;
  gold.timing_info.time_scale = 1000;
  gold.timing_info.equal_picture_interval = false;
  gold.decoder_model_info_present_flag = false;
  data.SetBit(5, static_cast<uint8_t>(gold.timing_info_present_flag));
  data.InsertLiteral(6, 32, gold.timing_info.num_units_in_tick);
  data.InsertLiteral(38, 32, gold.timing_info.time_scale);
  data.InsertBit(70,
                 static_cast<uint8_t>(gold.timing_info.equal_picture_interval));
  data.InsertBit(71,
                 static_cast<uint8_t>(gold.decoder_model_info_present_flag));

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);

  gold.timing_info.equal_picture_interval = true;
  gold.timing_info.num_ticks_per_picture = 7;
  data.SetBit(70,
              static_cast<uint8_t>(gold.timing_info.equal_picture_interval));
  EXPECT_EQ(data.InsertUvlc(71, gold.timing_info.num_ticks_per_picture - 1), 5);

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);

  gold.decoder_model_info_present_flag = true;
  gold.decoder_model_info.encoder_decoder_buffer_delay_length = 5;
  gold.decoder_model_info.num_units_in_decoding_tick = 1000;
  gold.decoder_model_info.buffer_removal_time_length = 18;
  gold.decoder_model_info.frame_presentation_time_length = 20;

  data.SetBit(76, static_cast<uint8_t>(gold.decoder_model_info_present_flag));
  data.InsertLiteral(
      77, 5, gold.decoder_model_info.encoder_decoder_buffer_delay_length - 1);
  data.InsertLiteral(82, 32,
                     gold.decoder_model_info.num_units_in_decoding_tick);
  data.InsertLiteral(114, 5,
                     gold.decoder_model_info.buffer_removal_time_length - 1);
  data.InsertLiteral(
      119, 5, gold.decoder_model_info.frame_presentation_time_length - 1);
  data.InsertBit(147, 0);  // decoder_model_present_for_this_op.

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);

  gold.operating_parameters.decoder_buffer_delay[0] = 10;
  gold.operating_parameters.encoder_buffer_delay[0] = 20;
  gold.operating_parameters.low_delay_mode_flag[0] = true;

  data.SetBit(147, 1);  // decoder_model_present_for_this_op.
  data.InsertLiteral(
      148, gold.decoder_model_info.encoder_decoder_buffer_delay_length,
      gold.operating_parameters.decoder_buffer_delay[0]);
  data.InsertLiteral(
      153, gold.decoder_model_info.encoder_decoder_buffer_delay_length,
      gold.operating_parameters.encoder_buffer_delay[0]);
  data.InsertBit(158, static_cast<uint8_t>(
                          gold.operating_parameters.low_delay_mode_flag[0]));

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

TEST_F(ObuParserTest, SequenceHeaderInitialDisplayDelay) {
  BytesAndBits data;
  data.AppendBytes(kDefaultSequenceHeader);
  ObuSequenceHeader gold;
  DefaultSequenceHeader(&gold);

  gold.initial_display_delay[0] = 8;

  data.SetBit(6, 1);      // initial_display_delay_present_flag.
  data.InsertBit(29, 1);  // initial_display_delay_present_for_this_op.
  data.InsertLiteral(30, 4, gold.initial_display_delay[0] - 1);

  ASSERT_TRUE(ParseSequenceHeader(data.GenerateData()));
  VerifySequenceHeader(gold);
}

// Parsing of a frame header should fail if no sequence header has been
// received.
TEST_F(ObuParserTest, FrameHeaderWithoutSequenceHeader) {
  // The aom-test-data test vector av1-1-b8-01-size-16x16.ivf has two temporal
  // units. The first temporal unit has a presentation timestamp of 0 and
  // consists of three OBUs: a temporal delimiter OBU, a sequence header OBU,
  // and a frame OBU.
  const std::vector<uint8_t> kTemporalDelimiter = {0x12, 0x00};
  const std::vector<uint8_t> kSequenceHeader = {
      0x0a, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x9f, 0xfb, 0xff, 0xf3, 0x00, 0x80};
  const std::vector<uint8_t> kFrame = {
      0x32, 0xa6, 0x01, 0x10, 0x00, 0x87, 0x80, 0x00, 0x03, 0x00, 0x00, 0x00,
      0x40, 0x00, 0x9e, 0x86, 0x5b, 0xb2, 0x22, 0xb5, 0x58, 0x4d, 0x68, 0xe6,
      0x37, 0x54, 0x42, 0x7b, 0x84, 0xce, 0xdf, 0x9f, 0xec, 0xab, 0x07, 0x4d,
      0xf6, 0xe1, 0x5e, 0x9e, 0x27, 0xbf, 0x93, 0x2f, 0x47, 0x0d, 0x7b, 0x7c,
      0x45, 0x8d, 0xcf, 0x26, 0xf7, 0x6c, 0x06, 0xd7, 0x8c, 0x2e, 0xf5, 0x2c,
      0xb0, 0x8a, 0x31, 0xac, 0x69, 0xf5, 0xcd, 0xd8, 0x71, 0x5d, 0xaf, 0xf8,
      0x96, 0x43, 0x8c, 0x9c, 0x23, 0x6f, 0xab, 0xd0, 0x35, 0x43, 0xdf, 0x81,
      0x12, 0xe3, 0x7d, 0xec, 0x22, 0xb0, 0x30, 0x54, 0x32, 0x9f, 0x90, 0xc0,
      0x5d, 0x64, 0x9b, 0x0f, 0x75, 0x31, 0x84, 0x3a, 0x57, 0xd7, 0x5f, 0x03,
      0x6e, 0x7f, 0x43, 0x17, 0x6d, 0x08, 0xc3, 0x81, 0x8a, 0xae, 0x73, 0x1c,
      0xa8, 0xa7, 0xe4, 0x9c, 0xa9, 0x5b, 0x3f, 0xd1, 0xeb, 0x75, 0x3a, 0x7f,
      0x22, 0x77, 0x38, 0x64, 0x1c, 0x77, 0xdb, 0xcd, 0xef, 0xb7, 0x08, 0x45,
      0x8e, 0x7f, 0xea, 0xa3, 0xd0, 0x81, 0xc9, 0xc1, 0xbc, 0x93, 0x9b, 0x41,
      0xb1, 0xa1, 0x42, 0x17, 0x98, 0x3f, 0x1e, 0x95, 0xdf, 0x68, 0x7c, 0xb7,
      0x98};

  BytesAndBits data;
  data.AppendBytes(kTemporalDelimiter);
  // Skip the sequence header OBU.
  data.AppendBytes(kFrame);
  ASSERT_FALSE(Parse(data.GenerateData()));

  // Now verify that all three OBUs are correct, by adding them to |data|
  // successively.
  data.Clear();
  data.AppendBytes(kTemporalDelimiter);
  ASSERT_TRUE(Parse(data.GenerateData()));
  data.Clear();
  data.AppendBytes(kTemporalDelimiter);
  data.AppendBytes(kSequenceHeader);
  ASSERT_TRUE(Parse(data.GenerateData()));
  data.Clear();
  data.AppendBytes(kTemporalDelimiter);
  data.AppendBytes(kSequenceHeader);
  data.AppendBytes(kFrame);
  ASSERT_TRUE(Parse(data.GenerateData()));
}

TEST_F(ObuParserTest, FrameParameterShowExistingFrame) {
  BytesAndBits data;
  data.AppendBit(1);                    // show_existing_frame.
  data.AppendLiteral(3, kFrameToShow);  // frame_to_show.
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);
  gold.show_existing_frame = true;
  gold.frame_to_show = kFrameToShow;

  // kFrameToShow'th frame is not yet decoded.
  ASSERT_FALSE(ParseFrameParameters(data.GenerateData()));

  decoder_state_.reference_frame[kFrameToShow] = buffer_pool_->GetFreeBuffer();
  // kFrameToShow'th frame is not a showable frame.
  ASSERT_FALSE(ParseFrameParameters(data.GenerateData()));

  decoder_state_.reference_frame[kFrameToShow]->set_showable_frame(true);
  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParametersShowExistingFrameWithDisplayFrameId) {
  BytesAndBits data;
  data.AppendBit(1);                        // show_existing_frame.
  data.AppendLiteral(3, kFrameToShow);      // frame_to_show.
  data.AppendLiteral(15, kDisplayFrameId);  // display_frame_id.
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);
  gold.show_existing_frame = true;
  gold.frame_to_show = kFrameToShow;
  gold.display_frame_id = kDisplayFrameId;

  // kFrameToShow'th frame is not yet decoded.
  ASSERT_FALSE(ParseFrameParameters(data.GenerateData(), true));

  decoder_state_.reference_frame_id[kFrameToShow] = kDisplayFrameId;
  decoder_state_.reference_frame[kFrameToShow] = buffer_pool_->GetFreeBuffer();
  // kFrameToShow'th frame is not a showable frame.
  ASSERT_FALSE(ParseFrameParameters(data.GenerateData(), true));

  decoder_state_.reference_frame[kFrameToShow]->set_showable_frame(true);
  ASSERT_TRUE(ParseFrameParameters(data.GenerateData(), true));
  VerifyFrameParameters(gold, true);
}

TEST_F(ObuParserTest, FrameParameterShowExistingFrameTemporalPointInfo) {
  BytesAndBits data;
  data.AppendBit(1);                    // show_existing_frame.
  data.AppendLiteral(3, kFrameToShow);  // frame_to_show.
  data.AppendLiteral(20, 38);           // frame_presentation_time.
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);
  gold.show_existing_frame = true;
  gold.frame_to_show = kFrameToShow;
  gold.frame_presentation_time = 38;

  EXPECT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->frame_width_bits = kFrameWidthBits;
  obu_sequence_header_->frame_height_bits = kFrameHeightBits;
  obu_sequence_header_->max_frame_width = kWidth;
  obu_sequence_header_->max_frame_height = kHeight;

  obu_sequence_header_->decoder_model_info_present_flag = true;
  obu_sequence_header_->decoder_model_info.frame_presentation_time_length = 20;

  decoder_state_.reference_frame[kFrameToShow] = buffer_pool_->GetFreeBuffer();
  decoder_state_.reference_frame[kFrameToShow]->set_showable_frame(true);

  ASSERT_TRUE(ObuParseFrameParameters());
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterErrorResilientMode) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderIntraOnlyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameIntraOnly);

  gold.error_resilient_mode = true;
  data.SetBit(4, static_cast<uint8_t>(gold.error_resilient_mode));

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterKeyFrame) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderKeyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterKeyFrameTemporalPointInfo) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderKeyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);

  data.InsertLiteral(4, 20, 38);  // frame_presentation_time.
  gold.frame_presentation_time = 38;

  EXPECT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->frame_width_bits = kFrameWidthBits;
  obu_sequence_header_->frame_height_bits = kFrameHeightBits;
  obu_sequence_header_->max_frame_width = kWidth;
  obu_sequence_header_->max_frame_height = kHeight;

  obu_sequence_header_->decoder_model_info_present_flag = true;
  obu_sequence_header_->decoder_model_info.frame_presentation_time_length = 20;

  ASSERT_TRUE(ObuParseFrameParameters());
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterKeyFrameOverrideSize) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderKeyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);

  OverrideFrameSize(&data, &gold, 5, 6);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);

  OverrideRenderSize(&data, &gold, 23);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterKeyFrameSuperRes) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderKeyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);
  gold.use_superres = true;
  gold.superres_scale_denominator = 15;
  gold.width = kWidth * 8 / 15;
  gold.columns4x4 = 58;

  data.SetBit(6, static_cast<int>(gold.use_superres));
  data.SetLiteral(7, 3, gold.superres_scale_denominator - 9);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData(), false, 0, 0, true));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterKeyFrameAllowScreenContentTools) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderKeyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameKey);

  data.InsertBit(5, 1);  // allow_screen_content_tools.
  data.InsertBit(8, 1);  // allow_intrabc.
  gold.allow_screen_content_tools = true;
  gold.allow_intrabc = true;

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData(), false, 2));
  VerifyFrameParameters(gold);

  data.InsertBit(6, 1);  // force_integer_mv.
  gold.force_integer_mv = 1;

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData(), false, 2, 2));
  VerifyFrameParameters(gold);

  data.SetBit(6, 0);  // force_integer_mv.

  // Gold need not be updated, because force_integer_mv is always 1 for
  // keyframes.
  ASSERT_TRUE(ParseFrameParameters(data.GenerateData(), false, 2, 2));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterIntraOnlyFrame) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderIntraOnlyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameIntraOnly);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterIntraOnlyFrameOverrideSize) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderIntraOnlyFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameIntraOnly);

  OverrideFrameSize(&data, &gold, 6, 15);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);

  OverrideRenderSize(&data, &gold, 32);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

// An INTRA_ONLY_FRAME cannot set refresh_frame_flags to 0xff.
TEST_F(ObuParserTest, FrameParameterIntraOnlyFrameRefreshAllFrames) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderIntraOnlyFrame);
  data.SetLiteral(7, 8, 0xFF);  // refresh_frame_flags.

  ASSERT_FALSE(ParseFrameParameters(data.GenerateData()));
}

TEST_F(ObuParserTest, FrameParameterInterFrame) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderInterFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameInter);
  ObuFrameHeader reference_frame_header;
  reference_frame_header.width = kWidth;
  reference_frame_header.height = kHeight;
  reference_frame_header.render_width = kWidth;
  reference_frame_header.render_height = kHeight;
  reference_frame_header.upscaled_width = kWidth;
  reference_frame_header.rows4x4 = kRows4x4;
  reference_frame_header.columns4x4 = kColumns4x4;
  reference_frame_header.refresh_frame_flags = 0;
  for (auto& reference_frame : decoder_state_.reference_frame) {
    reference_frame = buffer_pool_->GetFreeBuffer();
    EXPECT_TRUE(reference_frame->SetFrameDimensions(reference_frame_header));
  }

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

TEST_F(ObuParserTest, FrameParameterInterFrameOverrideSize) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderInterFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameInter);
  ObuFrameHeader reference_frame_header;
  reference_frame_header.width = kWidth;
  reference_frame_header.height = kHeight;
  reference_frame_header.render_width = kWidth;
  reference_frame_header.render_height = kHeight;
  reference_frame_header.upscaled_width = kWidth;
  reference_frame_header.rows4x4 = kRows4x4;
  reference_frame_header.columns4x4 = kColumns4x4;
  reference_frame_header.refresh_frame_flags = 0;
  for (auto& reference_frame : decoder_state_.reference_frame) {
    reference_frame = buffer_pool_->GetFreeBuffer();
    EXPECT_TRUE(reference_frame->SetFrameDimensions(reference_frame_header));
  }

  data.InsertLiteral(39, kNumInterReferenceFrameTypes, 0);  // found_ref.
  OverrideFrameSize(&data, &gold, 6, 46);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);

  OverrideRenderSize(&data, &gold, 63);

  ASSERT_TRUE(ParseFrameParameters(data.GenerateData()));
  VerifyFrameParameters(gold);
}

// This test verifies we check the following requirement at the end of Section
// 6.8.4:
//   If FrameIsIntra is equal to 0 (indicating that this frame may use inter
//   prediction), the requirements described in the frame size with refs
//   semantics of section 6.8.6 must also be satisfied.
TEST_F(ObuParserTest, FrameParameterInterFrameInvalidSize) {
  BytesAndBits data;
  data.AppendBytes(kDefaultFrameHeaderInterFrame);
  ObuFrameHeader gold;
  DefaultFrameHeader(&gold, kFrameInter);
  ObuFrameHeader reference_frame_header;
  reference_frame_header.width = kWidth;
  reference_frame_header.height = 2 * kHeight + 8;
  reference_frame_header.render_width = kWidth;
  reference_frame_header.render_height = 2 * kHeight + 8;
  reference_frame_header.upscaled_width = kWidth;
  reference_frame_header.rows4x4 = 2 * kRows4x4 + 2;
  reference_frame_header.columns4x4 = kColumns4x4;
  reference_frame_header.refresh_frame_flags = 0;
  for (auto& reference_frame : decoder_state_.reference_frame) {
    reference_frame = buffer_pool_->GetFreeBuffer();
    EXPECT_TRUE(reference_frame->SetFrameDimensions(reference_frame_header));
  }

  EXPECT_FALSE(ParseFrameParameters(data.GenerateData()));
}

// Tests the ObuParser::SetFrameReferences() method.
//
// This method uses the following data members as input:
//   decoder_state_.reference_order_hint
//   sequence_header_.enable_order_hint
//   sequence_header_.order_hint_bits
//   frame_header_.order_hint
// So we need to set up these data members before calling
// ObuParser::SetFrameReferences().
//
// The output is in frame_header_.reference_frame_index.
TEST_F(ObuParserTest, SetFrameReferences) {
  // All reference frames are forward references (because 9 < 17).
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    decoder_state_.reference_order_hint[i] = 9;
  }

  ASSERT_TRUE(Init());
  obu_sequence_header_->enable_order_hint = true;
  obu_sequence_header_->order_hint_bits = 5;
  obu_sequence_header_->order_hint_shift_bits =
      Mod32(32 - obu_sequence_header_->order_hint_bits);
  obu_frame_header_->order_hint = 17;

  const int8_t last_frame_idx = 0;
  const int8_t gold_frame_idx = 1;

  // Since all reference frames are forward references, we set the remaining
  // five references in reverse chronological order. So Last2, Last3, Backward,
  // Alternate2, and Alternate are set to 7, 6, 5, 4, and 3, respectively.

  EXPECT_TRUE(ObuSetFrameReferences(last_frame_idx, gold_frame_idx));

  EXPECT_EQ(
      obu_frame_header_
          ->reference_frame_index[kReferenceFrameLast - kReferenceFrameLast],
      0);
  EXPECT_EQ(
      obu_frame_header_
          ->reference_frame_index[kReferenceFrameLast2 - kReferenceFrameLast],
      7);
  EXPECT_EQ(
      obu_frame_header_
          ->reference_frame_index[kReferenceFrameLast3 - kReferenceFrameLast],
      6);
  EXPECT_EQ(
      obu_frame_header_
          ->reference_frame_index[kReferenceFrameGolden - kReferenceFrameLast],
      1);
  EXPECT_EQ(obu_frame_header_->reference_frame_index[kReferenceFrameBackward -
                                                     kReferenceFrameLast],
            5);
  EXPECT_EQ(obu_frame_header_->reference_frame_index[kReferenceFrameAlternate2 -
                                                     kReferenceFrameLast],
            4);
  EXPECT_EQ(obu_frame_header_->reference_frame_index[kReferenceFrameAlternate -
                                                     kReferenceFrameLast],
            3);
}

TEST_F(ObuParserTest, LoopFilterParameters) {
  LoopFilter gold;
  memset(&gold, 0, sizeof(gold));

  BytesAndBits data;
  data.AppendBit(0);  // dummy.

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->primary_reference_frame = kPrimaryReferenceNone;
  obu_frame_header_->coded_lossless = true;
  gold.ref_deltas[kReferenceFrameIntra] = 1;
  gold.ref_deltas[kReferenceFrameGolden] = -1;
  gold.ref_deltas[kReferenceFrameAlternate] = -1;
  gold.ref_deltas[kReferenceFrameAlternate2] = -1;
  ASSERT_TRUE(ObuParseLoopFilterParameters());
  VerifyLoopFilterParameters(gold);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->primary_reference_frame = kPrimaryReferenceNone;
  obu_frame_header_->allow_intrabc = true;
  ASSERT_TRUE(ObuParseLoopFilterParameters());
  VerifyLoopFilterParameters(gold);

  gold.level[0] = 32;
  gold.level[3] = 48;
  gold.sharpness = 4;
  data.Clear();
  for (const auto& level : gold.level) {
    data.AppendLiteral(6, level);
  }
  data.AppendLiteral(3, gold.sharpness);
  data.AppendBit(0);  // delta_enabled.

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->primary_reference_frame = kPrimaryReferenceNone;
  ASSERT_TRUE(ObuParseLoopFilterParameters());
  VerifyLoopFilterParameters(gold);

  gold.delta_enabled = true;
  gold.delta_update = true;
  gold.ref_deltas[0] = 20;
  gold.mode_deltas[0] = -20;
  data.SetBit(27, 1);  // delta_enabled.
  data.AppendBit(1);   // delta_update.
  for (int i = 0; i < kNumReferenceFrameTypes; ++i) {
    if (i == 0) {
      data.AppendBit(1);  // update_ref_delta.
      data.AppendInverseSignedLiteral(6, gold.ref_deltas[0]);  // ref_delta.
    } else {
      data.AppendBit(0);  // update_ref_delta.
    }
  }
  for (int i = 0; i < kLoopFilterMaxModeDeltas; ++i) {
    if (i == 0) {
      data.AppendBit(1);  // update_mode_delta.
      data.AppendInverseSignedLiteral(6, gold.mode_deltas[0]);  // mode_delta.
    } else {
      data.AppendBit(0);  // update_mode_delta.
    }
  }

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->primary_reference_frame = kPrimaryReferenceNone;
  ASSERT_TRUE(ObuParseLoopFilterParameters());
  VerifyLoopFilterParameters(gold);
}

TEST_F(ObuParserTest, QuantizerParameters) {
  QuantizerParameters gold = {};
  gold.base_index = 48;

  BytesAndBits data;
  data.AppendLiteral(8, gold.base_index);
  data.AppendLiteral(3, 0);  // delta_coded.
  data.AppendBit(0);         // use_matrix.

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);
}

TEST_F(ObuParserTest, QuantizerParametersMonochrome) {
  QuantizerParameters gold = {};
  gold.base_index = 48;

  BytesAndBits data;
  data.AppendLiteral(8, gold.base_index);
  data.AppendBit(0);  // delta_coded.
  data.AppendBit(0);  // use_matrix.
  // The quantizer parameters end here. Add a 1 bit. It should not be parsed.
  data.AppendBit(1);  // Would be segmentation_enabled in a bitstream.

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->color_config.is_monochrome = true;
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);
}

TEST_F(ObuParserTest, QuantizerParametersDeltaCoded) {
  QuantizerParameters gold = {};
  gold.base_index = 48;
  gold.delta_dc[kPlaneY] = -30;

  BytesAndBits data;
  data.AppendLiteral(8, gold.base_index);
  data.AppendBit(1);  // delta_coded.
  data.AppendInverseSignedLiteral(6, gold.delta_dc[kPlaneY]);
  data.AppendLiteral(2, 0);  // delta_coded u dc/ac.
  data.AppendBit(0);         // use_matrix.

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);

  gold.delta_dc[kPlaneU] = -40;
  gold.delta_dc[kPlaneV] = gold.delta_dc[kPlaneU];
  data.SetBit(16, 1);  // delta_coded.
  data.InsertInverseSignedLiteral(17, 6, gold.delta_dc[kPlaneU]);

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);

  gold.delta_ac[kPlaneU] = 50;
  gold.delta_ac[kPlaneV] = gold.delta_ac[kPlaneU];
  data.SetBit(24, 1);  // delta_coded.
  data.InsertInverseSignedLiteral(25, 6, gold.delta_ac[kPlaneU]);

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);

  gold.delta_dc[kPlaneV] = 60;
  gold.delta_ac[kPlaneV] = 0;
  data.InsertBit(16, 1);  // diff_uv_delta.
  data.InsertBit(33, 1);  // delta_coded.
  data.InsertInverseSignedLiteral(34, 6, gold.delta_dc[kPlaneV]);
  data.InsertBit(41, 0);  // delta_coded.

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->color_config.separate_uv_delta_q = true;
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);

  gold.delta_ac[kPlaneV] = -20;
  data.SetBit(41, 1);  // delta_coded.
  data.InsertInverseSignedLiteral(42, 6, gold.delta_ac[kPlaneV]);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->color_config.separate_uv_delta_q = true;
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);
}

TEST_F(ObuParserTest, QuantizerParametersUseQmatrix) {
  QuantizerParameters gold = {};
  gold.base_index = 48;
  gold.use_matrix = true;
  gold.matrix_level[kPlaneY] = 3;
  gold.matrix_level[kPlaneU] = 6;
  gold.matrix_level[kPlaneV] = gold.matrix_level[kPlaneU];

  // Test three cases.
  // 1. separate_uv_delta_q = false (which implies diff_uv_delta = false).
  BytesAndBits data;
  data.AppendLiteral(8, gold.base_index);
  data.AppendLiteral(3, 0);  // delta_coded.
  data.AppendBit(static_cast<uint8_t>(gold.use_matrix));
  data.AppendLiteral(4, gold.matrix_level[kPlaneY]);
  data.AppendLiteral(4, gold.matrix_level[kPlaneU]);

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);

  // 2. separate_uv_delta_q = true and diff_uv_delta = false.
  gold.matrix_level[kPlaneV] = 5;
  data.InsertBit(9, 0);  // diff_uv_delta.
  data.AppendLiteral(4, gold.matrix_level[kPlaneV]);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->color_config.separate_uv_delta_q = true;
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);

  // 3. separate_uv_delta_q = true and diff_uv_delta = true.
  data.SetBit(9, 1);             // diff_uv_delta.
  data.InsertLiteral(12, 2, 0);  // delta_coded.
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->color_config.separate_uv_delta_q = true;
  ASSERT_TRUE(ObuParseQuantizerParameters());
  VerifyQuantizerParameters(gold);
}

TEST_F(ObuParserTest, SegmentationParameters) {
  const int kPrimaryReferenceNotNone = 1;
  const int kPrevFrameIndexNotNone = 2;

  // Set up decoder_state_ with a previous frame containing saved segmentation
  // parameters.
  decoder_state_.reference_frame[kPrevFrameIndexNotNone] =
      buffer_pool_->GetFreeBuffer();
  ASSERT_NE(decoder_state_.reference_frame[kPrevFrameIndexNotNone], nullptr);
  Segmentation prev_segmentation = {};
  prev_segmentation.feature_enabled[2][0] = true;
  prev_segmentation.feature_enabled[5][0] = true;
  prev_segmentation.last_active_segment_id = 5;
  decoder_state_.reference_frame[kPrevFrameIndexNotNone]
      ->SetSegmentationParameters(prev_segmentation);

  Segmentation gold;
  memset(&gold, 0, sizeof(gold));

  BytesAndBits data;
  data.AppendBit(0);  // segmentation_enabled.

  // Since segmentation_enabled is false, we expect the parameters to be all
  // zero/false.
  ASSERT_TRUE(ParseSegmentationParameters(
      data.GenerateData(), kPrimaryReferenceNotNone, kPrevFrameIndexNotNone));
  VerifySegmentationParameters(gold);

  gold.enabled = true;
  gold.update_map = true;
  gold.temporal_update = true;
  data.SetBit(0, static_cast<uint8_t>(gold.enabled));
  data.AppendBit(static_cast<uint8_t>(gold.update_map));
  data.AppendBit(static_cast<uint8_t>(gold.temporal_update));
  data.AppendBit(static_cast<uint8_t>(gold.update_data));

  // Since update_data is false, we expect the parameters to be loaded from the
  // previous frame in |decoder_state_|. So change |gold| accordingly.
  gold.feature_enabled[2][0] = true;
  gold.feature_enabled[5][0] = true;
  gold.last_active_segment_id = 5;

  ASSERT_TRUE(ParseSegmentationParameters(
      data.GenerateData(), kPrimaryReferenceNotNone, kPrevFrameIndexNotNone));
  VerifySegmentationParameters(gold);

  OverrideSegmentation(&data, &gold, 3);

  ASSERT_TRUE(ParseSegmentationParameters(
      data.GenerateData(), kPrimaryReferenceNotNone, kPrevFrameIndexNotNone));
  VerifySegmentationParameters(gold);

  // If primary_ref_frame is kPrimaryReferenceNone, these three fields are
  // implied.
  data.RemoveBit(1);  // segmentation_update_map.
  data.RemoveBit(1);  // segmentation_temporal_update.
  data.RemoveBit(1);  // segmentation_update_data.
  gold.update_map = true;
  gold.temporal_update = false;
  gold.update_data = true;

  // Since update_data is true, we expect the parameters to be read from
  // |data|.
  ASSERT_TRUE(ParseSegmentationParameters(data.GenerateData(),
                                          kPrimaryReferenceNone, 0));
  VerifySegmentationParameters(gold);
}

TEST_F(ObuParserTest, QuantizerIndexDeltaParameters) {
  BytesAndBits data;
  data.AppendBit(1);         // delta_q_present.
  data.AppendLiteral(2, 2);  // delta_q_res.

  Delta gold;
  memset(&gold, 0, sizeof(gold));

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseQuantizerIndexDeltaParameters());
  VerifyDeltaParameters(gold, obu_->frame_header().delta_q);

  gold.present = true;
  gold.scale = 2;
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->quantizer.base_index = 40;
  ASSERT_TRUE(ObuParseQuantizerIndexDeltaParameters());
  VerifyDeltaParameters(gold, obu_->frame_header().delta_q);
}

TEST_F(ObuParserTest, LoopFilterDeltaParameters) {
  BytesAndBits data;
  data.AppendBit(1);         // delta_lf_present.
  data.AppendLiteral(2, 2);  // delta_lf_res.
  data.AppendBit(1);         // delta_lf_multi.

  Delta gold;
  memset(&gold, 0, sizeof(gold));

  // delta_q_present is false, so loop filter delta will not be read.
  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseLoopFilterDeltaParameters());
  VerifyDeltaParameters(gold, obu_->frame_header().delta_lf);

  // allow_intrabc is true, so loop filter delta will not be read.
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->delta_q.present = true;
  obu_frame_header_->allow_intrabc = true;
  ASSERT_TRUE(ObuParseLoopFilterDeltaParameters());
  VerifyDeltaParameters(gold, obu_->frame_header().delta_lf);

  gold.present = true;
  gold.scale = 2;
  gold.multi = true;
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->delta_q.present = true;
  ASSERT_TRUE(ObuParseLoopFilterDeltaParameters());
  VerifyDeltaParameters(gold, obu_->frame_header().delta_lf);
}

TEST_F(ObuParserTest, ComputeSegmentLosslessAndQIndex) {
  BytesAndBits data;
  data.AppendBit(0);  // dummy.

  ASSERT_TRUE(Init(data.GenerateData()));

  // Segmentation is disabled. All quantizers are 0.
  ObuComputeSegmentLosslessAndQIndex();
  EXPECT_TRUE(obu_->frame_header().coded_lossless);
  EXPECT_TRUE(obu_->frame_header().upscaled_lossless);
  for (const auto& qindex : obu_->frame_header().segmentation.qindex) {
    EXPECT_EQ(qindex, 0);
  }

  // Segmentation is enabled. All quantizers are zero.
  obu_frame_header_->segmentation.enabled = true;
  ObuComputeSegmentLosslessAndQIndex();
  EXPECT_TRUE(obu_->frame_header().coded_lossless);
  EXPECT_TRUE(obu_->frame_header().upscaled_lossless);
  for (const auto& qindex : obu_->frame_header().segmentation.qindex) {
    EXPECT_EQ(qindex, 0);
  }

  // Segmentation is enabled. All quantizers are zero. upscaled_width != width.
  obu_frame_header_->segmentation.enabled = true;
  obu_frame_header_->upscaled_width = 100;
  ObuComputeSegmentLosslessAndQIndex();
  EXPECT_TRUE(obu_->frame_header().coded_lossless);
  EXPECT_FALSE(obu_->frame_header().upscaled_lossless);
  for (const auto& qindex : obu_->frame_header().segmentation.qindex) {
    EXPECT_EQ(qindex, 0);
  }

  // Segmentation in disabled. Some quantizer deltas are non zero.
  obu_frame_header_->segmentation.enabled = false;
  obu_frame_header_->quantizer.delta_dc[kPlaneY] = 40;
  ObuComputeSegmentLosslessAndQIndex();
  EXPECT_FALSE(obu_->frame_header().coded_lossless);
  EXPECT_FALSE(obu_->frame_header().upscaled_lossless);
  for (const auto& qindex : obu_->frame_header().segmentation.qindex) {
    EXPECT_EQ(qindex, 0);
  }

  // Segmentation is disabled. Quantizer base index is non zero.
  obu_frame_header_->segmentation.enabled = true;
  obu_frame_header_->quantizer.delta_dc[kPlaneY] = 0;
  obu_frame_header_->quantizer.base_index = 40;
  ObuComputeSegmentLosslessAndQIndex();
  EXPECT_FALSE(obu_->frame_header().coded_lossless);
  EXPECT_FALSE(obu_->frame_header().upscaled_lossless);
  for (const auto& qindex : obu_->frame_header().segmentation.qindex) {
    EXPECT_EQ(qindex, 40);
  }
}

TEST_F(ObuParserTest, CdefParameters) {
  Cdef gold;
  memset(&gold, 0, sizeof(gold));
  const int coeff_shift = 2;  // bitdepth - 8.
  gold.damping = 3 + coeff_shift;

  BytesAndBits data;
  data.AppendBit(0);  // dummy.

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->color_config.bitdepth = 10;
  ASSERT_TRUE(ObuParseCdefParameters());
  // Cdef will be {0} except for damping because enable_cdef is false.
  VerifyCdefParameters(gold);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->enable_cdef = true;
  obu_sequence_header_->color_config.bitdepth = 10;
  obu_frame_header_->coded_lossless = true;
  ASSERT_TRUE(ObuParseCdefParameters());
  // Cdef will be {0} except for damping because coded_lossless is true.
  VerifyCdefParameters(gold);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->enable_cdef = true;
  obu_sequence_header_->color_config.bitdepth = 10;
  obu_frame_header_->allow_intrabc = true;
  ASSERT_TRUE(ObuParseCdefParameters());
  // Cdef will be {0} except for damping because allow_intrabc is true.
  VerifyCdefParameters(gold);

  gold.damping = 5;
  gold.bits = 1;
  data.Clear();
  data.AppendLiteral(2, gold.damping - 3);  // cdef_damping_minus3.
  gold.damping += coeff_shift;
  data.AppendLiteral(2, gold.bits);  // cdef_bits.
  for (int i = 0; i < 2; ++i) {
    gold.y_primary_strength[i] = 10;
    gold.y_secondary_strength[i] = (i == 0) ? 2 : 3;
    gold.uv_primary_strength[i] = 12;
    gold.uv_secondary_strength[i] = (i == 1) ? 2 : 3;
    data.AppendLiteral(4, gold.y_primary_strength[i]);
    data.AppendLiteral(2, gold.y_secondary_strength[i]);
    data.AppendLiteral(4, gold.uv_primary_strength[i]);
    data.AppendLiteral(2, gold.uv_secondary_strength[i]);
    if (gold.y_secondary_strength[i] == 3) ++gold.y_secondary_strength[i];
    if (gold.uv_secondary_strength[i] == 3) ++gold.uv_secondary_strength[i];
    gold.y_primary_strength[i] <<= coeff_shift;
    gold.uv_primary_strength[i] <<= coeff_shift;
    gold.y_secondary_strength[i] <<= coeff_shift;
    gold.uv_secondary_strength[i] <<= coeff_shift;
  }

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_sequence_header_->enable_cdef = true;
  obu_sequence_header_->color_config.bitdepth = 10;
  ASSERT_TRUE(ObuParseCdefParameters());
  VerifyCdefParameters(gold);
}

TEST_F(ObuParserTest, LoopRestorationParameters) {
  for (bool use_128x128_superblock : testing::Bool()) {
    SCOPED_TRACE("use_128x128_superblock: " +
                 std::to_string(use_128x128_superblock));
    LoopRestoration gold;
    memset(&gold, 0, sizeof(gold));

    BytesAndBits data;
    data.AppendBit(0);  // dummy.

    // enable_restoration is false. nothing will be read.
    ASSERT_TRUE(Init(data.GenerateData()));
    obu_frame_header_->allow_intrabc = true;
    obu_frame_header_->coded_lossless = true;
    ASSERT_TRUE(ObuParseLoopRestorationParameters());
    VerifyLoopRestorationParameters(gold);

    // allow_intrabc is true. nothing will be read.
    ASSERT_TRUE(Init(data.GenerateData()));
    obu_frame_header_->allow_intrabc = true;
    obu_sequence_header_->enable_restoration = true;
    ASSERT_TRUE(ObuParseLoopRestorationParameters());
    VerifyLoopRestorationParameters(gold);

    // coded_lossless is true. nothing will be read.
    ASSERT_TRUE(Init(data.GenerateData()));
    obu_frame_header_->coded_lossless = true;
    obu_sequence_header_->enable_restoration = true;
    ASSERT_TRUE(ObuParseLoopRestorationParameters());
    VerifyLoopRestorationParameters(gold);

    data.Clear();
    for (int i = 0; i < kMaxPlanes; ++i) {
      data.AppendLiteral(2, kLoopRestorationTypeNone);  // lr_type.
    }

    ASSERT_TRUE(Init(data.GenerateData()));
    obu_sequence_header_->enable_restoration = true;
    obu_sequence_header_->use_128x128_superblock = use_128x128_superblock;
    ASSERT_TRUE(ObuParseLoopRestorationParameters());
    VerifyLoopRestorationParameters(gold);

    gold.type[0] = gold.type[1] = kLoopRestorationTypeWiener;
    gold.unit_size_log2[0] = gold.unit_size_log2[1] = gold.unit_size_log2[2] =
        use_128x128_superblock ? 8 : 7;
    data.SetLiteral(0, 2, gold.type[0]);  // lr_type.
    data.SetLiteral(2, 2, gold.type[0]);  // lr_type.
    data.AppendBit(1);                    // lr_unit_shift.
    if (!use_128x128_superblock) {
      data.AppendBit(0);  // lr_unit_extra_shift.
    }

    ASSERT_TRUE(Init(data.GenerateData()));
    obu_sequence_header_->enable_restoration = true;
    obu_sequence_header_->use_128x128_superblock = use_128x128_superblock;
    ASSERT_TRUE(ObuParseLoopRestorationParameters());
    VerifyLoopRestorationParameters(gold);

    if (!use_128x128_superblock) {
      gold.unit_size_log2[0] = gold.unit_size_log2[1] = gold.unit_size_log2[2] =
          8;
      data.SetBit(7, 1);  // lr_unit_extra_shift.

      ASSERT_TRUE(Init(data.GenerateData()));
      obu_sequence_header_->enable_restoration = true;
      obu_sequence_header_->use_128x128_superblock = use_128x128_superblock;
      ASSERT_TRUE(ObuParseLoopRestorationParameters());
      VerifyLoopRestorationParameters(gold);
    }

    gold.unit_size_log2[1] = gold.unit_size_log2[2] = 7;
    data.AppendBit(1);  // lr_uv_shift.

    ASSERT_TRUE(Init(data.GenerateData()));
    obu_sequence_header_->enable_restoration = true;
    obu_sequence_header_->use_128x128_superblock = use_128x128_superblock;
    obu_sequence_header_->color_config.subsampling_x = 1;
    obu_sequence_header_->color_config.subsampling_y = 1;
    ASSERT_TRUE(ObuParseLoopRestorationParameters());
    VerifyLoopRestorationParameters(gold);
  }
}

TEST_F(ObuParserTest, TxModeSyntax) {
  BytesAndBits data;
  data.AppendBit(1);  // tx_mode_select.

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseTxModeSyntax());
  EXPECT_EQ(kTxModeSelect, obu_->frame_header().tx_mode);

  data.SetBit(0, 0);  // tx_mode_select.

  ASSERT_TRUE(Init(data.GenerateData()));
  ASSERT_TRUE(ObuParseTxModeSyntax());
  EXPECT_EQ(kTxModeLargest, obu_->frame_header().tx_mode);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->coded_lossless = true;
  ASSERT_TRUE(ObuParseTxModeSyntax());
  EXPECT_EQ(kTxModeOnly4x4, obu_->frame_header().tx_mode);
}

TEST_F(ObuParserTest, FrameReferenceModeSyntax) {
  BytesAndBits data;
  data.AppendBit(0);  // dummy.

  ASSERT_TRUE(ParseFrameReferenceModeSyntax(data.GenerateData(), kFrameKey));
  EXPECT_FALSE(obu_->frame_header().reference_mode_select);

  data.SetBit(0, 1);  // reference_mode_select.

  ASSERT_TRUE(ParseFrameReferenceModeSyntax(data.GenerateData(), kFrameInter));
  EXPECT_TRUE(obu_->frame_header().reference_mode_select);
}

TEST_F(ObuParserTest, SkipModeParameters) {
  BytesAndBits data;
  data.AppendBit(1);  // skip_mode_present.

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameKey;
  ASSERT_FALSE(ObuIsSkipModeAllowed());
  ASSERT_TRUE(ObuParseSkipModeParameters());
  EXPECT_FALSE(obu_->frame_header().skip_mode_present);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameInter;
  obu_frame_header_->reference_mode_select = true;
  ASSERT_FALSE(ObuIsSkipModeAllowed());
  ASSERT_TRUE(ObuParseSkipModeParameters());
  EXPECT_FALSE(obu_->frame_header().skip_mode_present);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameInter;
  obu_frame_header_->reference_mode_select = true;
  obu_sequence_header_->enable_order_hint = true;
  obu_sequence_header_->order_hint_bits = 7;
  obu_sequence_header_->order_hint_shift_bits =
      Mod32(32 - obu_sequence_header_->order_hint_bits);
  ASSERT_FALSE(ObuIsSkipModeAllowed());
  ASSERT_TRUE(ObuParseSkipModeParameters());
  EXPECT_FALSE(obu_->frame_header().skip_mode_present);

  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameInter;
  obu_frame_header_->reference_mode_select = true;
  obu_frame_header_->order_hint = 1;
  decoder_state_.order_hint = 1;
  obu_sequence_header_->enable_order_hint = true;
  obu_sequence_header_->order_hint_bits = 7;
  obu_sequence_header_->order_hint_shift_bits =
      Mod32(32 - obu_sequence_header_->order_hint_bits);
  ASSERT_FALSE(ObuIsSkipModeAllowed());
  ASSERT_TRUE(ObuParseSkipModeParameters());
  EXPECT_FALSE(obu_->frame_header().skip_mode_present);

  ASSERT_TRUE(Init(data.GenerateData()));
  for (int i = 0; i < kNumInterReferenceFrameTypes; ++i) {
    obu_frame_header_->reference_frame_index[i] = i;
    decoder_state_.reference_order_hint[i] = i;
  }
  obu_frame_header_->frame_type = kFrameInter;
  obu_frame_header_->reference_mode_select = true;
  obu_frame_header_->order_hint = 1;
  decoder_state_.order_hint = 1;
  obu_sequence_header_->enable_order_hint = true;
  obu_sequence_header_->order_hint_bits = 7;
  obu_sequence_header_->order_hint_shift_bits =
      Mod32(32 - obu_sequence_header_->order_hint_bits);
  ASSERT_TRUE(ObuIsSkipModeAllowed());
  ASSERT_TRUE(ObuParseSkipModeParameters());
  EXPECT_TRUE(obu_->frame_header().skip_mode_present);
}

TEST_F(ObuParserTest, AllowWarpedMotion) {
  BytesAndBits data;
  data.AppendBit(0xff);  // dummy.

  // IsIntraFrame is true, so nothing will be read.
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameKey;
  obu_frame_header_->error_resilient_mode = false;
  obu_sequence_header_->enable_warped_motion = true;
  ASSERT_TRUE(ObuReadAllowWarpedMotion());
  EXPECT_FALSE(obu_->frame_header().allow_warped_motion);

  // error_resilient_mode is true, so nothing will be read.
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameInter;
  obu_frame_header_->error_resilient_mode = true;
  obu_sequence_header_->enable_warped_motion = true;
  ASSERT_TRUE(ObuReadAllowWarpedMotion());
  EXPECT_FALSE(obu_->frame_header().allow_warped_motion);

  // enable_warped_motion is false, so nothing will be read.
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameInter;
  obu_frame_header_->error_resilient_mode = false;
  obu_sequence_header_->enable_warped_motion = false;
  ASSERT_TRUE(ObuReadAllowWarpedMotion());
  EXPECT_FALSE(obu_->frame_header().allow_warped_motion);

  // allow_warped_motion will be read and equal to true.
  ASSERT_TRUE(Init(data.GenerateData()));
  obu_frame_header_->frame_type = kFrameInter;
  obu_frame_header_->error_resilient_mode = false;
  obu_sequence_header_->enable_warped_motion = true;
  ASSERT_TRUE(ObuReadAllowWarpedMotion());
  EXPECT_TRUE(obu_->frame_header().allow_warped_motion);
}

TEST_F(ObuParserTest, GlobalMotionParameters) {
  BytesAndBits data;
  data.AppendBit(0);  // dummy.
  std::array<GlobalMotion, kNumReferenceFrameTypes> gold;
  for (int i = kReferenceFrameLast; i <= kReferenceFrameAlternate; ++i) {
    gold[i].type = kGlobalMotionTransformationTypeIdentity;
    for (int j = 0; j < 6; ++j) {
      gold[i].params[j] = (j % 3 == 2) ? 1 << kWarpedModelPrecisionBits : 0;
    }
  }

  ASSERT_TRUE(ParseGlobalMotionParameters(data.GenerateData(), kFrameKey));
  VerifyGlobalMotionParameters(gold);

  data.Clear();
  for (int i = kReferenceFrameLast; i <= kReferenceFrameAlternate; ++i) {
    // is_global=1; is_rot_zoom=1; parameter_values;
    data.AppendBytes(kDefaultGlobalMotionParametersRotZoom);

    // Magic numbers based on kDefaultGlobalMotionParametersRotZoom.
    gold[i].type = kGlobalMotionTransformationTypeRotZoom;
    gold[i].params[0] = -73728;
    gold[i].params[1] = -23552;
    gold[i].params[2] = 65952;
    gold[i].params[3] = -62;
    gold[i].params[4] = 62;
    gold[i].params[5] = 65952;
  }

  ASSERT_TRUE(ParseGlobalMotionParameters(data.GenerateData(), kFrameInter));
  VerifyGlobalMotionParameters(gold);

  data.Clear();
  for (int i = kReferenceFrameLast; i <= kReferenceFrameAlternate; ++i) {
    // This bit is not part of the hex string because it would make the whole
    // string not align to 8 bits. Appending this separately so that we can keep
    // the rest of them a magic hex string.
    data.AppendBit(1);  // is_global.
    // is_rot_zoom=0; is_translation=0; parameter_values;
    data.AppendBytes(kDefaultGlobalMotionParametersAffine);

    // Magic numbers based on kDefaultGlobalMotionParametersAffine.
    gold[i].type = kGlobalMotionTransformationTypeAffine;
    gold[i].params[4] = -62;
  }

  ASSERT_TRUE(ParseGlobalMotionParameters(data.GenerateData(), kFrameInter));
  VerifyGlobalMotionParameters(gold);
}

TEST_F(ObuParserTest, FilmGrainParameters) {
  BytesAndBits data;
  data.AppendBit(0);  // dummy.

  // Test film grain not present.
  FilmGrainParams gold = {};
  ObuSequenceHeader sequence_header = {};
  sequence_header.film_grain_params_present = false;
  ObuFrameHeader frame_header = {};
  ASSERT_TRUE(ParseFilmGrainParameters(data.GenerateData(), sequence_header,
                                       frame_header));
  VerifyFilmGrainParameters(gold);

  // Test if show_frame = false and showable_frame = false.
  data.Clear();
  gold = {};
  sequence_header.film_grain_params_present = true;
  frame_header.show_frame = false;
  frame_header.showable_frame = false;
  ASSERT_TRUE(ParseFilmGrainParameters(data.GenerateData(), sequence_header,
                                       frame_header));
  VerifyFilmGrainParameters(gold);

  // Test if apply_grain = false.
  data.Clear();
  gold = {};
  sequence_header.film_grain_params_present = true;
  frame_header.show_frame = true;
  frame_header.showable_frame = true;
  data.AppendBit(0);
  ASSERT_TRUE(ParseFilmGrainParameters(data.GenerateData(), sequence_header,
                                       frame_header));
  VerifyFilmGrainParameters(gold);

  // Test if update_grain = false.
  data.Clear();
  gold = {};
  sequence_header.film_grain_params_present = true;
  frame_header.show_frame = true;
  frame_header.showable_frame = true;
  frame_header.frame_type = kFrameInter;
  for (auto& index : frame_header.reference_frame_index) {
    index = 1;
  }
  data.AppendBit(1);
  gold.apply_grain = true;
  data.AppendLiteral(16, 8);
  gold.grain_seed = 8;
  data.AppendBit(0);
  gold.update_grain = false;
  data.AppendLiteral(3, 1);
  gold.reference_index = 1;
  // Set up decoder_state_ with a previous frame containing saved film grain
  // parameters.
  decoder_state_.reference_frame[1] = buffer_pool_->GetFreeBuffer();
  EXPECT_NE(decoder_state_.reference_frame[1], nullptr);
  FilmGrainParams prev_grain_params = {};
  prev_grain_params.apply_grain = true;
  prev_grain_params.grain_seed = 11;
  prev_grain_params.update_grain = true;
  decoder_state_.reference_frame[1]->set_film_grain_params(prev_grain_params);
  ASSERT_TRUE(ParseFilmGrainParameters(data.GenerateData(), sequence_header,
                                       frame_header));
  VerifyFilmGrainParameters(gold);

  // Test if update_grain = true, is_monochrome = true;
  data.Clear();
  gold = {};
  frame_header.frame_type = kFrameKey;
  for (auto& index : frame_header.reference_frame_index) {
    index = 0;
  }
  data.AppendBit(1);
  gold.apply_grain = true;
  data.AppendLiteral(16, 8);
  gold.grain_seed = 8;
  gold.update_grain = true;
  data.AppendLiteral(4, 10);
  gold.num_y_points = 10;
  for (int i = 0; i < gold.num_y_points; ++i) {
    data.AppendLiteral(8, 2 * i);
    gold.point_y_value[i] = 2 * i;
    data.AppendLiteral(8, i);
    gold.point_y_scaling[i] = i;
  }
  sequence_header.color_config.is_monochrome = true;
  gold.chroma_scaling_from_luma = false;
  gold.num_u_points = 0;
  gold.num_v_points = 0;
  data.AppendLiteral(2, 3);
  gold.chroma_scaling = 11;
  data.AppendLiteral(2, 1);
  gold.auto_regression_coeff_lag = 1;
  const int num_pos_luma =
      2 * gold.auto_regression_coeff_lag * (gold.auto_regression_coeff_lag + 1);
  for (int i = 0; i < num_pos_luma; ++i) {
    data.AppendLiteral(8, i + 128);
    gold.auto_regression_coeff_y[i] = i;
  }
  data.AppendLiteral(2, 0);
  gold.auto_regression_shift = 6;
  data.AppendLiteral(2, 1);
  gold.grain_scale_shift = 1;
  data.AppendBit(1);
  gold.overlap_flag = true;
  data.AppendBit(0);
  gold.clip_to_restricted_range = false;
  ASSERT_TRUE(ParseFilmGrainParameters(data.GenerateData(), sequence_header,
                                       frame_header));
  ASSERT_TRUE(
      obu_->frame_header().frame_type == kFrameInter ||
      obu_->frame_header().film_grain_params.update_grain);  // a implies b.
  VerifyFilmGrainParameters(gold);

  // Test if update_grain = true, is_monochrome = false;
  data.Clear();
  gold = {};
  frame_header.frame_type = kFrameKey;
  data.AppendBit(1);
  gold.apply_grain = true;
  data.AppendLiteral(16, 8);
  gold.grain_seed = 8;
  gold.update_grain = true;
  data.AppendLiteral(4, 10);
  gold.num_y_points = 10;
  for (int i = 0; i < gold.num_y_points; ++i) {
    data.AppendLiteral(8, 2 * i);
    gold.point_y_value[i] = 2 * i;
    data.AppendLiteral(8, i);
    gold.point_y_scaling[i] = i;
  }
  sequence_header.color_config.is_monochrome = false;
  data.AppendBit(0);
  gold.chroma_scaling_from_luma = false;
  data.AppendLiteral(4, 5);
  gold.num_u_points = 5;
  for (int i = 0; i < gold.num_u_points; ++i) {
    data.AppendLiteral(8, 2 * i + 1);
    gold.point_u_value[i] = 2 * i + 1;
    data.AppendLiteral(8, i);
    gold.point_u_scaling[i] = i;
  }
  data.AppendLiteral(4, 3);
  gold.num_v_points = 3;
  for (int i = 0; i < gold.num_v_points; ++i) {
    data.AppendLiteral(8, i);
    gold.point_v_value[i] = i;
    data.AppendLiteral(8, i + 1);
    gold.point_v_scaling[i] = i + 1;
  }
  data.AppendLiteral(2, 3);
  gold.chroma_scaling = 11;
  data.AppendLiteral(2, 1);
  gold.auto_regression_coeff_lag = 1;
  const int num_pos_luma2 =
      2 * gold.auto_regression_coeff_lag * (gold.auto_regression_coeff_lag + 1);
  for (int i = 0; i < num_pos_luma2; ++i) {
    data.AppendLiteral(8, i + 128);
    gold.auto_regression_coeff_y[i] = i;
  }
  for (int i = 0; i < num_pos_luma2 + 1; ++i) {
    data.AppendLiteral(8, i);
    gold.auto_regression_coeff_u[i] = i - 128;
  }
  for (int i = 0; i < num_pos_luma2 + 1; ++i) {
    data.AppendLiteral(8, i);
    gold.auto_regression_coeff_v[i] = i - 128;
  }
  data.AppendLiteral(2, 0);
  gold.auto_regression_shift = 6;
  data.AppendLiteral(2, 1);
  gold.grain_scale_shift = 1;
  data.AppendLiteral(8, 2);
  gold.u_multiplier = -126;
  data.AppendLiteral(8, 1);
  gold.u_luma_multiplier = -127;
  data.AppendLiteral(9, 3);
  gold.u_offset = -253;
  data.AppendLiteral(8, 3);
  gold.v_multiplier = -125;
  data.AppendLiteral(8, 2);
  gold.v_luma_multiplier = -126;
  data.AppendLiteral(9, 1);
  gold.v_offset = -255;
  data.AppendBit(1);
  gold.overlap_flag = true;
  data.AppendBit(0);
  gold.clip_to_restricted_range = false;
  ASSERT_TRUE(ParseFilmGrainParameters(data.GenerateData(), sequence_header,
                                       frame_header));
  ASSERT_TRUE(
      obu_->frame_header().frame_type == kFrameInter ||
      obu_->frame_header().film_grain_params.update_grain);  // a implies b.
  VerifyFilmGrainParameters(gold);
}

TEST_F(ObuParserTest, TileInfoSyntax) {
  BytesAndBits data;
  TileInfo gold;
  memset(&gold, 0, sizeof(gold));

  gold.uniform_spacing = true;
  gold.tile_columns_log2 = 1;
  gold.tile_columns = 2;
  gold.tile_rows_log2 = 1;
  gold.tile_rows = 2;
  gold.tile_count = 4;
  gold.tile_column_start[1] = 64;
  gold.tile_column_start[2] = 88;
  gold.tile_row_start[1] = 64;
  gold.tile_row_start[2] = 72;
  gold.context_update_id = 3;
  gold.tile_size_bytes = 4;
  data.AppendBit(static_cast<uint8_t>(gold.uniform_spacing));
  data.AppendBit(1);  // increment_tile_cols_log2.
  data.AppendBit(0);  // increment_tile_cols_log2.
  data.AppendBit(1);  // increment_tile_rows_log2.
  data.AppendBit(0);  // increment_tile_rows_log2.
  data.AppendBit(1);  // context update id, columns_log2+rows_log2 bits
  data.AppendBit(1);
  data.AppendLiteral(2, gold.tile_size_bytes - 1);

  ASSERT_TRUE(ParseTileInfoSyntax(data.GenerateData(), 88, 72, true));
  VerifyTileInfoParameters(gold);

  gold.uniform_spacing = false;
  gold.tile_column_width_in_superblocks[0] = 2;
  gold.tile_column_width_in_superblocks[1] = 1;
  gold.tile_row_height_in_superblocks[0] = 2;
  gold.tile_row_height_in_superblocks[1] = 1;

  data.SetBit(0, static_cast<uint8_t>(gold.uniform_spacing));
  // The next 4 bits remain the same except now they represent f(w - 1) and
  // extra_bit in DecodeUniform. All the subsequent bits are unchanged the
  // represent the same thing as above.

  ASSERT_TRUE(ParseTileInfoSyntax(data.GenerateData(), 88, 72, true));
  VerifyTileInfoParameters(gold);

  // No tiles.
  memset(&gold, 0, sizeof(gold));
  gold.uniform_spacing = true;
  gold.tile_columns = 1;
  gold.tile_rows = 1;
  gold.tile_count = 1;
  gold.tile_column_start[1] = 88;
  gold.tile_row_start[1] = 72;
  data.Clear();
  data.AppendBit(static_cast<uint8_t>(gold.uniform_spacing));
  data.AppendBit(0);  // tile_cols_log2.
  data.AppendBit(0);  // tile_rows_log2.

  ASSERT_TRUE(ParseTileInfoSyntax(data.GenerateData(), 88, 72, true));
  VerifyTileInfoParameters(gold);

  // 64x64 superblocks. No tiles.
  gold.tile_column_start[1] = 640;
  gold.tile_row_start[1] = 360;

  ASSERT_TRUE(ParseTileInfoSyntax(data.GenerateData(), 640, 360, false));
  VerifyTileInfoParameters(gold);
}

TEST_F(ObuParserTest, MetadataUnknownType) {
  BytesAndBits data;
  // The metadata_type 10 is a user private value (6-31).
  data.AppendLiteral(8, 10);  // metadata_type.
  // The Note in Section 5.8.1 says "Decoders should ignore the entire OBU if
  // they do not understand the metadata_type."
  ASSERT_TRUE(ParseMetadata(data.GenerateData()));
}

TEST_F(ObuParserTest, MetadataHdrCll) {
  BytesAndBits data;
  ObuMetadataHdrCll gold;
  gold.max_cll = 25;
  gold.max_fall = 100;

  data.AppendLiteral(8, kMetadataTypeHdrContentLightLevel);
  data.AppendLiteral(16, gold.max_cll);
  data.AppendLiteral(16, gold.max_fall);

  ASSERT_TRUE(ParseMetadata(data.GenerateData()));
  VerifyMetadataHdrCll(gold);
}

TEST_F(ObuParserTest, MetadataHdrMdcv) {
  BytesAndBits data;
  ObuMetadataHdrMdcv gold;
  for (int i = 0; i < 3; ++i) {
    gold.primary_chromaticity_x[i] = 0;
    gold.primary_chromaticity_y[i] = 0;
  }
  gold.white_point_chromaticity_x = 250;
  gold.white_point_chromaticity_y = 2500;
  gold.luminance_max = 6000;
  gold.luminance_min = 3000;

  data.AppendLiteral(8, kMetadataTypeHdrMasteringDisplayColorVolume);
  for (int i = 0; i < 3; ++i) {
    data.AppendLiteral(16, gold.primary_chromaticity_x[i]);
    data.AppendLiteral(16, gold.primary_chromaticity_y[i]);
  }
  data.AppendLiteral(16, gold.white_point_chromaticity_x);
  data.AppendLiteral(16, gold.white_point_chromaticity_y);
  data.AppendLiteral(32, gold.luminance_max);
  data.AppendLiteral(32, gold.luminance_min);

  ASSERT_TRUE(ParseMetadata(data.GenerateData()));
  VerifyMetadataHdrMdcv(gold);
}

TEST_F(ObuParserTest, MetadataScalability) {
  BytesAndBits data;

  data.AppendLiteral(8, kMetadataTypeScalability);
  data.AppendLiteral(8, 0);  // scalability_mode_idc

  ASSERT_TRUE(ParseMetadata(data.GenerateData()));
}

TEST_F(ObuParserTest, MetadataItutT35) {
  BytesAndBits data;
  ObuMetadataItutT35 gold;
  gold.country_code = 0xA6;  // 1 0 1 0 0 1 1 0 Switzerland
  DynamicBuffer<uint8_t> payload_bytes;
  ASSERT_TRUE(payload_bytes.Resize(10));
  gold.payload_bytes = payload_bytes.get();
  for (int i = 0; i < 10; ++i) {
    gold.payload_bytes[i] = 9 - i;
  }
  gold.payload_size = 10;

  data.AppendLiteral(8, kMetadataTypeItutT35);
  data.AppendLiteral(8, gold.country_code);
  for (int i = 0; i < 10; ++i) {
    data.AppendLiteral(8, 9 - i);
  }
  // For the kMetadataTypeItutT35 metadata type, we must include the trailing
  // bit so that the end of the itu_t_t35_payload_bytes can be identified.
  data.AppendLiteral(8, 0x80);
  data.AppendLiteral(8, 0x00);
  data.AppendLiteral(8, 0x00);

  ASSERT_TRUE(ParseMetadata(data.GenerateData()));
  VerifyMetadataItutT35(gold);

  gold.country_code = 0xFF;
  gold.country_code_extension_byte = 10;

  data.SetLiteral(8, 8, gold.country_code);
  data.InsertLiteral(16, 8, gold.country_code_extension_byte);

  ASSERT_TRUE(ParseMetadata(data.GenerateData()));
  VerifyMetadataItutT35(gold);
}

TEST_F(ObuParserTest, MetadataTimecode) {
  BytesAndBits data;

  data.AppendLiteral(8, kMetadataTypeTimecode);
  data.AppendLiteral(5, 0);   // counting_type
  data.AppendBit(1);          // full_timestamp_flag
  data.AppendBit(0);          // discontinuity_flag
  data.AppendBit(0);          // cnt_dropped_flag
  data.AppendLiteral(9, 8);   // n_frames
  data.AppendLiteral(6, 59);  // seconds_value
  data.AppendLiteral(6, 59);  // minutes_value
  data.AppendLiteral(5, 23);  // hours_value
  data.AppendLiteral(5, 0);   // time_offset_length

  ASSERT_TRUE(ParseMetadata(data.GenerateData()));
}

TEST_F(ObuParserTest, MetadataTimecodeInvalidSecondsValue) {
  BytesAndBits data;

  data.AppendLiteral(8, kMetadataTypeTimecode);
  data.AppendLiteral(5, 0);   // counting_type
  data.AppendBit(1);          // full_timestamp_flag
  data.AppendBit(0);          // discontinuity_flag
  data.AppendBit(0);          // cnt_dropped_flag
  data.AppendLiteral(9, 8);   // n_frames
  data.AppendLiteral(6, 60);  // seconds_value
  data.AppendLiteral(6, 59);  // minutes_value
  data.AppendLiteral(5, 23);  // hours_value
  data.AppendLiteral(5, 0);   // time_offset_length

  EXPECT_FALSE(ParseMetadata(data.GenerateData()));
}

TEST_F(ObuParserTest, MetadataTimecodeInvalidMinutesValue) {
  BytesAndBits data;

  data.AppendLiteral(8, kMetadataTypeTimecode);
  data.AppendLiteral(5, 0);   // counting_type
  data.AppendBit(1);          // full_timestamp_flag
  data.AppendBit(0);          // discontinuity_flag
  data.AppendBit(0);          // cnt_dropped_flag
  data.AppendLiteral(9, 8);   // n_frames
  data.AppendLiteral(6, 59);  // seconds_value
  data.AppendLiteral(6, 60);  // minutes_value
  data.AppendLiteral(5, 23);  // hours_value
  data.AppendLiteral(5, 0);   // time_offset_length

  EXPECT_FALSE(ParseMetadata(data.GenerateData()));
}

TEST_F(ObuParserTest, MetadataTimecodeInvalidHoursValue) {
  BytesAndBits data;

  data.AppendLiteral(8, kMetadataTypeTimecode);
  data.AppendLiteral(5, 0);   // counting_type
  data.AppendBit(1);          // full_timestamp_flag
  data.AppendBit(0);          // discontinuity_flag
  data.AppendBit(0);          // cnt_dropped_flag
  data.AppendLiteral(9, 8);   // n_frames
  data.AppendLiteral(6, 59);  // seconds_value
  data.AppendLiteral(6, 59);  // minutes_value
  data.AppendLiteral(5, 24);  // hours_value
  data.AppendLiteral(5, 0);   // time_offset_length

  EXPECT_FALSE(ParseMetadata(data.GenerateData()));
}

}  // namespace libgav1
