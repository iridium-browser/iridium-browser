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

#ifndef AOM_AV1_REFERENCE_MANAGER_H_
#define AOM_AV1_REFERENCE_MANAGER_H_

#include <deque>
#include <iostream>
#include <vector>

#include "av1/ratectrl_qmode_interface.h"

namespace aom {

enum class RefUpdateType { kForward, kBackward, kLast, kNone };

class RefFrameManager {
 public:
  explicit RefFrameManager(int max_ref_frames)
      : max_ref_frames_(max_ref_frames) {
    forward_max_size_ = max_ref_frames - 2;
    Reset();
  }
  ~RefFrameManager() = default;

  RefFrameManager(const RefFrameManager &) = delete;
  RefFrameManager &operator=(const RefFrameManager &) = delete;

  friend std::ostream &operator<<(std::ostream &os,
                                  const RefFrameManager &rfm) {
    os << "=" << std::endl;
    os << "forward: ";
    for (const auto &ref_idx : rfm.forward_stack_) {
      os << rfm.ref_frame_table_[ref_idx].order_idx << " ";
    }
    os << std::endl;
    os << "backward: ";
    for (const auto &ref_idx : rfm.backward_queue_) {
      os << rfm.ref_frame_table_[ref_idx].order_idx << " ";
    }
    os << std::endl;
    os << "last: ";
    for (const auto &ref_idx : rfm.last_queue_) {
      os << rfm.ref_frame_table_[ref_idx].order_idx << " ";
    }
    os << std::endl;
    return os;
  }

  void Reset();
  int AllocateRefIdx();
  void UpdateOrder(int order_idx);
  int ColocatedRefIdx(int order_idx);
  int ForwardMaxSize() const { return forward_max_size_; }
  int MaxRefFrames() const { return max_ref_frames_; }
  void UpdateFrame(GopFrame *gop_frame, RefUpdateType ref_update_type,
                   EncodeRefMode encode_ref_mode);

 private:
  // TODO(angiebird): // Make RefFrameTable comply with max_ref_frames_
  int max_ref_frames_;
  int forward_max_size_;
  RefFrameTable ref_frame_table_;
  std::deque<int> free_ref_idx_list_;
  std::vector<int> forward_stack_;
  std::deque<int> backward_queue_;
  std::deque<int> last_queue_;
};

}  // namespace aom

#endif  // AOM_AV1_REFERENCE_MANAGER_H_
