---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: running-tests
title: Running tests locally
---

[TOC]

## Running basic tests (gtest binaries)

### Windows

1.  [Build](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/windows_build_instructions.md)
2.  Set the test as the startup project and press F5 to compile it.
3.  Or in a cmd window, run the test of interest in `src\chrome\Debug`,
            e.g. `src\chrome\Debug\base_unittests.exe`

### Linux

1.  `cd` `src/`
2.  [Build](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux/build_instructions.md)
3.  `Run the test of interest, e.g. ./out/Debug/base_unittests`

Many unit tests create a chromium UI, which means they need the ability to
create a visible window. To run these tests remotely in a terminal, or to keep
them from opening windows in your current desktop session, you can run tests
inside Xvfb. See "Running in headless mode" below.

### **Chromium OS**

You can't run browser tests or unit tests on actual Chrome OS devices or from
within the cros chrome-sdk shell; instead, run them on a Linux system using a
build directory with target_os="chromeos".

1.  gn gen out/Debug_chromeos/ -args='is_debug=true
            target_os="chromeos"'
2.  autoninja -C out/Debug_chromeos/&lt;test target&gt; (for instance,
            &lt;test target&gt; might be extensions_browsertests or
            base_unittests)
3.  out/Debug_chromeos/&lt;test target&gt; (for instance,
            out/Debug_chromeos/extensions_browsertests)

### Mac

1.  [Build](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/mac_build_instructions.md)
            the tests: **ninja -C out/{Debug,Release} {test_target}**
2.  Run the appropriate test target:
            **./out/{Debug,Release}/{test_target}**

## Running a particular subtest

The above test executables are built with gtest, so they accept command line
arguments to filter which sub-tests to run. For instance, `base_unittests
--gtest_filter=FileUtilTest.*`

## Making tests show visible output

Tests that create a visible window do not draw anything into the window by
default, in order to run the test faster (with exception of tests that verify
pixel output). To force tests to draw visible pixels for debugging, you can use
the --enable-pixel-output-in-tests command-line flag. This can be used for both
unit tests and browser tests.

## Web tests

Blink has a large suite of tests that typically verify a page is laid out
properly. We use them to verify much of the code that runs within a Chromium
renderer.
To run these tests, build the blink_tests target and then run
third_party/blink/tools/run_web_tests.py --debug .

More information about running web tests or fixing web tests can be found on the
[**Web Tests
page**](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_tests.md).

## **Unit tests and Browser tests**

Most top-level directories of src/ have a unit test build target, such as
`content_unittests` for `content/`, `cc_unittests` for `cc/`, and
`components_unittests` for `components/`

There is also the fallback `unit_tests` target for unit tests built on top of
the full chrome stack.

Unit tests verify some part of the chromium code base in an isolated test
environment, and are usually found in files with a `_unittest.cc` suffix.
Browser tests run a full browser, and then execute a test inside the browser
instance, and are usually found in files with a `_browsertest.cc` suffix. There
is more information on browser tests [here](/developers/testing/browser-tests).
To add a new test, you will generally find a similar test and clone it. If you
can, strongly prefer writing a unit test over a browser test as they are
generally faster and more reliable.

## Get dumps for Chromium crashes on Windows

Before running the tests, make sure to run `crash_service.exe`. We use this
program to intercept crashes in chromium and write a crash dump in the Crash
Reports folder under the User Data directory of your chromium profile.

If you also want `crash_service.exe` to intercept crashes for your normal Google
Chrome or Chromium build, add the flag `--noerrdialogs`.

You can also use the flag `--enable-dcheck` to get assertion errors in release
mode.

## Running in headless mode with `--ozone-platform` or xvfb

When ssh-ed in to a machine, you don't have a display connected, which means
that you normally can't run tests. Some tests don't need to draw to the screen,
and for them you can simply add `--ozone-platform=headless` to your command
line. For the others, you can run tests in headless mode with xvfb (X Virtual
Frame Buffer). There are multiple ways to run tests with xvfb!

Example with unittests and testing/xvfb.py (note: you will need to install
xcompmgr, although this is done as part of install-build-deps.py)

testing/xvfb.py out/Default/component_unittests

Example with browser tests and running Xvfb and setting the DISPLAY env
variable.

Xvfb.py :100 -screen 0 1600x1200x24 &
DISPLAY=localhost:100 out/Default/browser_tests

Third example, with xvfb-run:

xvfb-run -s "-screen 0 1024x768x24" out/Default/content_unittests

## **Run tests 5x faster on gLinux**

Browser tests on gLinux start up extremely slowly due to idiosyncratic NSS
configurations. If you need to regularly run browser tests on gLinux, consider
using the run_with_dummy_home.py helper script:

testing/run_with_dummy_home.py testing/xvfb.py out/Default/browser_tests

This can speed tests up by 5x or more.
