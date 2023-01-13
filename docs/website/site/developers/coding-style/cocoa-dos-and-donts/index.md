---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/coding-style
  - Coding Style
page_name: cocoa-dos-and-donts
title: Cocoa Dos and Don'ts
---

## Introduction

When writing code using Cocoa, there are rules to follow. Anything that uses
Objective-C++ must of course follow the [C++
guidelines](/developers/coding-style/cpp-dos-and-donts). However, there are
things in Cocoa that are more subtle than they look. Here are a few things to
keep in mind.

## Banned Practices

### Const NSObjects

The use of const NSObjects is **prohibited** as it makes Clang unhappy. (This is
enforced in the presubmit hooks.) See the [Mac Clang
page](/developers/clang-mac) for more details.

### Atomic properties

The use of atomic properties is **prohibited**. There are differences between
how our compilers handle them, and since we don't need them anyway we don't use
them. See the [Mac Clang page](/developers/clang-mac) for more details.

### Tracking rects and raw NSTrackingAreas

Tracking rects are tricky to get right. Apple realized that, and in Mac OS X
10.5 introduced the NSTrackingArea object. Unfortunately, that is *also*
difficult to get right, and has historically led to [nasty
crashes](http://crbug.com/48709).

The use of `-[NSView addTrackingRect:owner:userData:assumeInside:]` is
**prohibited**.

The use of raw `NSTrackingArea`s is **prohibited**.

You are required to use `CrTrackingArea`. Find it at
`ui/base/cocoa/tracking_area.h`. There are currently (and unfortunately) lots of
current usage of tracking rects and `NSTrackingArea`s. Please convert them as
you encounter them.

## YR DOIN IT RONG

There are spots of Cocoa that are tricky. Watch out.

### Coordinate conversion

All code that manages views is required to be correct, and that correctness is
more heavily tested when running under "resolution independence" or "HiDPI",
where window coordinates are not the same as view coordinates. (Turn on
resolution independence in Quartz Debug.app.)

Any time you need to compute window coordinates based on view coordinates, or
vice versa, make sure you use the appropriate conversion calls (i.e.
`-convertRect:toView`: and friends). There is a call for each data type and each
conversion direction. Also, remember that views have both a `-bounds` and
`-frame`. They are in different coordinate systems.

See
<http://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/CocoaDrawingGuide/Transforms/Transforms.html>
for details.

### The "base" coordinate system

In doing coordinate conversion, you may run into methods on views that convert
to and from the "base" coordinate system. *Those are almost certainly **not**
what you are looking for.* When talking about views, the "base" coordinate
system is referring to the raw backing pixels. If you need to precisely align to
pixels, they are correct, but most of the time they do not do what you think
they are doing.

(This problem is compounded by the references in methods on windows to the
"base" coordinate system, which is indeed the window coordinate system.)

See
<http://groups.google.com/a/chromium.org/group/chromium-dev/browse_thread/thread/6b989dcbb79e9ba/029eeb633c5287cc>
for details.

### Zero-sized windows

The Mac's windowserver has issues with zero-sized windows, [spewing tons of junk
to the console](http://crbug.com/78973), yet it's not immediately obvious when
you resize them that it's a problem.

Ensure that you do not resize windows to be empty, either implicitly or
explicitly.
