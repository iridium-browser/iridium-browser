---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/writing-and-developing-tests
  - Writing and Developing Tests
page_name: writing-tests-faq
title: Writing Tests FAQ
---

[TOC]

## Coding Style

As Autotest is a mature upstream project we follow their style code when it
comes to committing changes here as opposed to the Chromium OS style guide.
Please refer to the coding style document that is in autotest/CODING_STYLE or
view it
[here](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=CODING_STYLE;).

## Upstream Documentation

Upstream is hosted on github at <http://autotest.github.com>

Upstream Wiki <https://github.com/autotest/autotest/wiki>

The github website doesn't really let you search through the docs but, you can
clone the wiki repo and grep through it:

```none
git clone https://github.com/autotest/autotest.wiki.git
```

## Where are useful libraries?

Autotest has an import structure that exposes a lot of functionality from a
number of different places. Good places to grep for code that may already do
what you need is:

In general we should try to use the same functions in tests that are written
rather than creating similar functions. This helps keep the whole frame work in
a maintainable state and utilizes code that has been used for a while and is
known to work.

*   client/common_lib
*   client/bin

## I am writing RPCs, what a is good reference to look at?

Looking at the RPC doc from the server itself can be useful.

<http://cautotest/afe/server/rpc_doc>

## Writing Autotests

### Where do autotests live?

