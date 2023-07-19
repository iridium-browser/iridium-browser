---
breadcrumbs:
- - /getting-involved
  - Getting Involved
page_name: summerofcode2013
title: SummerOfCode2013
---

**Mentors**

Add your ideas below with your name as a possible mentor. [Register with GSOC as
a
mentor](https://google-melange.appspot.com/gsoc/profile/mentor/google/gsoc2013?org=chromium)
using your chromium.org account.

**Students**

Since Chromium is a huge project and Summer of Code is in just a few months, you
will be a lot more successful tackling these projects if you contributed patches
to Chromium before.

The [For Developers page](/developers) contains information on how to check out
the source code and build Chromium.

Here are a few project ideas you might want to work on:

**Improvements to flakiness dashboard**

Possible mentors: Ojan Vafai

Chromium has a dashboard for flaky tests. If many tests fail in one run,
appengine times out during results upload. This should be fixed.

<http://crbug.com/224008>

**De-flake Instant Extended browser tests, write unit tests**

Possible mentors: ?

The new Instant Extended feature has some browser tests that are flaky, and
could also use unit tests.

**Debug disabled tests**

Possible mentors: ?

Many of our tests are marked as DISABLED_. Debug the ones that fail reliably,
fix whatever bugs you uncover, and enable the test again.

**Improvements to the Dr. Memory memory debugging tool**

Possible mentors: Derek Bruening, Qin Zhao

Dr. Memory finds memory-based errors (buffer overflows, use-after-free, etc.) in
Chromium code on Windows. The tool needs additional work in order to find
uninitialized reads on graphical Chromium tests and running the full browser
without false positives. See [further information and a list of Dr.
Memory-specific projects](http://code.google.com/p/drmemory/wiki/Projects).

**SPDY compliance tests**

Possible mentors: Fred Akalin

It would be really useful to have a suite of tests to validate that a SPDY
client or server conforms to the spec. httpbis (the next version of HTTP) will
be based on SPDY, so this work could dovetail into compliance tests for httpbis
also. The best language for the tests is probably Python.

**Your own idea!**

If you have an idea you would like to propose, email
[chromium-dev](https://groups.google.com/a/chromium.org/group/chromium-dev/topics?pli=1)
with the following:

*   Your name
*   What school you go to / what year of school / what degree
*   One/two line proposal summary
*   Details
*   Possible mentors, if any
