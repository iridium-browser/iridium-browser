// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_RECEIVER_LIST_H_
#define OSP_IMPL_RECEIVER_LIST_H_

#include <vector>

#include "osp/public/service_info.h"
#include "platform/base/error.h"

namespace openscreen {
namespace osp {

class ReceiverList {
 public:
  ReceiverList();
  ~ReceiverList();
  ReceiverList(ReceiverList&&) noexcept = delete;
  ReceiverList& operator=(ReceiverList&&) = delete;

  void OnReceiverAdded(const ServiceInfo& info);

  Error OnReceiverChanged(const ServiceInfo& info);
  // If successfully removed, returns the service info that was removed. If
  // `info` is a reference to an entry in `receivers`, it is immediately
  // invalid after calling this method.
  ErrorOr<ServiceInfo> OnReceiverRemoved(const ServiceInfo& info);
  Error OnAllReceiversRemoved();

  const std::vector<ServiceInfo>& receivers() const { return receivers_; }

 private:
  std::vector<ServiceInfo> receivers_;
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_IMPL_RECEIVER_LIST_H_
