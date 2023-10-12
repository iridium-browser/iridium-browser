---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: autotest-developer-faq
title: Autotest Developer FAQ
---

[TOC]

The intent of this document is to provide a quick look up for common features
that are used in test development in Autotest. Including typical functions that
are used, where to search for other potential useful functions, and how to run
pylint/unittest_suite in Autotest.

Before you proceed, you should have read through the [Autotest User
Documentation](/chromium-os/testing/autotest-user-doc).

## Coding Style

As Autotest is a mature upstream project we follow their style code when it
comes to committing changes here as opposed to the Chromium OS style guide.
Please refer to the coding style document that is in
`autotest/docs/coding-style.md` or view it
[here](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/coding-style.md).

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

## How do I test changes to the Autotest codebase itself?

Often times when making changes to the Autotest codebase itself, it is difficult
to test your change unless you can actually run an instance of the Autotest
server locally.

To quickly get functional Autotest server setup please refer to: [Setup Autotest
Server](/chromium-os/testing/autotest-developer-faq/setup-autotest-server)

Reasons to run a local Autotest server might include:

*   Autotest Scheduler work
*   Adding or changing RPCs
*   Tests that involve common library changes
*   GUI/GWT Frontend work.

## I am writing RPCs, what a is good reference to look at?

Looking at the RPC doc from the server itself can be useful.

<http://cautotest/afe/server/rpc_doc>

## How do I test changes to the dynamic_suite infrastructure?

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

## Where do I store large files that need to be publicly accessible for tests?

Google Storage is used as the general storage facility for large files. Please
avoid putting large files in git as it increases everyone's check out size. A
good general rule of thumb is:

If it is a file larger than 5 megs that will be changing over time, put it in
Google Storage.

