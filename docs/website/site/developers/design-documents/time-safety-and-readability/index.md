---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: time-safety-and-readability
title: Time Safety and Code Readability
---

As someone that's written a ton of code using the src/base/time classes, and
have made significant contributions in src/base/time, there are "holes" where
these types can be too-easily used in semantically incorrect ways. In addition,
there are opportunities to improve readability.

My "wish list" for changes (live document) follows:

## One C++ class for each Clock/Timeline

base::Time represents values that have been sampled/computed from the local
system calendar clock. However, base::TimeTicks values may have come from
several different clocks. A recent bug revealed the mixing of values from
different clocks was actually happening in the code base, and this was resolved
via <https://codereview.chromium.org/797893003/>. The solution eliminated the
possibility of having separate "high-res" versus "low-res" clock sources for
TimeTicks.

Two other TimeTicks clock sources remain: "ThreadNow" time and "SystemTrace"
time. We should provide separate classes for these so that compile-time checking
enforces semantically correct time math.

## Eliminate From/ToInternalValue()

Code throughout Chromium subverts the compile-time type-checking of time math by
illegally using these public methods. The code only works under an assumption
that, internally, the time classes use a one-microsecond timebase with values
stored internally in a 64-bit signed integer type. The stated purpose of these
methods is only to allow (de)serialization of the time types (e.g., for IPC),
and **not** to provide direct access to private members.

These methods should be eliminated altogether and replaced with
From/ToMicroseconds(). Code that (de)serializes time values should also simply
use the From/ToMicroseconds() methods, since that is the highest-resolution
timebase currently supported; and so (de)serialization can be performed without
introducing any rounding error.

## Fix Time::is_null() and TimeTicks::is_null()

These methods make an incorrect assumption: That a zero-valued Time is the same
as an "unset" or "undefined" Time. However, this has caused bugs abound (e.g.
<https://crbug.com/447742>). The reason is because "negative" Time values can
represent a point-in-time before epoch (e.g, kernel boot time for TimeTicks).
These negative values can then increment over time and become zero at some
point. This then fools code into thinking a Time value is "unset" when it has
actually been assigned/incremented to zero.

Instead of `0LL`, which is dirt-smack in the middle of the range of possible
values, a boundary value should represent the "unset" value: `kint64min`. The
default constructors for base::Time and base::TimeTicks should then initialize
to `kint64min` as well. We will have to be careful that this does not break any
assumptions in client code.

## "Zero" TimeDelta Operations

Currently, a lot of code does stuff like this:

```none
if (last_foo_offset_ < base::TimeDelta())
  last_foo_offset_ = base::TimeDelta();
...or...
DCHECK_GT(duration, base::TimeDelta());
...or...
SetNetworkingDelay(base::TimeDelta());
```

These lines of code are poor for readability. Instead, the TimeDelta class
should provide convenience methods for a global "zero" constant as well
"compare-to-zero". Then, the example above becomes:

```none
if (last_foo_offset_.is_negative())
  last_foo_offset_ = base::TimeDelta::Zero();
...or...
DCHECK(duration.is_positive());
...or...
SetNetworkingDelay(base::TimeDelta::Zero());
```

After migrating code to use the new convenience methods, a PRESUBMIT check
should be added to make sure future code changes don't regress the code base.
(Note: The no-arg base::TimeDelta constructor must remain public to allow for
using this type in STL containers.)
