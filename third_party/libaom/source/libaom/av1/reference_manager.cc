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

#include "av1/reference_manager.h"

namespace aom {

void RefFrameManager::Reset() {
  free_ref_idx_list_.clear();
  for (int i = 0; i < kRefFrameTableSize; ++i) {
    free_ref_idx_list_.push_back(i);
  }
  forward_stack_.clear();
  backward_queue_.clear();
  last_queue_.clear();
}

int RefFrameManager::AllocateRefIdx() {
  if (free_ref_idx_list_.empty()) {
    size_t backward_size = backward_queue_.size();
    size_t last_size = last_queue_.size();
    if (last_size >= backward_size) {
      int ref_idx = last_queue_.front();
      last_queue_.pop_front();
      free_ref_idx_list_.push_back(ref_idx);
    } else {
      int ref_idx = backward_queue_.front();
      backward_queue_.pop_front();
      free_ref_idx_list_.push_back(ref_idx);
    }
  }

  int ref_idx = free_ref_idx_list_.front();
  free_ref_idx_list_.pop_front();
  return ref_idx;
}

void RefFrameManager::UpdateOrder(int order_idx) {
  if (forward_stack_.empty()) {
    return;
  }
  int ref_idx = forward_stack_.back();
  const GopFrame &gf_frame = ref_frame_table_[ref_idx];
  if (gf_frame.order_idx <= order_idx) {
    forward_stack_.pop_back();
    if (gf_frame.is_golden_frame) {
      // high quality frame
      backward_queue_.push_back(ref_idx);
    } else {
      last_queue_.push_back(ref_idx);
    }
  }
}

int RefFrameManager::ColocatedRefIdx(int order_idx) {
  if (forward_stack_.size() == 0) return -1;
  int ref_idx = forward_stack_.back();
  int arf_order_idx = ref_frame_table_[ref_idx].order_idx;
  if (arf_order_idx == order_idx) {
    return ref_idx;
  }
  return -1;
}

void RefFrameManager::UpdateFrame(GopFrame *gop_frame,
                                  RefUpdateType ref_update_type,
                                  EncodeRefMode encode_ref_mode) {
  gop_frame->colocated_ref_idx = ColocatedRefIdx(gop_frame->order_idx);
  if (gop_frame->is_show_frame) {
    UpdateOrder(gop_frame->order_idx);
  }
  if (ref_update_type == RefUpdateType::kNone) {
    gop_frame->update_ref_idx = -1;
  } else {
    const int ref_idx = AllocateRefIdx();
    gop_frame->update_ref_idx = ref_idx;
    switch (ref_update_type) {
      case RefUpdateType::kForward: forward_stack_.push_back(ref_idx); break;
      case RefUpdateType::kBackward: backward_queue_.push_back(ref_idx); break;
      case RefUpdateType::kLast: last_queue_.push_back(ref_idx); break;
      case RefUpdateType::kNone: break;
    }
    ref_frame_table_[ref_idx] = *gop_frame;
  }
  gop_frame->encode_ref_mode = encode_ref_mode;
}

}  // namespace aom
