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

#include "src/loop_restoration_info.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>

#include "src/utils/common.h"
#include "src/utils/logging.h"

namespace libgav1 {
namespace {

// Controls how self guided deltas are read.
constexpr int kSgrProjReadControl = 4;
// Maps the restoration type encoded in the compressed headers (restoration_type
// element in the spec) of the bitstream to LoopRestorationType. This is used
// only when the restoration type in the frame header is
// LoopRestorationTypeSwitchable.
constexpr LoopRestorationType kBitstreamRestorationTypeMap[] = {
    kLoopRestorationTypeNone, kLoopRestorationTypeWiener,
    kLoopRestorationTypeSgrProj};

inline int CountLeadingZeroCoefficients(const int16_t* const filter) {
  int number_zero_coefficients = 0;
  if (filter[0] == 0) {
    number_zero_coefficients++;
    if (filter[1] == 0) {
      number_zero_coefficients++;
      if (filter[2] == 0) {
        number_zero_coefficients++;
      }
    }
  }
  return number_zero_coefficients;
}

}  // namespace

bool LoopRestorationInfo::Reset(const LoopRestoration* const loop_restoration,
                                uint32_t width, uint32_t height,
                                int8_t subsampling_x, int8_t subsampling_y,
                                bool is_monochrome) {
  loop_restoration_ = loop_restoration;
  subsampling_x_ = subsampling_x;
  subsampling_y_ = subsampling_y;

  const int num_planes = is_monochrome ? kMaxPlanesMonochrome : kMaxPlanes;
  int total_num_units = 0;
  for (int plane = kPlaneY; plane < num_planes; ++plane) {
    if (loop_restoration_->type[plane] == kLoopRestorationTypeNone) {
      plane_needs_filtering_[plane] = false;
      continue;
    }
    plane_needs_filtering_[plane] = true;
    const int plane_width =
        (plane == kPlaneY) ? width : SubsampledValue(width, subsampling_x_);
    const int plane_height =
        (plane == kPlaneY) ? height : SubsampledValue(height, subsampling_y_);
    num_horizontal_units_[plane] =
        std::max(1, RightShiftWithRounding(
                        plane_width, loop_restoration_->unit_size_log2[plane]));
    num_vertical_units_[plane] = std::max(
        1, RightShiftWithRounding(plane_height,
                                  loop_restoration_->unit_size_log2[plane]));
    num_units_[plane] =
        num_horizontal_units_[plane] * num_vertical_units_[plane];
    total_num_units += num_units_[plane];
  }
  // Allocate the RestorationUnitInfo arrays for all planes in a single heap
  // allocation and divide up the buffer into arrays of the right sizes.
  if (!loop_restoration_info_buffer_.Resize(total_num_units)) {
    return false;
  }
  RestorationUnitInfo* loop_restoration_info =
      loop_restoration_info_buffer_.get();
  for (int plane = kPlaneY; plane < num_planes; ++plane) {
    if (loop_restoration_->type[plane] == kLoopRestorationTypeNone) {
      continue;
    }
    loop_restoration_info_[plane] = loop_restoration_info;
    loop_restoration_info += num_units_[plane];
  }
  return true;
}

bool LoopRestorationInfo::PopulateUnitInfoForSuperBlock(
    Plane plane, BlockSize block_size, bool is_superres_scaled,
    uint8_t superres_scale_denominator, int row4x4, int column4x4,
    LoopRestorationUnitInfo* const unit_info) const {
  assert(unit_info != nullptr);
  if (!plane_needs_filtering_[plane]) return false;
  const int numerator_column =
      is_superres_scaled ? superres_scale_denominator : 1;
  const int pixel_column_start =
      RowOrColumn4x4ToPixel(column4x4, plane, subsampling_x_);
  const int pixel_column_end = RowOrColumn4x4ToPixel(
      column4x4 + kNum4x4BlocksWide[block_size], plane, subsampling_x_);
  const int unit_row_log2 = loop_restoration_->unit_size_log2[plane];
  const int denominator_column_log2 =
      unit_row_log2 + (is_superres_scaled ? 3 : 0);
  const int pixel_row_start =
      RowOrColumn4x4ToPixel(row4x4, plane, subsampling_y_);
  const int pixel_row_end = RowOrColumn4x4ToPixel(
      row4x4 + kNum4x4BlocksHigh[block_size], plane, subsampling_y_);
  unit_info->column_start = RightShiftWithCeiling(
      pixel_column_start * numerator_column, denominator_column_log2);
  unit_info->column_end = RightShiftWithCeiling(
      pixel_column_end * numerator_column, denominator_column_log2);
  unit_info->row_start = RightShiftWithCeiling(pixel_row_start, unit_row_log2);
  unit_info->row_end = RightShiftWithCeiling(pixel_row_end, unit_row_log2);
  unit_info->column_end =
      std::min(unit_info->column_end, num_horizontal_units_[plane]);
  unit_info->row_end = std::min(unit_info->row_end, num_vertical_units_[plane]);
  return true;
}

void LoopRestorationInfo::ReadUnitCoefficients(
    EntropyDecoder* const reader,
    SymbolDecoderContext* const symbol_decoder_context, Plane plane,
    int unit_id,
    std::array<RestorationUnitInfo, kMaxPlanes>* const reference_unit_info) {
  LoopRestorationType unit_restoration_type = kLoopRestorationTypeNone;
  if (loop_restoration_->type[plane] == kLoopRestorationTypeSwitchable) {
    unit_restoration_type = kBitstreamRestorationTypeMap
        [reader->ReadSymbol<kRestorationTypeSymbolCount>(
            symbol_decoder_context->restoration_type_cdf)];
  } else if (loop_restoration_->type[plane] == kLoopRestorationTypeWiener) {
    const bool use_wiener =
        reader->ReadSymbol(symbol_decoder_context->use_wiener_cdf);
    if (use_wiener) unit_restoration_type = kLoopRestorationTypeWiener;
  } else if (loop_restoration_->type[plane] == kLoopRestorationTypeSgrProj) {
    const bool use_sgrproj =
        reader->ReadSymbol(symbol_decoder_context->use_sgrproj_cdf);
    if (use_sgrproj) unit_restoration_type = kLoopRestorationTypeSgrProj;
  }
  loop_restoration_info_[plane][unit_id].type = unit_restoration_type;

  if (unit_restoration_type == kLoopRestorationTypeWiener) {
    ReadWienerInfo(reader, plane, unit_id, reference_unit_info);
  } else if (unit_restoration_type == kLoopRestorationTypeSgrProj) {
    ReadSgrProjInfo(reader, plane, unit_id, reference_unit_info);
  }
}

void LoopRestorationInfo::ReadWienerInfo(
    EntropyDecoder* const reader, Plane plane, int unit_id,
    std::array<RestorationUnitInfo, kMaxPlanes>* const reference_unit_info) {
  for (int i = WienerInfo::kVertical; i <= WienerInfo::kHorizontal; ++i) {
    if (plane != kPlaneY) {
      loop_restoration_info_[plane][unit_id].wiener_info.filter[i][0] = 0;
    }
    int sum = 0;
    for (int j = static_cast<int>(plane != kPlaneY); j < kNumWienerCoefficients;
         ++j) {
      const int8_t wiener_min = kWienerTapsMin[j];
      const int8_t wiener_max = kWienerTapsMax[j];
      const int control = j + 1;
      int value;
      if (!reader->DecodeSignedSubexpWithReference(
              wiener_min, wiener_max + 1,
              (*reference_unit_info)[plane].wiener_info.filter[i][j], control,
              &value)) {
        LIBGAV1_DLOG(
            ERROR,
            "Error decoding Wiener filter coefficients: plane %d, unit_id %d",
            static_cast<int>(plane), unit_id);
        return;
      }
      loop_restoration_info_[plane][unit_id].wiener_info.filter[i][j] = value;
      (*reference_unit_info)[plane].wiener_info.filter[i][j] = value;
      sum += value;
    }
    loop_restoration_info_[plane][unit_id].wiener_info.filter[i][3] =
        128 - 2 * sum;
    loop_restoration_info_[plane][unit_id]
        .wiener_info.number_leading_zero_coefficients[i] =
        CountLeadingZeroCoefficients(
            loop_restoration_info_[plane][unit_id].wiener_info.filter[i]);
  }
}

void LoopRestorationInfo::ReadSgrProjInfo(
    EntropyDecoder* const reader, Plane plane, int unit_id,
    std::array<RestorationUnitInfo, kMaxPlanes>* const reference_unit_info) {
  const int sgr_proj_index =
      static_cast<int>(reader->ReadLiteral(kSgrProjParamsBits));
  loop_restoration_info_[plane][unit_id].sgr_proj_info.index = sgr_proj_index;
  for (int i = 0; i < 2; ++i) {
    const uint8_t radius = kSgrProjParams[sgr_proj_index][i * 2];
    const int8_t multiplier_min = kSgrProjMultiplierMin[i];
    const int8_t multiplier_max = kSgrProjMultiplierMax[i];
    int multiplier;
    if (radius != 0) {
      if (!reader->DecodeSignedSubexpWithReference(
              multiplier_min, multiplier_max + 1,
              (*reference_unit_info)[plane].sgr_proj_info.multiplier[i],
              kSgrProjReadControl, &multiplier)) {
        LIBGAV1_DLOG(ERROR,
                     "Error decoding Self-guided filter coefficients: plane "
                     "%d, unit_id %d",
                     static_cast<int>(plane), unit_id);
        return;
      }
    } else {
      // The range of (*reference_unit_info)[plane].sgr_proj_info.multiplier[0]
      // from DecodeSignedSubexpWithReference() is [-96, 31], the default is
      // -32, making Clip3(128 - 31, -32, 95) unnecessary.
      static constexpr int kMultiplier[2] = {0, 95};
      multiplier = kMultiplier[i];
      assert(
          i == 0 ||
          Clip3((1 << kSgrProjPrecisionBits) -
                    (*reference_unit_info)[plane].sgr_proj_info.multiplier[0],
                multiplier_min, multiplier_max) == kMultiplier[1]);
    }
    loop_restoration_info_[plane][unit_id].sgr_proj_info.multiplier[i] =
        multiplier;
    (*reference_unit_info)[plane].sgr_proj_info.multiplier[i] = multiplier;
  }
}

}  // namespace libgav1
