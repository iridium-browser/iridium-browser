---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: running-smoke-suite-on-a-vm-image
title: Running Smoke Suite On a VM Image
---

[TOC]

## Introduction

The VMTest "smoke suite" is a suite of client-side autotests that performs basic
sanity checks on a Chromium OS build.

Each of the tests in the smoke test suite is capable of running in a virtual
machine.

This allows the smoke suite to be run as part of the automated build process on
a 'builder'.

## Smoke Suite tests

The Smoke suite currently consists of the following tests (this list is subject
to change):

*   build_RootFilesystemSize
*   desktopui_ChromeFirstRender
*   desktopui_FlashSanityCheck
*   desktopui_KillRestart('^chrome$')
*   desktopui_KillRestart('^session_manager$')
*   logging_CrashSender
*   login_Backdoor('$backdoor')
*   login_BadAuthentication
*   login_CryptohomeIncognitoMounted('$backdoor')
*   login_CryptohomeMounted
*   login_LoginSuccess
*   platform_FilePerms
*   platform_OSLimits

## Running the Smoke Test Suite on a Virtual Machine (VM)

### Useful Links:

The instructions on this page should be enough to get you going. They install a
testable Chromium OS image on a VM and run the smoke suite. However, if you want
more details, you can always go to one of the following pages:

*   Building a testable VM image is much like [building other Chromium
            OS images](/system/errors/NodeNotFound).
*   However, some extra steps are needed to [build a test
            image](/system/errors/NodeNotFound).
*   Once built, a clone of this [image should be created to run in a
            VM](http://www.chromium.org/chromium-os/developer-guide#TOC-Building-an-image-to-run-in-a-virtu).
*   Here are some instructions to [run the image on a
            VM](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/cros_vm.md).
*   Here's how to build and run [autotest tests on the
            VM](/system/errors/NodeNotFound).

### Install qemu-kvm

First of all, to run in a vm, you must have vm software installed.

On Ubuntu Lucid, use apt-get to install qemu-kvm:

<pre><code><b>$</b> sudo apt-get install qemu-kvm
</code></pre>

### Build a testable VM image

