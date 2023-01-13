---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/test-code-labs
  - Test Code Labs
page_name: autotest-client-tests
title: Autotest Client Tests
---

[TOC]

# References

*   [Autotest Best
            Practices](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/best-practices.md)
*   [Autotest Coding Style
            Guide](http://git.chromium.org/gitweb/?p=chromiumos/third_party/autotest.git;a=blob;f=CODING_STYLE;h=777cc1de3e409a69581ae44a9432d8634dc114e6;hb=HEAD)
*   [Writing
            Autotests](http://www.chromium.org/chromium-os/testing/autotest-developer-faq#TOC-Writing-Autotests)
*   [Dynamic Suites
            Codelab](/chromium-os/testing/test-code-labs/dynamic-suite-codelab)
*   [Server Side Autotests
            Codelab](/chromium-os/testing/test-code-labs/server-side-test)
*   [Result logs
            ](/chromium-os/testing/test-code-labs/autotest-client-tests/autotest-results-logs)
*   [Client helper
            libraries](/chromium-os/testing/test-code-labs/autotest-client-tests/autotest-client-helper-libraries)
*   [Troubleshooting ebuild
            files](/chromium-os/testing/test-code-labs/autotest-client-tests/basic-ebuild-troubleshooting)

# Overview

In this codelab, you will build a client-side Autotest to check the disk and
cache throughput of a ChromiumOS device. You will learn how to:

1. Setup the environment needed for autotest

2. Run and edit a test

3. Write a new test and control file

4. Check results of the test

In the process of doing so you will also learn a little about the autotest
framework.

### Background

Autotest is an open source project designed for testing the linux kernel. Before
starting this codelab you might benefit from scrolling through some [upstream
documentation](https://github.com/autotest/autotest/wiki/AddingTest) on autotest
client tests. Autotest is responsible for managing the state of multiple client
devices as a distributed system, by integrating a web interface, database,
servers and the clients themselves. Since this codelab is about client tests,
what follows is a short description of how autotest runs a specific test, on one
client.

Autotest looks through all directories in client/tests and client/site_tests for
simple python files that begin with ‘control.’. These files contain a list of
variables, and a call to job.run_test. The control variables tell autotest when
to schedule the test, and the call to run_test tells autotest how. Each test
instance is part of a job. Autotest creates this job object and forks a child
process to execute its control file.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/szgUiW_L7jkUJospRoV-dlQ/image?w=668&h=107&rev=1&ac=1"
height=107px; width=668px;>

Note the exec mentioned above is the python keyword, not os.exec

Tests reside in a couple of key locations in your checkout, and map to similar
locations on the DUT (Device Under Test). Understanding the layout of these
directories might give you some perspective:

*   `<cros_checkout>/chromium/src/chrome/test/functional`: holds
            chrome/chromeos functional tests.
    *   These include pyauto tests, but not autotest tests (Note: pyauto
                is deprecated).
    *   On the DUT, these map to:
                /usr/local/autotest/deps/chrome_test/test_src/chrome/test/functional/
*   `<cros_checkout>/src/third_party/autotest/client/site_tests`: holds
            autotest tests.
    *   On the DUT, these map to /usr/local/autotest/tests.
*   `<cros_checkout>/src/platform/factory`: holds some private factory
            tests, although the bulk of factory tests reside in site_tests.

Please consult the [dynamic suite
codelab](/chromium-os/testing/test-code-labs/dynamic-suite-codelab) to
understand how your tests can run as a suite.

## Prerequisites

    chroot build environment.

    Autotest source, basic python knowledge.

## Objectives

In this codelab, we will:

    Run a test, edit and rerun a test on the client device

    Write a new test from scratch

    View and collect results of the test

    Get an overview of how autotest on the client works

    Get an overview of the classes and libraries available to help you write
    tests.

# Running a test on the client

First, get the autotest source:

a. If you [Got the Code](http://www.chromium.org/chromium-os/developer-guide),
you already have autotest.

b. If you do not wish to sync the entire source and reimage a device, you can
run tests in a vm.

*   [Install
            gsutils](https://sites.google.com/a/google.com/chromeos/for-team-members/lab/gsutil-setup)
*   Get an image
    *   Select your image, e.g.,

        ```none
        gsutil ls gs://chromeos-releases/dev-channel/lumpy/*
        ```

    *   Copy your image, e.g.,

        ```none
        gsutil cp gs://chromeos-releases/dev-channel/lumpy/3014.0.0/ChromeOS-R24-3014.0.0-lumpy.zip ./
        ```

    *   Unzip the image and untar autotest.tar.bz2, e.g.,

        ```none
        unzip ChromeOS-R24-3014.0.0-lumpy.zip chromiumos_qemu_image.bin autotest.tar.bz2
        ```

*   [Get a
            VM](http://www.chromium.org/running-smoke-suite-on-a-vm-image):
    *   The unzipped folder from 2.b should contain a VM.

        ```none
        <cros_checkout>/src/scripts/bin/cros_start_vm --image_path=path/to/chromiumos_qemu_image.bin
        ```

If the cros_start_vm scripts fails you need to enable virtualization on your
workstation. check for /dev/kvm or run ‘sudo kvm-ok’ (you might have to ‘sudo
apt-get install cpu-checker’ first). It will either say /dev/kvm exists and kvm
acceleration can be used or that /dev/kvm doesn’t and kvm acceleration can NOT
be used. In the latter case, hit esc on boot and go to ‘system security:’, turn
on virtualization. More information about running tests on a vm can be found
[here](http://www.chromium.org/running-smoke-suite-on-a-vm-image).

Once you have autotest, there are 2 ways to run tests, either using your machine
as a server or from the client DUT. Running it directly on the device is faster,
but requires invoking it from your server at least once.

#### Through test_that

1. enter chroot:

```none
cros_checkout_directory$ cros_sdk
```

2. Invoke test_that, to run login_LoginSuccess on a vm with local autotest bits:

```none
test_that localhost:9222 login_LoginSuccess
```

The basic usage of test_that:

```none
test_that -b board dut_ip[:port] TEST
```

TEST can be the name of the test, or suite:suite_name for a suite. For example,
to run the smoke suite on a device with board x86-mario

```none
test_that -b x86-mario 192.168.x.x suite:smoke
```

Please see the[ test_that
](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/test-that.md)page
for more details.

## Directly on the DUT

You have to use test_that at least once so it copies over the test/dependencies
before attempting this; If you haven’t, /usr/local/autotest may not exist on the
device.

```none
ssh root@<ip_address_of_dut>, password=test0000 
```

Once you're on the client device:

```none
cd /usr/local/autotest
```

```none
./bin/autotest_client tests/login_LoginSuccess/control
```

# Editing a Test

*For python-only changes, test_that uses` autotest_quickmerge` to copy your
python changes to the sysroot. There is no need to run rcp/scp to copy the
change over to your DUT.*

The fastest way to edit a test is directly on the client. If you find the text
editor on a Chromium OS device non-intuitive then edit the file locally and use
a copy tool like rcp/scp to send it to the DUT.

1. Add a print statement to the
[login_LoginSuccess](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=client/site_tests/login_LoginSuccess/login_LoginSuccess.py;h=2b46ccbb0b37c2eaaa5e0c1a0a23b37c7d877546;hb=HEAD)
test you just ran

2. rsync it into /usr/local/autotest/tests on the client

```none
rcp path/to/login_LoginSuccess.py root@<DUT_ip>:/usr/local/autotest/tests/login_LoginSuccess/
```

3. run it by invoking autotest_client, as described in the section on Running
Tests Directly on the client. Note a print statement won’t show up when the test
is run via test_that.

The more formal way of editing a test is to change the source and emerge it. The
steps to doing this are very similar to those described in the section on
==[emerging
tests](/chromium-os/testing/test-code-labs/autotest-client-tests#TOC-Emerging-and-Running)==.
You might want to perform a full emerge if you’ve modified several files, or
would like to run your test in an environment similar to the automated
build/test pipeline.

# Writing a New Test

A word of caution: copy-pasting from Google Docs has been known to convert
consecutive whitespace characters into unicode characters, which will break your
control file. Using CTRL-C + CTRL-V is safer than using middle-click pasting on
Linux.

Our aim is to create a test which does the following:

*   run `hdparm -T <disk>`
*   Search output for timing numbers.
*   Report this as a result.

1. Create a directory in `client/site_tests`, name` kernel_HdParmBasic`.

2. Create a control file kernel_HdParmBasic/control, a bare minimum control file
for the hdparm test:

```none
AUTHOR = "Chrome OS Team"NAME = "kernel_HdParmBasic"TIME = "SHORT"TEST_TYPE = "client"DOC = """This test uses hdparm to measure disk performance."""job.run_test('kernel_HdParmBasic', named_arg='kernel test')
```

To which you can add the necessary control variables as described in the
[autotest best
practices](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/best-practices.md).
Job.run_test can take any named arguments, and the appropriate ones will be
[cherry
picked](http://git.chromium.org/gitweb/?p=chromiumos/third_party/autotest.git;a=blob;f=client/common_lib/test.py;h=9b51cd65f25b05b9f9a0b60339f77af85f46dc4b;hb=546142013484dbdb2c1322debe9e5fc5e91f63c1#l474)
and passed on to the test.

3. Create a test file:

At a bare minimum the test needs a run_once method, which should contain the
implementation of the test; it also needs to inherit from[
test.test](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=client/bin/test.py;h=33f194ca39e955d030789b08ae8f89bda8f45cab;hb=HEAD).
Most tests also need initialize and cleanup methods. Create a test file
`client/site_tests/kernel_HdParmBasic/kernel_HdParmBasic.py`:

```none
import logging
```

```none
```

```none
from autotest_lib.client.bin import testclass kernel_HdParmBasic(test.test):    version = 1    def initialize(self):      logging.debug('initialize')    def run_once(self, named_arg=''):      logging.debug(named_arg)    def cleanup(self):      logging.debug('cleanup')
```

Notice how only run_once takes the argument named_arg, which was passed in by
the `job.run_test `method. You can pass arguments to initialize and cleanup this
way too. You can find examples of initialize and cleanup methods in helper
classes, like
[cros_ui_test](https://gerrit.chromium.org/gerrit/gitweb?p=chromiumos/third_party/autotest.git;a=blob;f=client/cros/cros_ui_test.py;h=1f95267d8dce01626e7bcb560922e9513eef265d;hb=HEAD).

## Emerging and Running

Basic flow:

*   Add the new test: add `+tests_kernel_HdParmBasic` in the`
            IUSE_TESTS` section of the autotest-tests ebuild file:

```none
#third_party/chromiumos-overlay/chromeos-base/autotest-tests/autotest-tests-9999.ebuild
IUSE_TESTS="${IUSE_TESTS}
  # some other tests
  # some other tests
  # ...
  +tests_kernel_HdParmBasic
"
```

*   cros_workon autotest-test

```none
cros_workon --board=lumpy start autotest-tests
```

*   emerge autotest-tests

```none
emerge-lumpy chromeos-base/autotest-tests
```

(if that fails because of dependency problems, you can try cros_workon
--board=lumpy autotest-chrome and append `chromeos-base/autotest-chrome` to the
line above)

*   run test_that

```none
test_that -b lumpy DUT_IP kernel_HdParmBasics
```

If you’d like more perspective you might benefit from consulting the
[troubleshooting](/chromium-os/testing/test-code-labs/autotest-client-tests/basic-ebuild-troubleshooting)
doc.

## Checking results

The results folder contains [many
logs](/chromium-os/testing/test-code-labs/autotest-client-tests/autotest-results-logs),
to analyze client test logging messages you need to find
kernel_HdParmBasic.(DEBUG, INFO, ERROR) depending on which logging macro you
used. Note: logging message priorities escalate, and debug &lt; info &lt;
warning &lt; error. If you want to see all logging messages just look in the
debug logs.

Client test logs should be in:
`/tmp/test_that.<RESULTS_DIR_HASH>/kernel_HdParmBasic/kernle_HdParmBasic/debug`

where you will have to replace ‘`/tmp/test_that.<RESULTS_DIR_HASH>`’ with
anything you might have specified through the --results_dir_root option.

You can also find the latest results in `/tmp/test_that_latest`

In the DEBUG logs you should see messages like:

```none
01/18 12:22:46.716 DEBUG|     kernel_HdParmBasic:0025| Your logging message
```

Note that print messages will not show up in these logs since we redirect
stdout. If you’ve already performed a ‘run_remote’ once you can directly invoke
your test on a client, as described in the previous section. Two things to note
when using this approach:

a. print messages do show up

b. logging messages are also available under autotest/results/default/

## Import helpers

You can import any autotest client helper module with the line

```none
from autotest_lib.client.<dir> import <module>
```

You might also benefit from reading how the framework [makes
autotest_lib](http://www.chromium.org/chromium-os/testing/autotest-developer-faq#TOC-Writing-Autotests)
available for you.

kernel_HdParmBasic Needs test.test, so it needs to import test from client/bin.

Looking back at our initial test plan, it also needs to:

1. run `hdparm -T <disk>`

This implies running things on the command line, modules to look at are
base/site utils.

However common_lib’s ‘utils.py’ conveniently gives us both.

```none
from autotest_lib.client.bin import test, utils
```

2. Search output for timing numbers.

3. Report this as a result.

```none
import logging, re
```

## run_once, cleanup and initialize

If your test manages any state on the DUT it might need initialization and
cleanup. In our case the subprocess handles it’s own cleanup, if any. Putting
together all we’ve talked about, our run_once method looks like:

```none
import[ logging](https://cs.corp.google.com/#search&q=package:%5Echromeos_public$%20file:(/%7C%5E)logging(%5C.(swig%7Cpy)$%7C/__init__%5C.(swig%7Cpy)$)&is_navigation=1),[ re](https://cs.corp.google.com/#search&q=package:%5Echromeos_public$%20file:(/%7C%5E)re(%5C.(swig%7Cpy)$%7C/__init__%5C.(swig%7Cpy)$)&is_navigation=1)
```

```none
```

```none
from[ autotest_lib.client.bin](https://cs.corp.google.com/#search&q=package:%5Echromeos_public$%20file:(/%7C%5E)autotest_lib/client/bin(%5C.(swig%7Cpy)$%7C/__init__%5C.(swig%7Cpy)$)&is_navigation=1) import[ test](https://cs.corp.google.com/#search&q=package:%5Echromeos_public$%20file:(/%7C%5E)autotest_lib/client/bin/test(%5C.(swig%7Cpy)$%7C/__init__%5C.(swig%7Cpy)$)&is_navigation=1),[ utils](https://cs.corp.google.com/#search&q=package:%5Echromeos_public$%20file:(/%7C%5E)autotest_lib/client/bin/utils(%5C.(swig%7Cpy)$%7C/__init__%5C.(swig%7Cpy)$)&is_navigation=1)
class kernel_HdParmBasic(test.test):
    """
    Measure disk performance: both disk (-t) and cache (-T).
    """
    version = 1
    def run_once(self):
        if (utils.get_cpu_arch() == "arm"):
            disk = '/dev/mmcblk0'
        else:
            disk = '/dev/sda'
        logging.debug("Using device %s", disk)
        result = utils.system_output('hdparm -T %s' % disk)
        match = re.search('(\d+\.\d+) MB\/sec', result)
        self.write_perf_keyval({'cache_throughput': match.groups()[0]})
        result = utils.system_output('hdparm -t %s' % disk)
        match = re.search('(\d+\.\d+) MB\/sec', result)
        self.write_perf_keyval({'disk_throughput': match.groups()[0]})
```

Note the use of [performance
keyvals](http://www.chromium.org/chromium-os/testing/autotest-developer-faq#TOC-How-do-I-write-a-performance-test-)
instead of plain logging statements. The keyvals are written to
`/usr/local/autotest/results/default/kernel_HdParmBasic/results/keyval `on the
client and will be reported on your console when run through run_remote_tests:

---------------------------------------
kernel_HdParmBasic/kernel_HdParmBasic cache_throughput 4346.76
kernel_HdParmBasic/kernel_HdParmBasic disk_throughput 144.28
---------------------------------------
