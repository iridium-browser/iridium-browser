# Open Screen Library Style Guide

The Open Screen Library follows the [Chromium C++ coding style](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md)
which, in turn, defers to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
We also follow the [Chromium C++ Do's and Don'ts](https://sites.google.com/a/chromium.org/dev/developers/coding-style/cpp-dos-and-donts).

C++14 language and library features are allowed in the Open Screen Library
according to the [C++14 use in Chromium](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++11.md)
guidelines.

In general Open Screen follows [You Aren't Gonna Need
It](https://martinfowler.com/bliki/Yagni.html) principles.

## Disallowed Styles and Features

Blink style is *not allowed* anywhere in the Open Screen Library.

C++17-only features are currently *not allowed* in the Open Screen Library.

GCC does not support designated initializers for non-trivial types.  This means
that the `.member = value` struct initialization syntax is not supported unless
all struct members are primitive types or structs of primitive types (i.e. no
unions, complex constructors, etc.).

## Modifications to the Chromium C++ Guidelines

- `<functional>` and `std::function` objects are allowed.
- `<chrono>` is allowed and encouraged for representation of time.
- Abseil types are allowed based on the allowed list in [DEPS](
  https://chromium.googlesource.com/openscreen/+/refs/heads/master/DEPS).
- However, Abseil types **must not be used in public APIs**.
- `<thread>` and `<mutex>` are allowed, but discouraged from general use as the
  library only needs to handle threading in very specific places;
  see [threading.md](threading.md).
- Following YAGNI principles, only implement comparison operator overloads as
  needed; for example, implementing operator< for use in an STL container does
  not require implementing all comparison operators.

## Code Syntax

- Braces are optional for single-line if statements; follow the style of
  surrounding code.
- Using-declarations are banned from headers.  Type aliases should not be
  included in headers, except at class scope when they form part of the class
  definition.
    - Exception: Using-declarations for convenience may be put into a shared
      header for internal library use.  These may only be included in
      .cc files.
    - Exception: if a class has an associated POD identifier (int/string), then
      declare a type alias at namespace scope for that identifier instead of using
      the POD type.  For example, if a class Foo has a string identifier, declare
      `using FooId = std::string` in foo.h.

## Copy and Move Operators

Use the following guidelines when deciding on copy and move semantics for
objects:

- Objects with data members greater than 32 bytes should be move-able.
- Known large objects (I/O buffers, etc.) should be be move-only.
- Variable length objects should be move-able
  (since they may be arbitrarily large in size) and, if possible, move-only.
- Inherently non-copyable objects (like sockets) should be move-only.

### Default Copy and Move Operators

We [prefer the use of `default` and `delete`](https://sites.google.com/a/chromium.org/dev/developers/coding-style/cpp-dos-and-donts#TOC-Prefer-to-use-default)
to declare the copy and move semantics of objects.  See
[Stroustrup's C++ FAQ](http://www.stroustrup.com/C++11FAQ.html#default)
for details on how to do that.

Classes should prefer [member
initialization](https://en.cppreference.com/w/cpp/language/data_members#Member_initialization)
for POD members (as opposed to value initialization in the constructor).  Every POD
member must be initialized by every constructor, of course, to prevent
(https://en.cppreference.com/w/cpp/language/default_initialization)[default
initialization] from setting them to indeterminate values.

### User Defined Copy and Move Operators

Classes should follow the [rule of three/five/zero](https://en.cppreference.com/w/cpp/language/rule_of_three).

This means that if they implement a destructor or any of the copy or move
operators, then all five (destructor, copy & move constructors, copy & move
assignment operators) should be defined or marked as `delete`d as appropriate.
Finally, polymorphic base classes with virtual destructors should `default` all
constructors, destructors, and assignment operators.

Note that operator definitions belong in the source (`.cc`) file, including
`default`, with the exception of `delete`, because it is not a definition,
rather a declaration that there is no definition, and thus belongs in the header
(`.h`) file.

## Passing Objects by Value or Reference

In most cases, pass by value is preferred as it is simpler and more flexible.
If the object being passed is move-only, then no extra copies will be made.  If
it is not, be aware this may result in unintended copies.

To guarantee that ownership is transferred, pass by rvalue reference for objects
with move operators.  Often this means adding an overload that takes a const
reference as well.

Pass ownership via `std::unique_ptr<>` for non-movable objects.

Ref: [Google Style Guide on Rvalue References](https://google.github.io/styleguide/cppguide.html#Rvalue_references)

## Noexcept

We prefer to use `noexcept` on move constructors.  Although exceptions are not
allowed, this declaration [enables STL optimizations](https://en.cppreference.com/w/cpp/language/noexcept_spec).

TODO(https://issuetracker.google.com/issues/160731444): Enforce this

Additionally, GCC requires that any type using a defaulted `noexcept` move
constructor/operator= has a `noexcept` copy or move constructor/operator= for
all of its members.

## Template Programming

Template programming should be not be used to write generic algorithms or
classes when there is no application of the code to more than one type.  When
similar code applies to multiple types, use templates sparingly and on a
case-by-case basis.

## Unit testing

Follow Google’s testing best practices for C++.  Design classes in such a way
that testing the public API is sufficient.  Strive to follow this guidance,
trading off with the amount of public API surfaces needed and long-term
maintainability.

Ref: [Test Behavior, Not Implementation](https://testing.googleblog.com/2013/08/testing-on-toilet-test-behavior-not.html)

## Open Screen Library Features

- For public API functions that return values or errors, please return
  [`ErrorOr<T>`](https://chromium.googlesource.com/openscreen/+/master/platform/base/error.h).
- In the implementation of public APIs invoked by the embedder, use
  `OSP_DCHECK(TaskRunner::IsRunningOnTaskRunner())` to catch thread safety
  problems early.

### Helpers for `std::chrono`

One of the trickier parts of the Open Screen Library is using time and clock
functionality provided by [`platform/api/time.h`](https://chromium.googlesource.com/openscreen/+/refs/heads/master/platform/api/time.h).

- When working extensively with `std::chrono` types in implementation code,
  [`util/chrono_helpers.h`](https://chromium.googlesource.com/openscreen/+/refs/heads/master/util/chrono_helpers.h)
  can be included for access to type aliases for
  common `std::chrono` types, so they can just be referred to as `hours`,
  `milliseconds`, etc. This header also includes helpful conversion functions,
  such as `to_milliseconds` instead of
  `std::chrono::duration_cast<std::chrono::milliseconds>`.
  `util/chrono_helpers.h` can only be used in library-internal code, and
  this is enforced by DEPS.
- `Clock::duration` is defined currently as `std::chrono::microseconds`, and
  thus is generally not suitable as a time type (developers generally think in
  milliseconds). Prefer casting from explicit time types using
  `Clock::to_duration`, e.g. `Clock::to_duration(seconds(2))`
  instead of using `Clock::duration` types directly.

### OSP_CHECK and OSP_DCHECK

These are provided in [`platform/api/logging.h`](https://chromium.googlesource.com/openscreen/+/refs/heads/master/platform/api/logging.h)
and act as run-time assertions (i.e., they
test an expression, and crash the program if it evaluates as false). They are
not only useful in determining correctness, but also serve as inline
documentation of the assumptions being made in the code. They should be used in
cases where they would fail only due to current or future coding errors.

These should *not* be used to sanitize non-const data, or data otherwise derived
from external inputs. Instead, one should code proper error-checking and
handling for such things.

OSP_CHECKs are "turned on" for all build types. However, OSP_DCHECKs are only
"turned on" in Debug builds, or in any build where the `dcheck_always_on=true`
GN argument is being used. In fact, at any time during development (including
Release builds), it is highly recommended to use `dcheck_always_on=true` to
catch bugs.

When OSP_DCHECKs are "turned off" they effectively become code comments: All
supported compilers will not generate any code, and they will automatically
strip-out unused functions and constants referenced in OSP_DCHECK expressions
(unless they are "extern" to the local module); and so there is absolutely no
run-time/space overhead when the program runs. For this reason, a developer need
not explicitly sprinkle "#if OSP_DCHECK_IS_ON()" guards all around any
functions, variables, etc. that will be unused in "DCHECK off" builds.

Use OSP_DCHECK and OSP_CHECK in accordance with the
[Chromium guidance for DCHECK/CHECK](https://chromium.googlesource.com/chromium/src/+/main/styleguide/c++/c++.md#check_dcheck_and-notreached).
