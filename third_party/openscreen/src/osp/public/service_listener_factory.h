// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_SERVICE_LISTENER_FACTORY_H_
#define OSP_PUBLIC_SERVICE_LISTENER_FACTORY_H_

#include <memory>

#include "osp/public/service_listener.h"

namespace openscreen {

class TaskRunner;

namespace osp {

class ServiceListenerFactory {
 public:
  static std::unique_ptr<ServiceListener> Create(
      const ServiceListener::Config& config,
      TaskRunner& task_runner);
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_PUBLIC_SERVICE_LISTENER_FACTORY_H_