WARNING: After
[crbug/710629](https://bugs.chromium.org/p/chromium/issues/detail?id=710629),
'betty' is the only board regularly run through pre-CQ and CQ VMTest and so is
the most likely to work at ToT. 'betty' is based on 'amd64-generic', though, so
'amd64-generic' is likely to also work for most (non-ARC) tests.

Next, build the testable VM image. The following steps assume that:

1.  depot_tools/repo have been installed and configured.
2.  Chromium OS source code has been checked out.
3.  A chroot has been created with `cros_sdk --create`.

``` bash
# Build all the source packages, including those required for running autotests
# Optional: "--use-any-chrome" may make your build faster, but will have old
# Chrome.
# NOTE: At the moment, "--with-autotest" is the default, so don't worry if you
# built without it.
(outside) $ cros build-packages --board=${BOARD}

# Build a bootable image
# Optional: Include "--no-enable-rootfs-verification" if you think you might
# need to modify your rootfs.
(outside) $ cros build-image --board=${BOARD} test

# Clone this image, modify it for test, and make image for use in qemu-kvm
# Virtual Machine
# Note: because we use "--test_image", an explicit "modify_image_for_test" is
# not required.
(chroot) $ ./image_to_vm.sh --board=${BOARD} --test_image
```

The newly created VM image should be in the following path (relative to
src/scripts):

```none
../build/images/${BOARD}/latest/chromiumos_qemu_image.bin
```

### Start a VM image

Before launching autotests, its fun to boot up a virtual Chromium OS on your
desktop!

<pre><code>
<b>(chroot)$</b> cros_vm --start --board=${BOARD}
</code></pre>

### Run the autotest tests on the VM image

Autotests can be run against the VM via test_that.

<pre><code><i># Run the entire smoke suite
</i> <b>(chroot)$</b> test_that --board=${BOARD} localhost:9222 suite:smoke
<i># Run an individual test from the smoke Suite
<b>(chroot)$</b> test_that --board=${BOARD} localhost:9222 logging_CrashSender</i>
</code></pre>

Don't forget to kill the head-less VM when you are done with it!

<pre><code>
<b>(chroot)$</b> cros_vm --stop
</code></pre>

### Run the autotest tests on the VM image using cros_run_vm_test

Another script, cros_run_vm_test, can be used to start the VM, run autotest
tests on it, and then shut down the VM, all from a single invocation:

<pre><code>
<i># Run the entire smoke suite:</i>
<b>(chroot)$</b> mkdir results
<b>(chroot)$</b> cros_run_vm_test --board=${BOARD} --results-dir=results --autotest suite:smoke
<i># Run an individual test from the suite</i>
<b>(chroot)$</b> cros_run_vm_test --board=${BOARD} --results-dir=results --autotest platform_OSLimits
</code></pre>

### Troubleshooting

#### No space left on device

If you get an error that looks like this:

```none
Unhandled OSError: [Errno 28] No space left on device: '/home/autotest/tmp/_autotmp_Woz_Qyharness-fifo'
```

...this may be because you have run out of space on your "stateful partition".
This can happen if you leave the VM running for a long time and errors fill up
the /var/log folder. In my case, this folder contained 250M of data! You should
go into /var/log and cleanup stuff.

Use `df` to determine how much space is available in the stateful partition:

<pre><code>
<b>localhost ~ #</b> df /mnt/stateful_partition
<b>Filesystem           1K-blocks      Used Available Use% Mounted on</b>
<b>/dev/sda1              2064208    450612   1508740  23% /mnt/stateful_partition</b>
</code></pre>

`stop <daemon_name>.`You may be getting lots of errors if you're running a board-specific image with drivers for hardware that is not present on the VM. If this is the case, try stopping the responsible daemon with

#### Could not set up host forwarding rule 'tcp::9222-:22'

If you get this error when running cros_vm --start, make sure that you don't
have a VM already running. This should kill it for you.

```none
cros_vm --stop
```

#### Could not initialize KVM, will disable KVM support

This is probably because you don't have the kvm module loaded. The VM will
run.....very.......slowly.

```none
open /dev/kvm: No such file or directory
Could not initialize KVM, will disable KVM support
$ modprobe kvm_intel    # if you have an AMD machine, use 'kvm_amd'
```

Note that this module interferes with VirtualBox so you may need to remove it (`rmmod kvm_intel`) if you want to use this.

## Updating a client autotest

Autotest tests are usually built and compiled on the autotest server machine.

A 'client' test is installed on the client by the autotest infrastructure when
the test is run.

The autotest infrastructure is maintained in the **chromeos-base/autotest**
cros_workon
[ebuild](http://www.gentoo.org/proj/en/devrel/handbook/handbook.xml?part=2&chap=1).

Individual tests and test suites are in the **chromeos-base/autotest-tests**
ebuild.

### The new way: Use cros_workon with all Python tests

Working on all Python autotests should be simple and fast.

First `cros_workon` `autotest` for the board on which you plan to run tests:

<pre><code>
<b>(chroot)$</b> cros_workon start autotest --board=${BOARD}
</code></pre>

Now, test_that will detect that you are working on autotests and automatically
copy over Python in the autotest source tree to the sysroot.

Run test_that as usual:

<pre><code><b>(chroot)$</b> test_that --board=${BOARD} localhost:9222 suite:smoke
</code></pre>

### The old way: emerge individual autotest tests

If you have a test with binary dependencies, you must rebuild them explicitly:

<pre><code><b>(chroot)$</b> emerge-${BUILD} autotest-tests
</code></pre>

This can take a very, very long time since it will rebuild ALL tests.

To speed this up, set the TESTS variable on the command line when runnning
emerge to install an individual test:

<pre><code>
<b>(chroot)$</b> TESTS=&lt;TestName&gt; emerge-${BOARD} autotest-tests
</code></pre>

Note that doing this will deactivate all other tests! This is a problem for test
suites.

To rebuild a whole test suite, specify the suite and all individual tests in the
suite. For example, to rebuild smoke suite:

<pre><code>
<b>(chroot)$</b> TESTS='build_RootFilesystemSize desktopui_ChromeFirstRender \
desktopui_FlashSanityCheck desktopui_KillRestart logging_CrashSender \
login_Backdoor login_BadAuthentication login_CryptohomeIncognitoMounted \
login_CryptohomeMounted login_LoginSuccess platform_FilePerms platform_OSLimits' \
emerge-${BOARD} autotest-tests
</code></pre>

Then go grab a coffee...

### Debugging

You can use [chromium os debugging
tips](/chromium-os/how-tos-and-troubleshooting/debugging-tips#TOC-We-recommend-that-developers-use-gd)
on VM as well.
