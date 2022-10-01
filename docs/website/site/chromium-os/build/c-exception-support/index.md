---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: c-exception-support
title: C++ exception support
---

C++ exceptions are disabled by default for all C and C++ code. If you need them,
you can allow-list your ebuild.

**Background**

C++ exceptions are not allowed by the [Google C++ Style
Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml) and only
a few projects use them. This supports adds a some overhead to the binary file
on the .eh_frame and .eh_frame_hdr sections, even if you don't use
try/catch/throw, even if your program is in C. Nevertheless, they are enabled by
default by the compiler, since some exception support is required even if you
don't use them. For example, if function f() calls g() calls h(), and h() throws
an exception that f() is supposed to catch, then g() also needs to be compiled
with exception support, even if g() is a C function.

The overhead is in the range of 5% to 10% of the binary size, depending of the
average size of your functions, so we disable it by default.

**What to allow-list**

If your application uses try, catch or throw, it needs to be allow-listed
because otherwise it won't compile.

If your application calls a library that might throw an exception, but you don't
catch it, the application doesn't need to be compiled with exceptions support.
If the library throws an exception, the application will be terminated in either
case.

If your application catches an exception that is thrown by a function called by
an intermediate library, the library also needs to be compiled with exceptions
support. For example, if you pass a callback to a library that might throw an
exception, then the library should be also compiled with exception support.
That's why glib and glibc are allow-listed.

**Implementation**

To disable the exception support, we pass "-fno-exceptions -fno-unwind-tables
-fno-asynchronous-unwind-tables" in CFLAGS and CXXFLAGS by default. To
allow-list them, we filter these flags from CFLAGS and CXXFLAGS. We also define
the environment variable CXXEXCEPTIONS to 0 or 1 to determine whether the
exceptions are enabled.

The flags are added on the chromiumos-overlay/chromeos/make.conf.\*-target
files. To re-enable this support, you can call the bash function
cros_enable_cxx_exceptions defined in
chromiumos-overlay/profiles/base/profile.bashrc. That function will filter out
the flags from CXXFLAGS and CFLAGS.

This can be done on a per ebuild basis. For example, the dev-libs/dbus-c++
ebuild has the following snippet on
chromiumos-overlay/chromeos/config/env/dev-libs/dbus-c++.

> cros_pre_src_prepare_enable_cxx_exceptions() {

> cros_enable_cxx_exceptions

> }