A similar approach is taken as
[localmirror](https://sites.google.com/a/google.com/chromeos/resources/engineering/releng/localmirror)
except we use the Google Storage bucket ***gs://chromeos-test-public***

To upload your files please follow ["How to get your files my files to local
mirror"](https://sites.google.com/a/google.com/chromeos/resources/engineering/releng/localmirror#TOC-How-do-I-get-my-files-in-localmirro)
but using the bucket ***gs://chromeos-test-public***

<http://sandbox.google.com/storage/?arg=chromeos-test-public>

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

## Writing Autotests

### Where do autotests live?

Most tests are checked in under third_party/autotest/files/ (autotest.git
chromium-os project). Some may be scattered around other locations. See [the
autotest
docs](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/user-doc.md#Q1_What-autotest-ebuilds-are-out-there)
to find out the complete list.

**`Test cases are not upstreamed to the general autotest repository as most are
Chromium OS specific. They are checked out as part of the regular sync command
you used from the Chromium OS repository.`**

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

For more information on writing your test, see the [user
docs](http://www.chromium.org/chromium-os/testing/autotest-user-doc).

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

****Use logging.info() to generate log messages. If you are writing a
client-side test, the output unfortunately will not be shown while you are
running autoserv (either directly or indirectly by using run_remote_test.sh). If
you want to print out numeric data that would be generally useful for developers
to track over time, consider instead making this a performance test.****

### **How do I write a performance test?**

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
Autotest](/chromium-os/testing/autotest-design-patterns#TOC-Wrapping-a-Telemetry-test-in-Autotest)

### How do I bundle a bunch of tests into a suite that can be scheduled and run as a group?

See [Test Suites](/chromium-os/testing/test-suites)

### How do I write a test that requires some human interaction?

We call these semi-automated tests. If you want to write a test that pops up a
Chrome window, ask the test engineer some questions or to interact with some web
browser functionality, and verify the result, refer to the
desktopui_ChromeSemiAuto test.

### How do I create a test that requires running existing Linux utilities that are not currently installed?

If your test is a server-side test, then you need to add the package to
utils/external_packages.py in autotest to get the package installed on the
autotest servers. If you have unittests that import the package also, then you
need to additionally add it to virtual/target-chromium-os-sdk so that it's in
the chroot so that it will be available for your unittests on builders.

If your test is a client-side test, your path changes based on if the
library/utility would be useful outside the context of your test. If yes, then
add it to virtual/target-chromium-os-test to get it included on the test image.
If no it's better to create a deps directory for the tool which builds it and
installs it. A simple example is hardware_SsdDetection which installs the hdparm
utility. ***It is your responsibility to make sure the test will build for all
supported platforms as it will cause a build break if it does not.***

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
your C program. After this, you may run run_remote_tests --use_emerged. Don't
use gmerge for autotest or autotest-tests.

### Why do I get the error "make: command not found" or "patch: command not found"?

These messages come up when autotest tries to run 'make' or 'patch' on the
client. They are not supposed to run there. Test code is only patched and
compiled on the host as describe
[above](http://www.chromium.org/chromium-os/testing/autotest-developer-faq#TOC-How-do-I-create-a-test-that-requires-compiling-code-).

Common mistakes are:

*   failed to first build with 'TESTS=newtest emerge-&lt;board&gt;
            autotest-tests'.
*   failed to run the run_remote_tests.sh script with --use_emerged.

### Why do I get the message "Not building any tests, because the requested list is empty" when I use "TESTS=$my_test"?

Recently (relative to the creation of most of these notes) autotests-tests was
split into several other packages in
chromiumos-overlay/chromeos-base/autotest-\*. If $my_test was moved out of
autotest-tests into autotest-chrome, for example, you have to cros_workon and
emerge autotest-chrome:

```none
cros_workon start --board <board> autotest-chrome
TESTS=$my_test emerge-<board> autotest-chrome
```

In general, you can grep for "tests_${my_tests}" in
chromiumos-overlay/chromeos-base/autotest-\*/\*9999.ebuild to find the package
with your test.

### How do I use deps

The deps feature is used to install packages that are required by tests.
Sometimes these packages must be compiled from source and installed on the test
machine. Rather than include this installation procedure in the test itself, we
can create a 'dep' which does this for us. Then the test can specify that it
needs this dep to work. Autotest will make sure that the dependency is built and
copied to the target.
First take a look at an example: hardware_SsdDetection. You will notice that it
does this in the setup method:

```none
self.job.setup_dep(['hdparm'])
```

This ends up running the python file `client/deps/hdparm.py`. This will look
after building the hdparm utility and making sure it is available for copying to
the target. The mechanics of this are explained in the next topic, but imagine
using `./configure` and installing with the `client/deps/hdparm` directory as
the install root.
Within the run_once() method in the test you will see this:

```none
        dep = 'hdparm'
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        self.job.install_pkg(dep, 'dep', dep_dir)
```

This is the code which actually installs the hdparm utility on the target. It
will be made available in the `/usr/local/autotest/deps/hdparm` directory in
this case. When it comes time to run it, you will see this code:

```none
        path = self.autodir + '/deps/hdparm/sbin/'
        hdparm = utils.run(path + 'hdparm -I %s' % device)
```

This is pretty simple - since the hdparm binary was installed to
`client/deps/hdparm/sbin/hdparm` we can run it there on the target.

### How can I create my own dep?

If you have your own tool and want to create a dep for it so that various tests
can use it, this section is for you.
First create a subdirectory within deps. Let's call your dep 'harry'. So you
will create `client/deps/harry` and put `harry.py` inside there.
In harry.py you will want something like the following:

```none
#!/usr/bin/python
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
from autotest_lib.client.bin import utils
version = 1
def setup(tarball, topdir):
    srcdir = os.path.join(topdir, 'src')
    utils.extract_tarball_to_dir(tarball, srcdir)
    os.chdir(srcdir)
    utils.system('patch -p1 < ../fix-wibble.patch')
    utils.configure()
    utils.make()
    utils.make('install')
# We got the src from http://harry.net/harry-0.24.tar.bz2
pwd = os.getcwd()
tarball = os.path.join(pwd, 'harry-0.24.tar.bz2')
utils.update_version(pwd + '/src', False, version, setup, tarball, pwd)
```

The URL mentioned in the file should be the location where you found your
tarball. Download this and put it in the deps directory as well. You will be
checking this into the repository as a binary file.
The `utils.update_version()` call ensures that the correct version of harry is
installed. It will call your setup() method to build and install it. If you
specify False for the preserve_srcdir parameter then it will always erase the
src directory, and call your setup() method. But if you specify True for
preserve_srcdir, then update_version() will check the installed version of the
src directory, and if it hasn't changed it will not bother installing it again.
You will also see that pwd is set to the current directory. This is
`client/deps/harry` in this case because autotest changes this directory before
importing your python module. We unpack into a src subdirectory within that to
keep things clean.
When your setup() method is called it should unpack the tarball, configure and
build the source, then install the resulting binaries under the same
`client/deps/harry` directory. You can see the steps in the example above - it
mirrors a standard GNU UNIX build process. If you need to pass parameters to
configure or make you can do that also. Any parameters after 'setup' in the
update_version() call are passed to your setup method.
After calling utils.update_version() from within harry.py we will have binaries
installed (say in `client/deps/harry/sbin/harry`) as well as the src directory
still there (`client/deps/harry/src`).
Finally for reasons that we won't go into you should create the file common.py
in your directory, like this:

```none
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os, sys
dirname = os.path.dirname(sys.modules[__name__].__file__)
client_dir = os.path.abspath(os.path.join(dirname, "../../"))
sys.path.insert(0, client_dir)
import setup_modules
sys.path.pop(0)
setup_modules.setup(base_path=client_dir,
                    root_module_name="autotest_lib.client")
```

and you need a file called 'control' too:

```none
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
job.setup_dep(['fio'])
```

### Troubleshooting your dep

If you have created a new dep but run_remote_tests.sh doesn't work and you get
an error like:

```none
PackageInstallError: Installation of harry(type:dep) failed : dep-harry.tar.bz2 could not be fetched from any of the repos ['autoserv://']
```

then this section is for you. It explains a little more of the detail of how the
dependencies are transfered from server (your workstation) to the client
(target).
When you call self.job.install_pkg(dep, 'dep', self.dep_dir), it causes the
autotest client to install the dependency. In this case the client will end up
using the autoserv fetcher. Within client/bin/harness_autoserve.py you will see
the class AutoservFetcher. It has a method fetch_pkg_file() which calls
harness_autoserv.fetch_package() to fetch the package. This issues a
AUTOTEST_FETCH_PACKAGE command over the logging connection to autoserv,
requesting the package. Yes the logging connection is re-used to support a kind
of ftp server for the client!
The server end of autoserv sits in server/autotest.py. Each line of log output
coming from the client is examined in client_logger._process_line(). When it
sees a line starting with AUTOTEST_FETCH_PACKAGE it calls _send_tarball() which
tries to find a matching directory to package up and send. The pkg_name is in a
standard format: `pkgtype-name.tar.bz2` and in this case pkgtype is 'dep'. It
will package up the client/deps/&lt;name&gt; directory into a tarball and send
it to the client. When this is working you will see something like this message
when you run the test:

```none
12:08:06 INFO | Bundling /home/sjg/trunk/src/third_party/autotest/files/client/deps/harry into dep-harry.tar.bz2
```

When it is not working, make sure the directory exists on the server side in the
right place.

### What if my dep is built by another ebuild?

The above method is fine for external packages, but it is not uncommon to want
to use a byproduct of another ebuild within a test. A good example of this is
the Chromium functional tests, which require PyAuto, a test automation framework
built by Chromium. In the
`src/third_party/chromiumos-overlay/chromeos-base/chromeos-chrome` ebuild you
will see:

```none
    if use build_tests; then
        install_chrome_test_resources "${WORKDIR}/test_src"
        # NOTE: Since chrome is built inside distfiles, we have to get
        # rid of the previous instance first.
        rm -rf "${WORKDIR}/${P}/${AUTOTEST_DEPS}/chrome_test/test_src"
        mv "${WORKDIR}/test_src" "${WORKDIR}/${P}/${AUTOTEST_DEPS}/chrome_test/"
        # HACK: It would make more sense to call autotest_src_prepare in
        # src_prepare, but we need to call install_chrome_test_resources first.
        autotest_src_prepare
```

You can see that this is creating a deps directory within the build root of
chrome, called chrome_test. From the previous section we know that a
chrome_test.py file must be installed in this deps directory for this to work.
This actually comes from the chromium tree
(`chrome/src/chrome/test/chromeos/autotest/files/client/deps/chrome_test/chrome_test.py`
since you asked). This file is very simple:

```none
#!/usr/bin/python
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import common, commands, logging, os
from autotest_lib.client.bin import utils
version = 1
def setup(top_dir):
    return
pwd = os.getcwd()
utils.update_version(pwd + '/src', False, version, setup, None)
```

In this case the directory already contains pre-built binaries, since they were
built and copied in by the ebuild above. The setup() method will always be
called, but has nothing to do.

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

**What if my test requires a build artifact?**

**[Internal-only documentation for build artifact downloading](https://docs.google.com/a/google.com/document/d/1rcfuMIPaaCOUp_41ahNdVM9K88iMYtCSdbiKCHn346U/edit?usp=sharing)**
