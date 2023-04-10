---
breadcrumbs:
- - /for-testers
  - For Testers
page_name: saft
title: SAFT
---

[TOC]

### Theory of operation

SAFT stands for Semi Automated Firmware Test. It is a Python and BASH based
Linux application enabled in the [ChromeOS](/chromium-os) test distribution. On
the target system SAFT code is placed in `/usr/sbin/firmware/saft. The source
code is controlled by a separate [git](goog_880557684)`
`[repository](http://gerrit.chromium.org/gerrit/gitweb?p=chromiumos%2Fplatform%2Fsaft.git;a=shortlog;h=HEAD).
`

SAFT is designed to verify BIOS functionality, including different aspects of
[Verified Boot](/chromium-os/chromiumos-design-docs/verified-boot) and [Google
Chrome OS Firmware - High Level
Specification](https://docs.google.com/a/google.com/Doc?docid=0AbUTgMyqR_XeZGRuaDd4OXdfODJkbTRjZzJjdA&hl=en).
To test this thoroughly multiple system restarts are needed, some including
booting in recovery mode. This is why **SAFT requires the flash drive to be
present on the system under test**, both to keep state between reboots and to
provide recovery mode boot medium. Recovery mode boot requires operator
involvement (unplugging and plugging in the flash drive), which makes SAFT a
'semi automated' test. SAFT is meant to be run by the firmware vendors, and is
required to be easy to set up and run. This is why it is not using the autotest
framework, all what's needed is a target netbook and a flash memory stick.

The Chrome OS root file system is immutable, so there is a need to enable or
disable SAFT execution without modifying the root fs contents. The SAFT
[upstart](http://upstart.ubuntu.com/getting-started.html) script
`/etc/init/saft.conf` is executed on every reboot. It checks if the flash drive
is present, and a "follow up" script `/var/saft.sh` exists on it. If the follow
up script does not exist, SAFT execution terminates, if it does exist, the
follow up script is invoked. This allows to enable/disable SAFT execution
without modifying the root file system.

When SAFT is not running, the upstart script terminates after logging the state
in `/tmp/saft.conf`:

```none
localhost ~ # cat /tmp/saft.log
Found sdc after 0 seconds
No SAFT in progress
```

To enable SAFT execution on a system, the 'follow up' script is created and the
system is reset. On the next and all following startups until the 'follow up'
script is deleted, SAFT application is invoked.

SAFT needs some means of representing the target boot state (what firmware and
kernel were used, what was the reason for reboot, where the root file system is
hosted, etc.). A so called boot state vector is used for that, which is a string
described as follows in the source code:

```none
The string has a form of x:x:x:<removable>:<partition_number>, where x' represent contents of the appropriate BINF files as reported by ACPI, <removable> is set to 1 or 0 depending if the root device is removable or not, and <partition number> is the last element of the root device name, designating the partition where the root fs is mounted. 
This vector fully describes the way the system came up.
```

When executed on start up, the 'follow up' script creates the necessary
environment and starts the actual SAFT Python application located in` <flash
drive mount point>/usr/sbin/firmware/saft/saft_utility.py`. This application is
a state machine, where each state introduces a unique defect in the system
(corrupted firmware or kernel, different CGPT attributes, etc., etc.).

In each step the application checks if the current boot state vector matches the
expected state, and if so corrects the previously introduced defect, introduces
the next one and reboots again. This continues until all steps are passed,
provided every state is processed properly. If SAFT application detects that the
target state vector does not match the expected state, it terminates
immediately. The sequence of defect introducing actions is stored in the
`saft_utility.py:TEST_STATE_SEQUENCE` table.

On termination the SAFT application removes the 'follow up' script and copies
the SAFT log into the `/var/fw_test_log.txt`, which allows the operator to
examine the test results.

### Running SAFT

Two important conditions have to be met before SAFT can run properly:

*   both kernels/root fs pairs need to be populated on the main storage
            device
*   the flash device needs to be configured not to use verified root fs,
            because it gets mounted in course of SAFT execution, which causes
            verified root fs control structures to go out of sync.

Neither of these conditions are met by default in the recent ChromeOS
distributions, so before SAFT can run, they need to be taken care of. The SAFT
wrapper script` /usr/sbin/firmware/saft/runtests.sh` does that. The wrapper
script checks if both partiton pairs exist, and if not - it duplicates the firs
pair on the second pair's devices (`/dev/sda2` gets copied to `/dev/sda4` and
`/dev/sda3` gets copied to `/dev/sda5`) and checks the command line of the flash
device hosted recovery kernel. If it is configured to run with verified root fs,
the kernel command line is edited to bring the recovery image up without
verified root fs protection.

So, a typical procedure to invoke SAFT is as follows:

*   download ChromeOS test distribution and put it on the flash drive
*   install ChromeOS on the target using the flash drive
*   reboot the target, enter the shell dialog and type

```none
sudo /usr/sbin/firmware/saft/runtests.sh [<BIOS image to test>]
```

If `runtests.sh` is invoked without any parameters, it just verifies the
environment (modifies it as described above, if required), copies SAFT code into
the removable device (into` <flash drive mount point>/usr/sbin/firmware/saft)`
and runs SAFT unit tests. The actual SAFT starts if `runtests.sh` is invoked
with a command line parameter, the name of the file containing the firmware
image to test.

It is presumed that the flash device (carrying the recovery image from which the
system has been installed) is plugged in into the target and gets instantiated
as `/dev/sdb`. If the flash device is plugged in into a USB hub or is
instantiated as a different device for some other reason, the environment
variable `FLASH_DEVICE `can be used to communicate it to the script, for
instance:

```none
FLASH_DEVICE=sdc /usr/sbin/firmware/saft/runtests.sh [<BIOS image to test>]
```

When invoked with the BIOS image file name, it is verified that the image has
proper keys in it and would be able to bring up the existing kernels. If
verification succeeds, SAFT gets under way, rebooting the target a few times (as
many times as there are steps in `saft_utility.py:TEST_STATE_SEQUENCE).`

Certain steps take a long time between restarts (for instance, when a step
involves reprogramming of the flashrom). It is easy to tell if SAFT is completed
or not by examining the SAFT temp file in `/tmp/saft.log`. When SAFT completes,
this file would have either an error message or the phrase 'we are done' in the
end. This file includes only messages generated since the most recent startup.
The full SAFT log can be found in `/var/fw_test.log.txt **after** the test
completes.`

### `Tests implemented so` far

The following tests are implemented as of commit `6cabd80 Introduce gpt tests in
SAFT`. (make sure to update this string when editing this section).

<table>
<tr>
<td> Test</td>
<td colspan=3> System state after reboot</td>
</tr>
<tr>
<td> Firmware</td>
<td> Kernel</td>
<td> GPT attributes</td>
</tr>
<tr>
<td> Set 'try FW B' in NV ram</td>
<td> B</td>
<td> A </td>
</tr>
<tr>
<td> Corrupt FW A</td>
<td> B</td>
<td> A</td>
</tr>
<tr>
<td> Restore FW A</td>
<td> A</td>
<td> A</td>
</tr>
<tr>
<td> Corrupt both FW A and B</td>
<td> Recovery</td>
<td>Recovery</td>
</tr>
<tr>
<td> Restore FW A and B</td>
<td>A</td>
<td> A</td>
</tr>
<tr>
<td> Corrupt kernel A</td>
<td> A </td>
<td> B</td>
</tr>
<tr>
<td> Corrupt kernel B</td>
<td> Recovery</td>
<td>Recovery</td>
</tr>
<tr>
<td> Restore kernels A and B</td>
<td> A</td>
<td> A</td>
</tr>
<tr>
<td> Set GPT A\[S:0 T:15 P:10\] B\[S:0 T=15 P=9\]</td>
<td> A</td>
<td> A</td>
<td>A\[S:0 T:14 P:10\] B\[S:0 T=15 P=9\] </td>
</tr>
</table>

### Enhancing SAFT

It is easy to add new steps to SAFT by extending
`saft_utility.py:TEST_STATE_SEQUENCE` table with new steps. The format of the
table is simple, each line is a tri-tuple of

```none
(<boot_state_vector>, <function>, <argument>),
```

where `boot_state_vector` is the expected boot state after the previous step,
and `function` with argument `argument` will implement the new step of SAFT.

### Interpreting SAFT errors

One of the common SAFT errors is 'wrong boot vector'
