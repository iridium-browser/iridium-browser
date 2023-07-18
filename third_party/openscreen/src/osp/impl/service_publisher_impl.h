// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_SERVICE_PUBLISHER_IMPL_H_
#define OSP_IMPL_SERVICE_PUBLISHER_IMPL_H_

#include <memory>

#include "osp/impl/with_destruction_callback.h"
#include "osp/public/service_publisher.h"
#include "platform/base/macros.h"

namespace openscreen {
namespace osp {

class ServicePublisherImpl final : public ServicePublisher,
                                   public WithDestructionCallback {
 public:
  class Delegate {
   public:
    Delegate();
    virtual ~Delegate();

    void SetPublisherImpl(ServicePublisherImpl* publisher);

    virtual void StartPublisher(const ServicePublisher::Config& config) = 0;
    virtual void StartAndSuspendPublisher(
        const ServicePublisher::Config& config) = 0;
    virtual void StopPublisher() = 0;
    virtual void SuspendPublisher() = 0;
    virtual void ResumePublisher(const ServicePublisher::Config& config) = 0;

   protected:
    void SetState(State state) { publisher_->SetState(state); }

    ServicePublisherImpl* publisher_ = nullptr;
  };

  // |observer| is optional.  If it is provided, it will receive appropriate
  // notifications about this ServicePublisher.  |delegate| is required and
  // is used to implement state transitions.
  ServicePublisherImpl(Observer* observer, std::unique_ptr<Delegate> delegate);
  ~ServicePublisherImpl() override;

  // ServicePublisher overrides.
  bool Start() override;
  bool StartAndSuspend() override;
  bool Stop() override;
  bool Suspend() override;
  bool Resume() override;

 private:
  // Called by |delegate_| to transition the state machine (except kStarting and
  // kStopping which are done automatically).
  void SetState(State state);

  // Notifies |observer_| if the transition to |state_| is one that is watched
  // by the observer interface.
  void MaybeNotifyObserver();

  std::unique_ptr<Delegate> delegate_;

  OSP_DISALLOW_COPY_AND_ASSIGN(ServicePublisherImpl);
};

}  // namespace osp
}  // namespace openscreen

#endif  // OSP_IMPL_SERVICE_PUBLISHER_IMPL_H_
