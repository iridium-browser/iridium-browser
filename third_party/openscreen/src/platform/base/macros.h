// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_BASE_MACROS_H_
#define PLATFORM_BASE_MACROS_H_

#ifdef DISALLOW_COPY
#define OSP_DISALLOW_COPY DISALLOW_COPY
#else
#define OSP_DISALLOW_COPY(ClassName) ClassName(const ClassName&) = delete
#endif

#ifdef DISALLOW_ASSIGN
#define OSP_DISALLOW_ASSIGN DISALLOW_ASSIGN
#else
#define OSP_DISALLOW_ASSIGN(ClassName) \
  ClassName& operator=(const ClassName&) = delete
#endif

#ifdef DISALLOW_COPY_AND_ASSIGN
#define OSP_DISALLOW_COPY_AND_ASSIGN DISALLOW_COPY_AND_ASSIGN
#else
#define OSP_DISALLOW_COPY_AND_ASSIGN(ClassName) \
  OSP_DISALLOW_COPY(ClassName);                 \
  OSP_DISALLOW_ASSIGN(ClassName)
#endif

#ifdef DISALLOW_IMPLICIT_CONSTRUCTORS
#define OSP_DISALLOW_IMPLICIT_CONSTRUCTORS DISALLOW_IMPLICIT_CONSTRUCTORS
#else
#define OSP_DISALLOW_IMPLICIT_CONSTRUCTORS(ClassName) \
  ClassName() = delete;                               \
  OSP_DISALLOW_COPY_AND_ASSIGN(ClassName)
#endif

#ifdef NOINLINE
#define OSP_NOINLINE NOINLINE
#else
#define OSP_NOINLINE __attribute__((noinline))
#endif

#endif  // PLATFORM_BASE_MACROS_H_
