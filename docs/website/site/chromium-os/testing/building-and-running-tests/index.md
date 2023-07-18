---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: building-and-running-tests
title: Building and Running Tests
---

[TOC]

## Building tests

Fundamentally, a test has two main phases:

*   run_\*() - This is is the main part that performs all testing and is
            invoked by the control files, once or repeatedly.
*   setup() - This function, present in the test case’s main .py file is
            supposed to prepare the test for running. This includes building any
            binaries, initializing data, etc.

During building using emerge, autotest will call a setup() function of all test
cases/deps involved. This is supposed to prepare everything. Typically, this
will invoke make on a Makefile present in the test’s src/ directory, but can
involve any other transformation of sources (also be empty if there’s nothing to
build).
Note, however, that setup() is implicitly called many times as test
initialization even during run_\*() step, so it should be a noop on reentry that
merely verifies everything is in order.
Unlike run_\*() functions, setup() gets called both during the prepare phase
which happens on the host and target alike. This creates a problem with code
that is being depended on or directly executed during setup(). Python modules
that are imported in any pathway leading to setup() are needed both in the host
chroot and on the target board to properly support the test. Any binaries would
need to be compiled using the host compiler and either ensured that they will be
skipped on the target (incremental setup() runs) or cross-compiled again and
dynamically chosen while running on target.
More importantly, in Chromium OS scenario, doing any write operations inside the
setup() function will lead to access denied failures, because tests are being
run from the intermediate read-only location.
Given the above, building is as easy as emerge-ing the autotest ebuild that
contains our test.

```none
$ emerge-${board} ${test_ebuild}
```

