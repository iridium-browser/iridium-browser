// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/call_setup_state_tracker.h"

namespace blink {

namespace {

bool IsRejectedOffererState(OffererState state) {
  return state == OffererState::kCreateOfferRejected ||
         state == OffererState::kSetLocalOfferRejected ||
         state == OffererState::kSetRemoteAnswerRejected;
}

bool IsRejectedAnswererState(AnswererState state) {
  return state == AnswererState::kSetRemoteOfferRejected ||
         state == AnswererState::kCreateAnswerRejected ||
         state == AnswererState::kSetLocalAnswerRejected;
}

}  // namespace

CallSetupStateTracker::CallSetupStateTracker()
    : valid_offerer_transitions_(
          {std::make_pair(OffererState::kNotStarted,
                          OffererState::kCreateOfferPending),
           // createOffer()
           std::make_pair(OffererState::kCreateOfferPending,
                          OffererState::kCreateOfferRejected),
           std::make_pair(OffererState::kCreateOfferPending,
                          OffererState::kCreateOfferResolved),
           std::make_pair(OffererState::kCreateOfferRejected,
                          OffererState::kCreateOfferResolved),
           std::make_pair(OffererState::kCreateOfferResolved,
                          OffererState::kSetLocalOfferPending),
           // setLocalDescription(offer)
           std::make_pair(OffererState::kSetLocalOfferPending,
                          OffererState::kSetLocalOfferRejected),
           std::make_pair(OffererState::kSetLocalOfferPending,
                          OffererState::kSetLocalOfferResolved),
           std::make_pair(OffererState::kSetLocalOfferRejected,
                          OffererState::kSetLocalOfferResolved),
           std::make_pair(OffererState::kSetLocalOfferResolved,
                          OffererState::kSetRemoteAnswerPending),
           // setRemoteDescription(answer)
           std::make_pair(OffererState::kSetRemoteAnswerPending,
                          OffererState::kSetRemoteAnswerRejected),
           std::make_pair(OffererState::kSetRemoteAnswerPending,
                          OffererState::kSetRemoteAnswerResolved),
           std::make_pair(OffererState::kSetRemoteAnswerRejected,
                          OffererState::kSetRemoteAnswerResolved)}),
      valid_answerer_transitions_({
          std::make_pair(AnswererState::kNotStarted,
                         AnswererState::kSetRemoteOfferPending),
          // setRemoteDescription(answer)
          std::make_pair(AnswererState::kSetRemoteOfferPending,
                         AnswererState::kSetRemoteOfferRejected),
          std::make_pair(AnswererState::kSetRemoteOfferPending,
                         AnswererState::kSetRemoteOfferResolved),
          std::make_pair(AnswererState::kSetRemoteOfferRejected,
                         AnswererState::kSetRemoteOfferResolved),
          std::make_pair(AnswererState::kSetRemoteOfferResolved,
                         AnswererState::kCreateAnswerPending),
          // createAnswer()
          std::make_pair(AnswererState::kCreateAnswerPending,
                         AnswererState::kCreateAnswerRejected),
          std::make_pair(AnswererState::kCreateAnswerPending,
                         AnswererState::kCreateAnswerResolved),
          std::make_pair(AnswererState::kCreateAnswerRejected,
                         AnswererState::kCreateAnswerResolved),
          std::make_pair(AnswererState::kCreateAnswerResolved,
                         AnswererState::kSetLocalAnswerPending),
          // setLocalDescription(answer)
          std::make_pair(AnswererState::kSetLocalAnswerPending,
                         AnswererState::kSetLocalAnswerRejected),
          std::make_pair(AnswererState::kSetLocalAnswerPending,
                         AnswererState::kSetLocalAnswerResolved),
          std::make_pair(AnswererState::kSetLocalAnswerRejected,
                         AnswererState::kSetLocalAnswerResolved),
      }),
      offerer_state_(OffererState::kNotStarted),
      answerer_state_(AnswererState::kNotStarted),
      has_ever_failed_(false),
      document_uses_media_(false) {}

OffererState CallSetupStateTracker::offerer_state() const {
  return offerer_state_;
}

AnswererState CallSetupStateTracker::answerer_state() const {
  return answerer_state_;
}

bool CallSetupStateTracker::document_uses_media() const {
  return document_uses_media_;
}

CallSetupState CallSetupStateTracker::GetCallSetupState() const {
  if (offerer_state_ == OffererState::kNotStarted &&
      answerer_state_ == AnswererState::kNotStarted) {
    return CallSetupState::kNotStarted;
  }
  if (offerer_state_ == OffererState::kSetRemoteAnswerResolved ||
      answerer_state_ == AnswererState::kSetLocalAnswerResolved) {
    return CallSetupState::kSucceeded;
  }
  if (has_ever_failed_)
    return CallSetupState::kFailed;
  return CallSetupState::kStarted;
}

bool CallSetupStateTracker::NoteOffererStateEvent(OffererState event,
                                                  bool document_uses_media) {
  if (document_uses_media)
    document_uses_media_ = true;
  auto transition = std::make_pair(offerer_state_, event);
  if (valid_offerer_transitions_.find(transition) ==
      valid_offerer_transitions_.end()) {
    return false;
  }
  offerer_state_ = event;
  if (IsRejectedOffererState(offerer_state_))
    has_ever_failed_ = true;
  return true;
}

bool CallSetupStateTracker::NoteAnswererStateEvent(AnswererState event,
                                                   bool document_uses_media) {
  if (document_uses_media)
    document_uses_media_ = true;
  auto transition = std::make_pair(answerer_state_, event);
  if (valid_answerer_transitions_.find(transition) ==
      valid_answerer_transitions_.end()) {
    return false;
  }
  answerer_state_ = event;
  if (IsRejectedAnswererState(answerer_state_))
    has_ever_failed_ = true;
  return true;
}

}  // namespace blink
