// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/service_listener_impl.h"

#include <algorithm>
#include <utility>

#include "platform/base/error.h"
#include "util/osp_logging.h"
#include "util/std_util.h"

namespace openscreen {
namespace osp {
namespace {

bool IsTransitionValid(ServiceListener::State from, ServiceListener::State to) {
  switch (from) {
    case ServiceListener::State::kStopped:
      return to == ServiceListener::State::kStarting ||
             to == ServiceListener::State::kStopping;
    case ServiceListener::State::kStarting:
      return to == ServiceListener::State::kRunning ||
             to == ServiceListener::State::kStopping ||
             to == ServiceListener::State::kSuspended;
    case ServiceListener::State::kRunning:
      return to == ServiceListener::State::kSuspended ||
             to == ServiceListener::State::kSearching ||
             to == ServiceListener::State::kStopping;
    case ServiceListener::State::kStopping:
      return to == ServiceListener::State::kStopped;
    case ServiceListener::State::kSearching:
      return to == ServiceListener::State::kRunning ||
             to == ServiceListener::State::kSuspended ||
             to == ServiceListener::State::kStopping;
    case ServiceListener::State::kSuspended:
      return to == ServiceListener::State::kRunning ||
             to == ServiceListener::State::kSearching ||
             to == ServiceListener::State::kStopping;
    default:
      OSP_DCHECK(false) << "unknown ServiceListener::State value: "
                        << static_cast<int>(from);
      break;
  }
  return false;
}

}  // namespace

ServiceListenerImpl::Delegate::Delegate() = default;
ServiceListenerImpl::Delegate::~Delegate() = default;

void ServiceListenerImpl::Delegate::SetListenerImpl(
    ServiceListenerImpl* listener) {
  OSP_DCHECK(!listener_);
  listener_ = listener;
}

ServiceListenerImpl::ServiceListenerImpl(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {
  delegate_->SetListenerImpl(this);
}

ServiceListenerImpl::~ServiceListenerImpl() = default;

void ServiceListenerImpl::OnReceiverUpdated(
    const std::vector<ServiceInfo>& new_receivers) {
  // All receivers are removed.
  if (new_receivers.empty()) {
    OnAllReceiversRemoved();
  }

  const auto& old_receivers = GetReceivers();

  if (new_receivers.size() < old_receivers.size()) {
    // A receiver is removed.
    for (const auto& receiver : old_receivers) {
      if (Contains(new_receivers, receiver)) {
        continue;
      }
      OnReceiverRemoved(receiver);
      return;
    }
  } else {
    // A receiver is added or updated.
    for (const auto& receiver : new_receivers) {
      if (Contains(old_receivers, receiver)) {
        continue;
      }
      new_receivers.size() > old_receivers.size() ? OnReceiverAdded(receiver)
                                                  : OnReceiverChanged(receiver);
      return;
    }
  }
}

void ServiceListenerImpl::OnReceiverAdded(const ServiceInfo& info) {
  OSP_VLOG << __func__ << ": new receiver added=" << info.ToString();
  receiver_list_.OnReceiverAdded(info);
  for (auto* observer : observers_) {
    observer->OnReceiverAdded(info);
  }
}

void ServiceListenerImpl::OnReceiverChanged(const ServiceInfo& info) {
  OSP_VLOG << __func__ << ": receiver changed=" << info.ToString();
  const Error changed_error = receiver_list_.OnReceiverChanged(info);
  if (changed_error.ok()) {
    for (auto* observer : observers_) {
      observer->OnReceiverChanged(info);
    }
  }
}

void ServiceListenerImpl::OnReceiverRemoved(const ServiceInfo& info) {
  OSP_VLOG << __func__ << ": receiver removed=" << info.ToString();
  const ErrorOr<ServiceInfo> removed_or_error =
      receiver_list_.OnReceiverRemoved(info);
  if (removed_or_error.is_value()) {
    for (auto* observer : observers_) {
      observer->OnReceiverRemoved(removed_or_error.value());
    }
  }
}

void ServiceListenerImpl::OnAllReceiversRemoved() {
  OSP_VLOG << __func__ << ": all receivers removed.";
  const Error removed_all_error = receiver_list_.OnAllReceiversRemoved();
  if (removed_all_error.ok()) {
    for (auto* observer : observers_) {
      observer->OnAllReceiversRemoved();
    }
  }
}

void ServiceListenerImpl::OnError(Error error) {
  last_error_ = error;
  for (auto* observer : observers_) {
    observer->OnError(error);
  }
}

bool ServiceListenerImpl::Start() {
  if (state_ != State::kStopped)
    return false;
  state_ = State::kStarting;
  delegate_->StartListener(config_);
  return true;
}

bool ServiceListenerImpl::StartAndSuspend() {
  if (state_ != State::kStopped)
    return false;
  state_ = State::kStarting;
  delegate_->StartAndSuspendListener(config_);
  return true;
}

bool ServiceListenerImpl::Stop() {
  if (state_ == State::kStopped || state_ == State::kStopping)
    return false;

  state_ = State::kStopping;
  delegate_->StopListener();
  return true;
}

bool ServiceListenerImpl::Suspend() {
  if (state_ != State::kRunning && state_ != State::kSearching &&
      state_ != State::kStarting) {
    return false;
  }
  delegate_->SuspendListener();
  return true;
}

bool ServiceListenerImpl::Resume() {
  if (state_ != State::kSuspended && state_ != State::kSearching)
    return false;

  delegate_->ResumeListener();
  return true;
}

bool ServiceListenerImpl::SearchNow() {
  if (state_ != State::kRunning && state_ != State::kSuspended)
    return false;

  delegate_->SearchNow(state_);
  return true;
}

void ServiceListenerImpl::AddObserver(Observer* observer) {
  OSP_DCHECK(observer);
  observers_.push_back(observer);
}

void ServiceListenerImpl::RemoveObserver(Observer* observer) {
  // TODO(btolsch): Consider writing an ObserverList in base/ for things like
  // CHECK()ing that the list is empty on destruction.
  observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                   observers_.end());
}

const std::vector<ServiceInfo>& ServiceListenerImpl::GetReceivers() const {
  return receiver_list_.receivers();
}

void ServiceListenerImpl::OnFatalError(Error error) {
  OnError(error);
}

void ServiceListenerImpl::OnRecoverableError(Error error) {
  OnError(error);
}

void ServiceListenerImpl::SetState(State state) {
  OSP_DCHECK(IsTransitionValid(state_, state));
  state_ = state;
  if (!observers_.empty()) {
    MaybeNotifyObservers();
  }
}

void ServiceListenerImpl::MaybeNotifyObservers() {
  OSP_DCHECK(!observers_.empty());
  switch (state_) {
    case State::kRunning:
      for (auto* observer : observers_) {
        observer->OnStarted();
      }
      break;
    case State::kStopped:
      for (auto* observer : observers_) {
        observer->OnStopped();
      }
      break;
    case State::kSuspended:
      for (auto* observer : observers_) {
        observer->OnSuspended();
      }
      break;
    case State::kSearching:
      for (auto* observer : observers_) {
        observer->OnSearching();
      }
      break;
    default:
      break;
  }
}

}  // namespace osp
}  // namespace openscreen