Currently, tests are organized within these notable ebuilds: (see
[FAQ](/chromium-os/testing/autotest-user-doc#TOC-Q1:-What-autotest-ebuilds-are-out-there-)
full list)
chromeos-base/autotest-tests - The main ebuild handling most of autotest.git
repository and its client and server tests.
chromeos-base/autotest-tests-\* - Various ebuilds that build other parts of
autotest.git
chromeos-base/chromeos-chrome - chrome tests; the tests that are part of chrome

### Building tests selectively

Test cases built by ebuilds generally come in large bundles. Sometimes, only a
subset, or generally a different set of the tests provided by a given ebuild is
desired. That is achieved using a
[USE_EXPANDed](http://devmanual.gentoo.org/general-concepts/use-flags/index.html)
flag called *TESTS.*
All USE flags (and therefore tests) have a default state, either enabled (+) or disabled (-), specified directly in the ebuild, that can be manually overridden from the commandline. There are two ways to do that.

*   **Non-Incremental - Simply override the default selection by an
            entirely new selection, ignoring the defaults. This is useful if you
            develop a single test and don’t want to waste time building the
            others.**

```none
$ TESTS="test1 test2" emerge-${board} ${ebuild}
```

*   Incremental - All USE_EXPAND flags are also accessible as USE flags,
            with the appropriate prefix, and can be used incrementally to
            selectively enable/disable tests in addition to the defaults. This
            can be useful if you aim to enable a test that is disabled by
            default and want to test locally.

```none
$ USE="test_to_be_enabled -test_to_be_disabled" emerge-${board} ${ebuild}
```

For operations across all tests, following incremental USE wildcard is supported
by portage: "tests_\*" to select all tests at once (or - to de-select).
NOTE: Both Incremental and Non-Incremental methods can be set/overriden by (in
this order): the ebuild (default values), make.profile, make.conf, /etc/portage,
commandline (see above). That means that any settings provided on the emerge
commandline override everything else.

## Running tests

**NOTE:** In order to run tests on your device, it needs to have a [test-enabled
image](/chromium-os/testing/common-testing-workflows#TOC-W4.-Create-and-run-a-test-enabled-image-on-your-device).

When running tests, fundamentally, you want to either:

*   Run sets of tests manually - Use case: Developing test cases

Take your local test sources, modify them, and then attempt to run them on a
target machine using autotest. You are generally responsible for making sure
that the machine is imaged to a test image, and the image contains all the
dependencies needed to support your tests.

*   Verify a given image - Use case: Developing the projects subject to
            testing

Take an image, re-image the target device and run a test suite on it. This
requires either use of build-time autotest artifacts or their reproduction by
not modifying or resyncing your sources after an image has been built.

### Running tests on a machine - test_that

The frontend for running tests is a script called test_that. It provides an
interface for running the autoserv process which deploys and executes tests on
remote machines.
Most basically, test_that takes several flags for specifying the remote machine
IP address and a list of control files. Control files can be specified either by
their NAME field, or by a NAME- or path- matching regular expression. For
example:

```none
$ test_that test.device.hostname.com dummy_Pass
```

This script will use autotest_quickmerge to copy over any python-only test
changes from your source tree to the board sysroot, if it detects that your
source tree is newer. This allows you to iterate rapidly on python only changes
without needing to emerge your changes. However, non-python changes still
require a normal emerge.

### Running tests in a VM - cros_run_vm_tests

WARNING: After
[crbug/710629](https://bugs.chromium.org/p/chromium/issues/detail?id=710629),
'betty' is the only board regularly run through pre-CQ and CQ VMTest and so is
the most likely to work at ToT. 'betty' is based on 'amd64-generic', though, so
'amd64-generic' is likely to also work for most (non-ARC) tests.

VM tests are conveniently wrapped into a script cros_run_vm_tests that sets up
the VM using a given image and then calls test_that. This is run by builders to
test using the Smoke suite.
If you want to run your tests on the VM (see
[here](/chromium-os/how-tos-and-troubleshooting/running-chromeos-image-under-virtual-machines)
for basic instructions for setting up KVM with cros images) be aware of the
following:

*   cros_run_vm_test starts up a VM and runs test_that using the port
            specified (defaults to 9222). As an example:

```none
$ ./bin/cros_run_vm_test --test_case=suite_Smoke ---image_path=<my_image_to_start or don't set to use most recent build> --board=betty # or 'amd64-generic'
```

*   The emulator command line redirects localhost port 922 to the
            emulated machine's port 22 to allow you to ssh into the emulator.
            For Chromium OS to actually listen on this port you must append the
            --test_image parameter when you run the ./image_to_vm.sh script, or
            perhaps run the mod_image_for_test.sh script instead.
*   You can then run tests on the correct ssh port with something like

```none
$ ./test_that --board=x86-generic localhost:922 platform_BootPerf
```

*   To set the sudo password run set_shared_user_password. Then within
            the emulator you can press Ctrl-Alt-T to get a terminal, and sudo
            using this password. This will also allow you to ssh into the unit
            with, e.g.

```none
$ ssh -p 922 root@localhost
```

### Interpreting test results

Running autotest will result in a lot of information going by which is probably
not too informative if you have not used autotest before. At the end of the
test_that run, you will see a summary of pass/failure status, along with
performance results:

```none
22:44:30 INFO | Using installation dir /home/autotest
22:44:30 ERROR| Could not install autotest from repos
22:44:32 INFO | Installation of autotest completed
22:44:32 INFO | GOOD  ----  Autotest.install timestamp=1263509072 localtime=Jan 14 22:44:32 
22:44:33 INFO | Executing /home/autotest/bin/autotest /home/autotest/control phase 0
22:44:36 INFO | START  ---- ----  timestamp=1263509075 localtime=Jan 14 14:44:35 
22:44:36 INFO |  START   sleeptest sleeptest timestamp=1263509076 localtime=Jan 14 14:44:36 
22:44:36 INFO | Bundling /usr/local/autotest/client/tests/sleeptest into test-sleeptest.tar.bz2
22:44:40 INFO |   GOOD  sleeptest  sleeptest  timestamp=1263509079  localtime=Jan 14 14:44:39 completed successfully
22:44:40 INFO |   END GOOD  sleeptest sleeptest  timestamp=1263509079  localtime=Jan 14 14:44:39 
22:44:42 INFO | END GOOD ---- ---- timestamp=1263509082 localtime=Jan 14 14:44:42 
22:44:44 INFO | Client complete
22:44:45 INFO | Finished processing control file
```

`` `` `` `A directory full of the latest results will also be left at
/tmp/test_that_latest` `` `` ``

### Running tests automatically, Suites

Suites provide a mechanism to group tests together in test groups. They also
serve as hooks for automated runs of tests verifying various builds. Most
importantly, that is the BVT (board verification tests) and Smoke (a subset of
BVT that can run in a VM

Please refer to the [suites
documentation](/chromium-os/testing/dynamic-suites/dynamic-test-suites)

**## ****Troubleshooting/FAQ******

Fairly asked questions, because yes, some of this may be confusing!

**### Q1: What autotest ebuilds are out there?**

**Note that the list of ebuilds may differ per board, as each board has potentially different list of overlays. To find all autotest ebuilds for board foo, you can run:**

**```none
$ board=foo
$ for dir in $(portageq-${board} envvar PORTDIR_OVERLAY); do
     find . -name '*.ebuild' | xargs grep "inherit.*autotest" | grep "9999" |cut -f1 -d: | \
     sed -e 's/.*\/\([^/]*\)\/\([^/]*\)\/.*\.ebuild/\1\/\2/'
   done
```**

**### Q2: I see a test of the name ‘greattests_TestsEverything’ in build output/logs/whatever! How do I find which ebuild builds it?**

**All ebuilds have lists of tests exported as USE_EXPANDed lists called TESTS. An expanded use can be searched for in the same way as other use flags, but with the appropriate prefix, in this case, you would search for ‘tests_greattests_TestsEverything’:**

**```none
$ use_search=tests_greattests_TestsEverything
$ equery-$board hasuse $use_search
 * Searching for USE flag tests_greattests_TestsEverything …
[I-O] [  ] some_ebuild_package_name:0
```**

**This will however only work on ebuilds which are already installed, ie. their potentially outdated versions.**
**Alternately, you can run a pretended emerge (emerge -p) of all autotest ebuilds and scan the output.**

**```none
$ emerge -p ${all_ebuilds_from_Q1} |grep -C 10 “${use_search}”
```**

**### Q3: I have an ebuild ‘foo’, where are its sources?**

**Generally speaking, one has to look at the ebuild source to figure that question out (and it shouldn’t be hard). However, all present autotest ebuilds (at the time of this writing) are also ‘cros-workon’, and for those, this should always work:**

**```none
$ ebuild_search=foo
$ ebuild $(equery-$board which $ebuild_search) info
CROS_WORKON_SRCDIR=”/home/you/trunk/src/third_party/foo”
CROS_WORKON_PROJECT=”chromiumos/third_party/foo”
```**

**### Q4: I have an ebuild, what tests does it build?**

**You can run a pretended emerge on the ebuild and observe the ‘TESTS=’
statement:**

```none
$ ebuild_name=foo
$ emerge-$board -pv ${ebuild_name}
These are the packages that would be merged, in order:
Calculating dependencies... done!
[ebuild   R   ] foo-foo_version to /build/$board/ USE="autox hardened tpmtools xset -buildcheck -opengles" TESTS="enabled_test1 enabled_test2 … enabled_testN -disabled_test1 ...disabled_testN" 0 kB [1]
```

Alternately, you can use equery, which will list tests with the USE_EXPAND
prefix:

```none
$ equery-$board uses ${ebuild_name}
[ Legend : U - final flag setting for installation]
[        : I - package is installed with flag     ]
[ Colors : set, unset                             ]
 * Found these USE flags for chromeos-base/autotest-tests-9999:
 U I
 + + autotest                                    : <unknown>
 + + autotest                                    : <unknown>
 + + autox                                       : <unknown>
 + + buildcheck                                  : <unknown>
 + + hardened                                    : activate default security enhancements for toolchain (gcc, glibc, binutils)
 - - opengles                                    : <unknown>
 + + tests_enabled_test                     : <unknown>
 - - tests_disabled_test                      : <unknown>
```

### Q5: I’m working on some test sources, how do I know which ebuilds to cros_workon start in order to properly propagate?

You should ‘workon’ and always cros_workon start all ebuilds that have files
that you touched.
If you’re interested in a particular file/directory, that is installed in
/build/$board/usr/local/autotest/ and would like know which package has provided
that file, you can use equery:

```none
$ equery-$board belongs /build/${board}/usr/local/autotest/client/site_tests/foo_bar/foo_bar.py
 * Searching for <filename> …
chromeos-base/autotest-tests-9999 (<filename>)
```

DON’T forget to do equery-$board. Just equery will also work, only never return
anything useful.
As a rule of thumb, if you work on anything from the core autotest framework or
shared libraries (anything besides
{server,client}/{test,site_tests,deps,profilers,config}), it belongs to
chromeos-base/autotest. Individual test case will each belong to a particular
ebuild, see Q2.
It is important to cros_workon start every ebuild involved.

### Q6: I created a test, added it into ebuild, emerged it, and I’m getting access denied failures. What did I do wrong?

Your test’s setup() function (which runs on the host before being uploaded) is
probably trying to write into the read-only intermediate location.

### Q7: Missing pyauto_dep

You're seeing something like:

```none
Missing pyauto_dep. Unhandled PackageInstallError: Installation of pyauto_dep(type:dep) failed : dep-pyauto_dep.tar.bz2 could not be fetched from any of the repos ['autoserv://']
```

**You're running the now-deprecated run_remote_tests.sh without the
--use_emerged flag. Use test_that instead. OR, you are using test_that but have
never emerged autotest-chrome, and thus the telemetry dep is missing. See
[here](/chromium-os/testing/building-and-running-tests#TOC-Running-tests-on-a-machine---run_remote_tests.sh).**
