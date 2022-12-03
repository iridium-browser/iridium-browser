---
breadcrumbs:
- - /developers
  - For Developers
page_name: clang-mac
title: clang mac
---

Read the [General Chromium/Clang wiki
page](http://code.google.com/p/chromium/wiki/Clang) first.

We want to keep chromium buildable with clang. Clang warns about more things
than gcc, so here are some things that need to be done even gcc doesn't complain
about them:

No const NSString\*s

All of Cocoa takes non-const NSObjects, and passing a `const NSString *` to a
function that takes `NSString *` is a const violation. gcc doesn't complain
about this, but clang intentionally does. constness is usually done via
immutable base classes and mutable subclasses in Cocoa anyway, so getting rid of
const isn't that bad. Also, we don't really have a choice. There is a good
chance what you are actually after is `NSString * const`.

**Converting between CFTypes and NSTypes**

The proper way to convert a CFType to a corresponding NSType is to use
CFToNSCast.

> #include "base/mac/foundation_util.h"

> .....

> NSString \*ns_string = base::mac::CFToNSCast(cf_string);

CFToNSCast avoids any ugly double C++ casts that you may dream up, is clang
safe, and only allows you to convert between properly toll-bridged types.

All properties are nonatomic

nonatomic: Some of our classes have custom setters that are not `@synchronize`d.
If the `@property` for that is not non-atomic, then the interface claims that
the method is synchronized but the implementation actually isn't. That's a bug.
gcc happens not to complain about this, but clang does. Since we shouldn't need
atomic properties anywhere, the simple rule is now to just make all properties
nonatomic.
