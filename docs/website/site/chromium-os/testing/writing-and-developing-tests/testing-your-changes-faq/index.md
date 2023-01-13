---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/writing-and-developing-tests
  - Writing and Developing Tests
page_name: testing-your-changes-faq
title: Testing Your Changes FAQ
---

[TOC]

## How do I test changes to the Autotest codebase itself?

Often times when making changes to the Autotest codebase itself, it is difficult
to test your change unless you can actually run an instance of the Autotest
server locally.

To quickly get functional Autotest server setup please refer to: [Setup Autotest
Server](/system/errors/NodeNotFound)

Reasons to run a local Autotest server might include:

*   Autotest Scheduler work
*   Adding or changing RPCs
*   Tests that involve common library changes
*   GUI/GWT Frontend work.

## How do I test changes to the dynamic_suite infastructure?

You'll definitely want a locally-running Autotest server. For most changes, you
will want to start by running the unit tests in server/cros/dynamic_suite as you
work. You may also need to actually run a suite of tests against your Autotest
instance. The 'dummy' suite is ideal, and there are several ways to run it:

*   site_utils/run_suite.py
*   cli/atest create_suite_job
*   server/autoserv test_suites/dev_harness.py

In the first two cases, there are a variety of command line options and
arguments that allow you to set the build and platform to test, etc. These
pathways will always stage a build (if necessary), pull control files from that
build, reimage devices and run the desired test suite.

dev_harness.py does not pull control files from a particular build, but rather
uses your local source tree. If you're trying to test changes to suite control
files, for example, this is probably the best way to go. It can also be
configured to skip reimaging, so you can iterate faster. dev_harness.py is
pulled in and executed directly by autoserv, so it can't take command line args.
Instead, it's configured using a file called dev_harness_conf that lives
adjacent to dev_harness.py.

## What's the fast way to make a change to a test and iterate to see if it now works?

The fastest way to develop a test is to *not use compiled components* in your
test. If you can write a test without cross-compiling code you can modify the
test in python/shell on the target and rerun it on the target directly. This of
course also applies to the situation where you want to modify the python/shell
parts of a test that has cross-compiled code. The point is, try to use the least
amount of cross-compiled code to write your test as possible.

Assuming you have done this, on the target device's console, su to root. Then
enter the /home/autotest directory on the target (provided you've run
run_remote_tests once with it) and modify the test which will be in the tests
subdirectory. You can run /home/autotest:

**```none
on_device# cd /home/autotest; ./bin/autotest tests/system_KernelVersion/control
```**

If you have modified cross-compiled test code, you can use the above
instructions but you'll be rebuilding a lot of stuff you don't need. Instead of
running build_platform --withautotest, you can instead just run
build_autotest.sh directly (which it calls). This emerges the autotest
cross-compiled binaries. You can also specify precisely which test you want to
rebuild to avoid having to build everything (and wait for it all to finish):

**```none
inside# ./build_autotest.sh --board=${BOARD} --build=storage_Fio,system_SAT
```**

**The resulting binaries will be placed in your chroot under
`${CHROOT}/build/${BOARD}/usr/local/autotest`. They are not installed into an
image at this time. Instead, autotest will copy them over when you run
run_remote_tests.sh.**

## **What's the fast way to see if a test now works with tip of tree?**

The best way is to prepare a CL and test it using
[trybots](/chromium-os/build/local-trybot-documentation).
