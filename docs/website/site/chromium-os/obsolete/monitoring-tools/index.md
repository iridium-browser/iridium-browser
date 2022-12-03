---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/obsolete
  - Obsolete Documentation
page_name: monitoring-tools
title: monitoring tools
---

We can use two tools to monitor the system:

1.  [streamline](http://www.arm.com/products/tools/software-tools/ds-5/streamline.php)
2.  systemtap

**Streamline**
For the ARM platform, we can use streamline to monitor the activities in the
system. It can give us a timeline of utilization of the system sources such as
CPU load, cache, interrupts, disk IO, memory, network, etc. It can also display
the CPU utilization of each process.
It requires two components:

1.  one works on the target device to monitor the system
2.  one works on our desktop computer to display the captured
            activities, which is a plugin in eclipse.

In order to capture the activities in the target device, we need to run a deamon
called ***gatord*** on the target device, which requires a kernel module.
In order to build the modules, we need to build the kernel first. The easiest
way is:
FEATURES=noclean emerge-tegra2_seaboard chromeos-kernel
The kernel will be built in
/build/tegra2_seaboard/tmp/portage/sys-kernel/chromeos-kernel-9999/work/chromeos-kernel-9999/build/tegra2_seaboard.
Then we run
make -C
/build/tegra2_seaboard/tmp/portage/sys-kernel/chromeos-kernel-9999/work/chromeos-kernel-9999/build/tegra2_seaboard/
M=\`pwd\` ARCH=arm CROSS_COMPILE=armv7a-cros-linux-gnueabi- modules
After building the kernel module, we need to copy gatord and the kernel module
to the target device, and they must be placed in the same directory.
[ARM DS-5 Using ARM
Streamline](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0482b/index.html)
has more detailed information about building and using streamline.
In order to profile ChromeOS during its bootup, we don't have enough time to set
the Eclipse for DS-5 to connect to ChromeOS, and capture the profiling result.
Instead, we can run the daemon in a "local capture" mode, i.e., we can run the
daemon on the target device and it stores the captured result in the local disk,
and we can fetch the result after ChromeOS boots up. We need to provide gatord a
xml file to specify options for the "local capture" mode. A xml file can be as
below:
&lt;?xml version="1.0" encoding="US-ASCII" ?&gt;
&lt;session version="1" title="local-test" call_stack_unwinding="yes"
buffer_mode="streaming" sample_rate="normal" target_path="/tmp/@F_@N"
duration="10"/&gt;
More information about "local capture" can be find at
[here](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka14991.html).
Here is the result of a local capture after post-startup, where ureadahead
starts running. Streamline captures the system information for 10 seconds and
the boot takes about 5 seconds to complete after post-startup. From streamline,
we can see that disk IO is constantly busy during boot, but the throughput isn't
high, about 5-10MB/s. Meanwhile, the CPU load isn't high either after 2 seconds.
ChromeOS uses squashfs as the root filesystem during this boot, and the best
read performance seen on this device is about 20Mb/s, which means we potentially
have resources to increase the IO performance to improve the boot performance.
From this snapshot, we can also observe that the CPU frequency goes up and down
a little bit because cpuidle is enabled, which might also hurt the boot
performance.
At the bottom, streamline shows the status of all processes and threads, which
tells us the valuable information such as which process is using the most of CPU
at some particular time.

[<img alt="image"
src="/chromium-os/obsolete/monitoring-tools/timeline.jpg">](/chromium-os/obsolete/monitoring-tools/timeline.jpg)

**Systemtap**
systemtap is another great tool to monitor the kernel and user space programs.
systemtap isn't well supported in the ARM platform, and a few patches are
required in order to install systemtap ChromeOS. A few changes to help build
systemtap for ChromeOS have been submitted for review, and hopefully to be
merged to the upstream very soon.
systemtap requires a compiler to build scripts to kernel modules. Since ChromeOS
doesn't have gcc built in by default, it's easier to build the systemtap scripts
on the desktop.
Before we can build the scripts, we need to cross compile the kernel, and we can
build the kernel as mentioned in the above.
FEATURES=noclean USE="systemtap" emerge-tegra2_seaboard chromeos-kernel

The command above builds a kernel for seaboard, and we need to enable the kernel
to support systemtap.
All compiled result will be in
/build/tegra2_seaboard/tmp/portage/sys-kernel/chromeos-kernel-9999/work/chromeos-kernel-9999/build/tegra2_seaboard.
KERNEL_BUILD_DIR=/build/tegra2_seaboard/tmp/portage/sys-kernel/chromeos-kernel-9999/work/chromeos-kernel-9999/build/tegra2_seaboard

use stap to generate the module code. The following command translates a
systemtap script and cross compiles it into a kernel module called example.ko
for the ARM platform.

stap -v -a arm -r ${KERNEL_BUILD_DIR} -p 4 -B
CROSS_COMPILE=armv7a-cros-linux-gnueabi- -k example.stp -m example

copy the kernel module to the target device.

In the target device, assume the module is placed in /mnt/stateful_partition,
the following command runs the kernel module with staprun.

SYSTEMTAP_STAPIO=/usr/local/libexec/systemtap/stapio staprun
/mnt/stateful_partition/example.ko

The systemtap website lists a list of examples:
<http://sourceware.org/systemtap/examples/>
Some systemtap doc
[SystemTap Beginners
Guide](http://sourceware.org/systemtap/SystemTap_Beginners_Guide/)
[Systemtap tutorial](http://sourceware.org/systemtap/tutorial/)
At some point of boot, streamline shows that both CPU and disk were idle.
Therefore, a script as below was written to monitor all system calls, and print
out the time a system call takes to complete. This information might tell us
which processes were blocking the system at some particular moment as some
system calls took several hundred milliseconds or even one second to complete.
However, the problem was gone after I sync'd all software in the system, and the
further investigation becomes unnecessary.
global start_time
global syscall_starttimes
probe begin {
start_time = gettimeofday_us()
printf ("monitor starts at %d\\n", start_time)
}
probe syscall.\* {
if (pid() != stp_pid()) {
syscall_starttimes\[name, tid()\] = gettimeofday_us()
}
}
probe syscall.\*.return {
if (pid() != stp_pid()) {
curr_time = gettimeofday_us()
delta = curr_time - syscall_starttimes\[name, tid()\]
if (curr_time &gt; start_time + 1000000 \* 3)
printf ("%d: %s (%d:%d) returns from %s, takes %d\\n", curr_time - start_time,
execname(), pid(), tid(), name, delta)
delete syscall_starttimes\[name,tid()\]
}
}
probe timer.s(10) {
print ("have monitored for 10 seconds\\n")
exit()
}
**NOTE: systemtap isn't built in the dev image or the test image by default.**
Right now, we need to apply the following patch manually in order to build
systemtap into the dev image.
git fetch
http://gerrit.chromium.org/gerrit/p/chromiumos/overlays/chromiumos-overlay
refs/changes/74/5274/4 && git checkout FETCH_HEAD
Furthermore, in the chroot, systemtap requires a high version of elfutils, but
its dependency check is broken, so when running systemtap, please make sure
elfutils-0.152 is installed, which has been tested to work OK with systemtap
1.5. Without a higher version of elfutils, systemtap might have an error such as
semantic error: failed to retrieve return value location for vfs_write