Most tests are checked in under third_party/autotest/files/ (autotest.git
chromium-os project). Some may be scattered around other locations. See
[link](/chromium-os/testing/building-and-running-tests#TOC-Q1:-What-autotest-ebuilds-are-out-there-)
to find out the complete list.

**Test cases are not upstreamed to the general autotest repository as most are
Chromium OS specific. They are checked out as part of the regular sync command
you used from the Chromium OS repository.**

**dfdf**

### A word about imports

Autotest has an interesting way of imports due to it not being installed
properly in PYTHONPATH. This is not an issue on Autotest trunk but Chromium OS
has not yet remerged all the work there.

When dealing with Autotest internal modules you will need to import/create a
*common.py* file that can relatively reference the root directory of Autotest.
You will find these spread throughout the code base and below is a concrete
example.

If you are working on code under **autotest/files/client/common_lib** and you
wanted to imported something from server you would need to *import a common.py*
that looks similar to the following:

```none
import os, sysdirname = os.path.dirname(sys.modules[__name__].__file__)client_dir = os.path.abspath(os.path.join(dirname, ".."))sys.path.insert(0, client_dir)import setup_modulessys.path.pop(0)setup_modules.setup(base_path=client_dir,                    root_module_name="autotest_lib.client")
```

\*Notice the relative path that is fed in to the client_dir variable.

Once this is in the directory where you want to use the Autotest

```none
#!/usr/bin/pythonimport os, sys
import commonfrom autotest_lib.server import frontend
frontend.. etc etc
```

**\*\*Note when you are writing a test the framework automatically makes
autotest_lib available for you. There is no need to place a copy of common.py in
your test directory.

### Writing a simple test

**`Autotest tests are checked into several locations under third_party/autotest.
`**

**`There are two flavors of tests: client and server. All tests are managed by
an autotest server machine that is typically the machine where run_remote_tests
is invoked. Client tests execute entirely on the Chromium OS device. Server
tests execute on both server and client or only the server. Client tests are
usually the simplest to write and run. Server tests are needed, for example, if
a test needs to reboot the device or interact with external devices (e.g. to cut
off power to the Chromium OS device).`**

**`Tests are located in 4 locations in the `**

**``**``**`third_party/autotest/files/ `**``**``**

**`` `tree:`**``**

    **``**`client/site_tests - These are where most tests live. These are client
    tests that are Chromium OS specific.`**``**

    **``**`client/tests - These are client tests that are general Linux tests
    (not Chromium OS specific).`**``**

    **``**`server/site_tests - These are server tests that are Chromium OS
    specific.`**``**

    **``**`server/tests - These are server tests that are general Linux tests
    (not Chromium OS specific).`**``**

****Decide if your test is a client or server test and choose the appropriate
directory from the above. In the sections below we refer to this directory as
${TEST_ROOT} below.****

Next decide which area your test falls within based on the tracker
(<http://code.google.com/p/chromium-os/issues/list>). It should be something
like "desktopui", "platform", or "network" for instance. This name is used to
create the test name; e.g. "network_UnplugCable". Create a directory for your
test at $(TEST_ROOT)/$(LOWERCASE_AREA)_$(TEST_NAME).

Try to find an example test that does something similar and copy it. You will
create at least 2 files:

${TEST_ROOT}/${LOWERCASE_AREA}_${TEST_NAME}/control

${TEST_ROOT}/${LOWERCASE_AREA}_${TEST_NAME}/${LOWERCASE_AREA}_${TEST_NAME}.py

**``********``********`******Your control file runs the test and sets up default parameters. The .py file is the actual implementation of the test.****** `********``********``**

**``********``********``********``****`Inside the control the TEST_CLASS
variable should be set to ${LOWERCASE_AREA}. The naming convention simply exists
to make it easier to find other similar tests and measure the coverage in
different areas of Chromium OS. `****``********``********``********``**

**``********``********``********``****`You may also want to add this test to one
or more existing test suites to have it run
automatically.`****``********``********``********``**

After this, you should modify the autotest-tests-9999.ebuild under
src/third_party/chromiumos-overlay/chromeos-base/autotest-tests and add your
test to IUSE_TESTS or it won't be picked up by autotest when you ask it to build
specific tests.

### Adding binaries for your tests to call as part of the test

In order to cross-compile, your test's compilation step should be implemented
inside the `setup() method of your python code.` A couple of simple examples:

*   Sources inside the autotest repo as part of the test: gl_Bench
*   Sources checked in as a tarball from upstream: hardware_SAT
*   Sources checked in other Chromium OS source repo:
            firmware_VbootCrypto, desktopui_DoLogin

*It is your responsibility to make sure the test will build for all supported
platforms as it will cause a build break if it does not.*

The setup method of all python scripts are built as part of build_autotest.sh
(which is called when you `cros build-packages --withautotest`).  This script
calls all the setup functions of every python test and runs them.  These setup
steps are compiled on the host for execution on the target.  The cross-compiler
flags are already set so make sure if you have your own Makefile to take in the
`CC`, `CFLAGS`, etc. from the environment rather than hardcode them.

*Note that if you have a* `*setup()* `*method, it should create a* `*src*`
*directory, even if empty, to avoid running the setup method on the target
device.*

### Writing tests that require that a user is logged in

If you write a test that requires a user is logged in, have your test subclass
site_ui_test.UITest. Any test that is a subclass of this class will login with
the default test account as part

of test initialization and logout as part of test cleanup.

### How do I do printf-style debugging in tests?

**Use logging.info() to generate log messages. If you are writing a client-side
test, the output unfortunately will not be shown while you are running autoserv
(either directly or indirectly by using run_remote_test.sh). If you want to
print out numeric data that would be generally useful for developers to track
over time, consider instead making this a performance test.**

### How do I write a performance test?

A performance test is like any other test except it also logs one or more
*performance keyvals*. A performance keyval is just an identifier and a floating
point number that is written to a keyvals file on the machine where
run_remote_tests is invoked. The identifier should be of the form
${UNITS}_${DESCRIPTION}. For a simple example, refer to hardware_DiskSize and
how it uses self.write_perf_keyval().

### How do I write a hardware qualification test?

Hardware qualification tests are like all other tests. The only difference is
that they are referenced in one of the HWQual/config.\* files. If they require
no manual setup or steps, they should be added to HWQual/config.auto. Otherwise,
a new config file should be created and instructions on the manual steps should
be added to suite_HWQual/README.txt.

Hardware qualification tests also usually involve a benchmark and a minimum
requirement for that benchmark. The nicest way to do this is to make your test
record performance metrics with write_perf_keyval() and then add a constraints
list in the HWQual control file that sets the minimum values. See
suite_HWQual/control.auto constraints parameters for examples.

### How do I write a manufacturing test?

See how to write a hardware qualification test. A manufacturing test should be
written similarly, however there is not currently (3/3/2010) a suite for
manufacturing tests.

### How do I write a test that interacts with the UI?

This varies widely depending on which UI you would like to interact with.

If you would like to temporarily shut down X:

```none
from autotest_lib.client.cros import cros_ui
cros_ui.stop()
```

You can then start it back up by doing:

```none
from autotest_lib.client.cros import cros_ui
cros_ui.start()
```

If you want to control chrome, use PyAuto. See [PyAuto on
ChromiumOS](/system/errors/NodeNotFound) page for more details.
desktopui_UrlFetch is a sample autotest test that uses pyauto to control chrome.

**Note:** Pyauto is getting deprecated and being replace with Telemetry. Please
see below for how to run a Telemetry Test/Benchmark in autotest.

### How do I write a test that uses Telemetry?

[Please refer to Wrapping a Telemetry test in
Autotest](/system/errors/NodeNotFound)

### How do I bundle a bunch of tests into a suite that can be scheduled and run as a group?

See
<https://www.chromium.org/chromium-os/testing2/dynamic-suites/dynamic-test-suites>

### How do I write a test that requires some human interaction?

We call these semi-automated tests. If you want to write a test that pops up a
Chrome window, ask the test engineer some questions or to interact with some web
browser functionality, and verify the result, refer to the
desktopui_ChromeSemiAuto test.

### How do I create a test that requires running existing Linux utilities that are not currently installed?

Refer to the steps for a "no cross-compilation" test first. For this additional
component, you could add it to the image, but this is highly discouraged,
especially if only your test is likely to use the tool. Instead, it's better to
create a deps directory for the tool which builds it and installs it. A simple
example is hardware_SsdDetection which installs the hdparm utility.

***It is your responsibility to make sure the test will build for all supported
platforms as it will cause a build break if it does not.***

### How do I create a test that requires compiling code?

In order to cross-compile, your test's compilation step should be implemented
inside the `setup() method of your python code.` A couple of simple examples:

*   Sources inside the autotest repo as part of the test: gl_Bench
*   Sources checked in as a tarball from upstream: system_SAT
*   Sources checked in other Chromium OS source repo:
            firmware_VbootCrypto, system_AutoLogin

*It is your responsibility to make sure the test will build for all supported
platforms as it will cause a build break if it does not.*

Note that if you have a `setup() `method, it should create a `src` directory,
even if empty, to avoid running the setup method on the target device.

Here's an example. This is the content of platform_NullTest.py:

```none
#!/usr/bin/python
# Copyright 2018 The Chromium Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
from autotest_lib.client.bin import test, utils
class platform_NullTest(test.test):
    """
    Test autotest.
    """
    version = 1
    executable = 'nulltest'
    def setup(self):
        os.chdir(self.srcdir)
        utils.make(self.executable)
    def run_once(self):
        utils.system(self.srcdir + "/" + self.executable + " autotesting",
                     timeout=60)
```

After creating this file (and control, src/Makefile, and src/nulltest.c), add a
line to the autotest-tests ebuild
(chromiumos-overlay/chromeos-base/autotest-tests/autotest-tests-9999.ebuild).
Make sure that autotest-tests is in your cros_workon list, then run
TESTS=platform_NullTest emerge-&lt;board&gt; autotest-tests. This should compile
your C program. After this, you may runrun_remote_tests --use_emerged. Don't use
gmerge for autotest or autotest-tests.

### Why do I get the error "make: command not found" or "patch: command not found"?

These messages come up when autotest tries to run 'make' or 'patch' on the
client. They are not supposed to run there. Test code is only patched and
compiled on the host.

Common mistakes are:

*   failed to first build with 'TESTS=newtest emerge-&lt;board&gt;
            autotest-tests'.
*   failed to run the run_remote_tests.sh script with --use_emerged.

### What if I want to make use of a binary tool?

See the instructions for including and building the source above, or use the
ebuild method if needed. You cannot simply include a pre-built binary (e.g. by
checking it into the repository) since this will not work on any platform. All
software must be built by the official toolchain for that platform and the only
reasonable way to be sure of that is to include the source and build it.

### How do I write a test that needs access to large data files, like media files?

If the data files are posted somewhere else, they may fetched by the test when
it starts. If you add the fetching to the setup() method, this will cause the
fetch to occur during build_autotest and cause the fetched results to be put in
every packaged build of autotest. This can cause a large package and might also
imply that the media/data may be repackaged. The same issues apply if you commit
the large data files. Instead it's preferable to fetch the data from the client
machine.

Beware of licensing restrictions on sample data used by a test.

### How do I write a test that reboots the device?

See server/site_tests/platform_BootPerfServer, which reboots the device and runs
the client test platform_BootPerf which logs performance data about boot time.
Note that your test must be a server test.

### How do I write a test that measures power consumption?

See power_IdleServer. This test cuts power and logs performance data in the form
of battery life remaining information.

### What language can I write my test in?

The most straightforward language to use is Python. Every autotest needs at
least some Python code in the control file, and recording performance results,
printf-style debugging, and passing up informative error messages all require
Python code. You can also write C/C++ cross-compiled code. You can even write
shell scripts, but do note that you will cause yourself extra pain either when
developing your test and/or when diagnosing problems if you choose a language
other than Python.
