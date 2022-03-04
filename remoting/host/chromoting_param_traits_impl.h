// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_CHROMOTING_PARAM_TRAITS_IMPL_H_
#define REMOTING_HOST_CHROMOTING_PARAM_TRAITS_IMPL_H_

#include "remoting/host/chromoting_param_traits.h"

#include "ipc/ipc_message_utils.h"

template <typename SuccessType, typename ErrorType>
void IPC::ParamTraits<remoting::Result<SuccessType, ErrorType>>::Log(
    const param_type& p,
    std::string* l) {
  if (p) {
    l->append("success: ");
    LogParam(p.success(), l);
  } else {
    l->append("error: ");
    LogParam(p.error(), l);
  }
}

template <typename SuccessType, typename ErrorType>
bool IPC::ParamTraits<remoting::Result<SuccessType, ErrorType>>::Read(
    const base::Pickle* m,
    base::PickleIterator* iter,
    param_type* p) {
  bool is_success = false;
  if (!ReadParam(m, iter, &is_success)) {
    return false;
  }
  if (is_success) {
    p->EmplaceSuccess();
    if (!ReadParam(m, iter, &p->success())) {
      return false;
    }
  } else {
    p->EmplaceError();
    if (!ReadParam(m, iter, &p->error())) {
      return false;
    }
  }
  return true;
}

template <typename SuccessType, typename ErrorType>
void IPC::ParamTraits<remoting::Result<SuccessType, ErrorType>>::Write(
    base::Pickle* m,
    const param_type& p) {
  WriteParam(m, p.is_success());
  if (p) {
    WriteParam(m, p.success());
  } else {
    WriteParam(m, p.error());
  }
}

#endif  // REMOTING_HOST_CHROMOTING_PARAM_TRAITS_IMPL_H_
