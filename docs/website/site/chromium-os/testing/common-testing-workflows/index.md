---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: common-testing-workflows
title: Common Testing Workflows
---

[TOC]

There are a lot of common developer workflows when it comes to testing. This
document details those.

## Workflow 1

### Developer already has a built system using build instructions. Developer wants to create a copy of their image for testing and build Chromium OS tests without re-running `cros build-packages`.

### cros_sdk and run mod_image_for_test on the chromiumos_image.bin (if you get complaints of unmount errors, sudo umount /media/\* outside the chroot).

## Workflow 2

*Developer has a testable image on their device. Developer makes modifications
to the python component of a test and wants to re-run the test on the target
system.*

If the user makes modifications on the host system, the easiest way to re-run
the tests is to re-run run_remote_tests. That script takes local changes from
third_party/autotest before running tests. However, if the developer wants to
make changes to dependencies of their tests, the developer must re-run
build_autotest.sh

## Workflow 3

### *Developer wants to run a test with certain conditions met on the host system before the system is ready for that test e.g. "browser is not running and has never run since boot".*

### For tests that require system-wide setup and system-wide cleanup, the best approach is probably to create a server-side test that sets up the environment, reboots the system, runs the test and cleans up the system. This should not be done for very small tests as rebooting is rather expensive. Take a look at [BootPerfServer](http://git.chromium.org/cgi-bin/gitweb.cgi?p=autotest.git;a=tree;f=server/site_tests/platform_BootPerfServer;h=2362958081700ed3e243935641ebe69b17890045;hb=HEAD) as an example to write such a test.

*Note in the long run we want to create utilities that verify certain
conditions. If you are interested in writing one of these utilities, please
email the dev list.*

## Workflow 4

*Developer wants to run a test that requires a user to be logged in.*

Subclass your test from site_ui_test.UITest

*Note we are trying to create a utility for such pre-req's as login, so make
sure to leave a big todo with DoLogin in it so that we can update your tests
once they the utility libraries are ready.*

### W1. Develop and iterate on a test

0. Set up environment

```none
$ cd ~/trunk/src/third_party/autotest/files/
$ export TESTS=”<the test cases to iterate on>”
$ EBUILD=<the ebuild that contains TEST>
$ board=<the board on which to develop>
```

1. Ensure cros_workon is started

```none
$ cros_workon --board=${board} start ${EBUILD}
$ repo sync # Necessary only if you use minilayout.
```

2. Make modifications (on first run, you may want to just do 3,4 to verify
everything works before you touch it & break it)

```none
$ …
```

3. Build test (TESTS= is not necessary if you exported it before)

```none
$ emerge-$board $EBUILD
```

4. Run test to make sure it works before you touch it

```none
$ run_remote_tests --remote=<machine IP> --use_emerged ${TESTS}
```

5. Go to 2) to iterate
6. Clean up environment

```none
$ cros_workon --board=${board} stop ${EBUILD}
$ unset TESTS
```

### W2. Creating a test - steps and checklist

When creating a test, the following steps should be done/verified.
1. Create the actual test directory, main test files/sources, at least one
control file
2. Find the appropriate ebuild package and start working on it:

```none
$ cros_workon --board=${board} start <package>
```

3. Add the new test into the IUSE_TESTS list of 9999 ebuild
4. Try building: (make sure it’s the 9999 version being built)

```none
$ TESTS=<test> emerge-$board <package>
```

5. Try running:

**```none
$ run_remote_tests.sh --remote=<IP> --use_emerged <test>
```**

**6. Iterate on 4,5 and modify source until happy with the initial version.**
**7. Commit test source first, when it is safely in, commit the 9999 ebuild version change.**
**8. Cleanup**

**```none
$ cros_workon --board=${board} stop <package>
```**

**### W3. Splitting autotest ebuild into two**

**Removing a test from one ebuild and adding to another in the same revision causes portage file collisions unless counter-measures are taken. Generally, some things routinely go wrong in this process, so this checklist should serve to help that.**
**0. We have ebuild foo-0.0.1-r100 with test and would like to split that test off into ebuild bar-0.0.1-r1.**
**Assume that:**
**- both ebuilds are using cros-workon (because it’s likely the case).**
**- foo is used globally (eg. autotest-all depends on it), rather than just some personal ebuild**
**1. Remove test from foo-{0.0.1-r100,9999}; uprev foo-0.0.1-r100 (to -r101)**
**2. Create bar-9999 (making a copy of foo and replacing IUSE_TESTS may be a good start), with IUSE_TESTS containing just the entry for test**
**3. Verify package dependencies of test. Make bar-9999 only depend on what is needed for test, remove the dependencies from foo-9999, unless they are needed by tests that remained.**
**4. Add a blocker. Since bar installs files owned by foo-0.0.1-r100 and earlier, the blocker’s format will be:**

**```none
RDEPEND=”!<=foo-0.0.1-r100”
```**

**5. Add a dependency to the new version of bar into chromeos-base/autotest-all-0.0.1**

**```none
RDEPEND=”bar”
```**

**6. Change the dependency of foo in chromeos-base/autotest-all-0.0.1 to be version locked to the new rev:**

**```none
RDEPEND=”>foo-0.0.1-r100”
```**

**7. Uprev (move) autotest-all-0.0.1-rX symlink by one.**
**8. Publish all as the same change list, have it reviewed, push.**

### W4. Create and run a test-enabled image on your device

1. Choose which board you want to build for (we'll refer to this as ${BOARD},
which is for example "x86-generic").

2. Set up a proper portage build chroot setup.

```bash
$ cros build-packages --board=${BOARD}
```

3. Build test image.

```bash
$ cros build-image --board=${BOARD} test
```

4. Install the Chromium OS testing image to your target machine. This is through
the standard mechanisms: either USB, or by reimaging a device currently running
a previously built Chromium OS image modded for test, or by entering the shell
on the machine and forcing an auto update to your machine when it's running a
dev server. For clarity we'll walk through two common ways below, but if you
already know about this, just do what you normally do.

4a. If you choose to use a USB boot, you first put the image on USB and run this
from outside the chroot.

```none
$ cros flash --board=${BOARD} usb:// chromiumos_test_image.bin
```

4b. Alternatively, if you happen to already have a machine running an image
modified for test and you know its IP address (${REMOTE_IP}), you can avoid
using a USB key and reimage it with a freshly built image by running this from
outside the chroot:

```none
$ ./image_to_live.sh --remote=${REMOTE_IP} --image=`./get_latest_image.sh --board=${BOARD}`/chromiumos_test_image.bin
```

This will automatically start dev server, ssh to your machine, cause it to
update to from that dev server using memento_updater, reboot, wait for reboot,
print out the new version updated to, and shut down your dev server.
