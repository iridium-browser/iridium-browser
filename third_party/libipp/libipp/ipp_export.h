// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBIPP_IPP_EXPORT_H_
#define LIBIPP_IPP_EXPORT_H_

#define IPP_EXPORT __attribute__((__visibility__("default")))
#define IPP_PRIVATE __attribute__((__visibility__("hidden")))

#endif  //  LIBIPP_IPP_EXPORT_H_
