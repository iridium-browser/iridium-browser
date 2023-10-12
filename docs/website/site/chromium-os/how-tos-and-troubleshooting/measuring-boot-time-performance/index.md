---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: measuring-boot-time-performance
title: Tools for Measuring Boot Time Performance
---

This is a short summary of selected tools for measuring boot time performance.
The `bootperf` command is used to run boot time performance tests on a target
device. The `showbootdata` command can display average results from previous
runs of `bootperf`. The `bootstat_summary` command computes and displays timings
from a single boot.

Requirements for bootperf

### As of January, 2021, the `bootperf` wrapper uses tast instead of `test_that` (autotest). In any case, one is required to build a test image, put the target test device into developer mode, and install the test image on the targeted Chrome OS device. Once the test image is installed, attach the device to a wired ethernet network and obtain the IP address by clicking on the network selection icon in the status box with the battery status indicator.

### Verified boot and performance

A word of warning about verified boot: If `cros build-image` is invoked with the
`--no-enable-rootfs-verification` option, this will substantially impact
measured boot time: The option measurably speeds up boot.  To do a fair,
apples-to-apples comparison of two images, *one must build both images with the
same setting of the option*.

Also, the performance results observed with verification off are not comparable
to when verification is on, and vice-versa. The Chromium/Chrome OS boot time
requirements generally only apply to systems with root verification enabled.
**Claims a change makes things faster, or slower, or has no impact are NOT
proven until those changes are tested with root verification enabled and
compared to the same build version without those changes.**

Last tip: developer mode has a boot delay and beeps on each boot. To quietly
boot with a much shorter "developer screen" pause, set GBB flags to 0x39. On
most platforms (e.g. not on sarien) this can be done with:

```none
localhost ~ # /usr/bin/futility gbb --set --flash --flags=0x39
```

### What Constitutes a "Significant" Change

Statistically, the boot time performance numbers tend to have a high variance
across runs. Moreover, small changes can't be perceived by ordinary human users.
To claim that a particular change in performance is good (or bad), one generally
needs the following:

*   The "before" and "after" test samples should both include at least
            50 cycles.
*   The "before" and "after" results should show a change of at least
            0.1 seconds (100 ms) to the kernel to login screen time. Changes
            just to intermediate phases don't count.

Changes that don't reach this threshold can be considered performance neutral.
For changes clearly below the threshold, focus instead on software engineering
questions such as "will it fix a bug?", or "will it make the code easier to
maintain?".

An example with bootperf

### Running the test

The `bootperf` command is run from inside the Chromium OS chroot environment;
run `cros_sdk `before using the script. The command should be on the default
PATH inside the chroot.

```none
(cros-chroot) ~/trunk/src/scripts $ bootperf -o ~/bootdata/bootperf.sample 172.22.75.70 2
16:34:58 - /home/jrbarnette/bootdata/bootperf.sample/run.000/log.txt
16:40:18 - /home/jrbarnette/bootdata/bootperf.sample/run.001/log.txt
16:45:23
(cros-chroot) ~/trunk/src/scripts $ 
```

In the example above, the `-o` option specifies a directory where the test
results will be stored. If the directory doesn't exist, it will be created. The
first argument is the IP address of the target device; the second argument is
the number of test runs to execute.

Each test iteration will reboot the target device 10 times. One can expect
autotest to spend about 30 to 60 seconds per boot cycle, or 5-10 minutes per
iteration.

### Looking at results

After running `bootperf`, use the `showbootdata` command to display the results
in a usable form. The `showbootdata` command can be run from in- or outside of
the chroot; this example shows it from inside.

```none
(cros-chroot) ~/trunk/src/scripts $ showbootdata ~/bootdata/bootperf.sample
(on 20 cycles):
 time  s%     dt  s%  event
 2311  0%  +2311  0%  startup
 2587  1%   +276  5%  startup_done
 4420  1%  +1833  2%  x_started
 4464  1%    +44 11%  chrome_exec
 4482  1%    +18 41%  chrome_main
 5284  1%   +802  2%  login
 6207  9%   +923 60%  network
(cros-chroot) ~/trunk/src/scripts $ 
```

The output shows these columns:

*   Average total time in milliseconds
*   Sample standard deviation of the total time as a percentage of the
            average
*   The average time difference between the current line and the
            previous line's total
*   Sample standard deviation of the differences (over all boot
            iterations) between the current and previous lines, as a percent of
            the average
*   The name of the event (point in boot) at which the statistic was
            measured

The events listed in the output correspond to particular points in source code
where sample data is taken. The samples record accumulated time since kernel
startup; they don't include time spent in the firmware. The events correspond to
the following points in boot:

*   **startup**: This event marks the beginning of the
            `chromeos_startup` script. This script is triggered by the `startup`
            event, which is the first event emitted by `/sbin/init at boot. This
            timestamp represents (roughly) the moment when the system first
            starts running user processes.`
*   **startup_done**: This marks the point when `chromeos_startup`
            finishes running. This is roughly the moment of the `started
            boot-services` event.
*   **x_started**: This marks the point in `session_manager_setup.sh`
            when the X server has completed initialization, and is ready to
            accept requests. This is also the point at which `session_manager`
            starts running.
*   **chrome_exec**: This marks the point when `session_manager` starts
            the chrome browser.
*   **chrome_main**: This marks a point early in Chrome initialization,
            after starting `main()`.
*   **login**: This marks completion of the `boot-complete` upstart job,
            which is triggered when the chrome browser displays the login
            screen.
*   **network**: This marks the point when `shill` records that a
            default network route has been added.

### Other summaries

The boot performance tests gather more than just boot time data from kernel
startup. The `-k` option to `showbootdata` can be used to display various sets
of Autotest keyvals for the test:

*   `-k boot` (the default) shows boot time statistics measured from
            kernel startup.
*   `-k disk` shows disk read statistics (in millions of bytes) measured
            from kernel startup.
*   `-k firmware` shows firmware timings measured from power on.
*   `-k reboot` shows boot time statistics measured from the start of
            the shutdown prior to boot.

Below is an example with `-k reboot`:

```none
(cros-chroot) ~/trunk/src/scripts $ showbootdata -k reboot ~/bootdata/bootperf.sample
(on 20 cycles):
 diskrd  s%    delta  s%  event
   3569 19%    +3569 19%  shutdown
   4481 15%     +912  1%  firmware
   6791 10%    +2310  0%  startup
   8945  8%    +2154  1%  chrome_exec
   9765  7%     +820  2%  login
```

As with `-k boot`, the times are measured in milliseconds. The events correspond
to the following points in the reboot sequence:

*   **shutdown**: This marks the end of shutdown, when the system hands
            control back to the firmware to request reboot.
*   **firmware**: This marks the end of firmware startup and kernel
            decompression, when the kernel begins initialization.
*   **startup**, **chrome_exec**, **login**: These are the same as the
            boot time events described above.

### Raw results

To process the data with custom scripts, raw results can be extracted using the
`-r` option to `showbootdata.`

```none
(cros-chroot) ~/trunk/src/scripts $ showbootdata -r ~/bootdata/bootperf.sample
  0  2310  2560  4420  4470  4490  5290  6070
  1  2310  2580  4370  4420  4440  5250  6070
  2  2330  2600  4480  4520  4550  5360  6060
  3  2320  2610  4430  4480  4490  5270  6350
  4  2310  2600  4390  4440  4450  5230  6050
  5  2310  2580  4430  4470  4490  5280  6070
  6  2290  2550  4350  4400  4410  5220  6040
  7  2320  2590  4430  4470  4490  5300  6060
  8  2320  2620  4420  4460  4480  5270  6090
  9  2320  2590  4450  4500  4510  5330  6050
 10  2310  2600  4440  4480  4490  5330  6050
 11  2300  2590  4400  4450  4460  5260  6100
 12  2310  2580  4430  4470  4480  5290  6130
 13  2300  2570  4380  4420  4440  5230  6120
 14  2310  2580  4440  4480  4500  5290  6050
 15  2320  2600  4400  4450  4460  5260  8510
 16  2310  2570  4430  4480  4500  5300  6090
 17  2290  2570  4410  4450  4470  5260  6060
 18  2310  2590  4450  4490  4520  5350  6040
 19  2310  2600  4440  4480  4510  5310  6070
(cros-chroot) ~/trunk/src/scripts $ 
```

The first column is the iteration number; the remaining columns correspond to
the time sampled at the corresponding event. In this example there were twenty
boot time data points, so there are twenty lines of output. Note that the event
names (and order) are implicit.

Data displayed can be restricted to selected events using the `-e` option.

```none
(cros-chroot) ~/trunk/src/scripts $ showbootdata -e startup,chrome_exec,login ~/bootdata/bootperf.sample
(on 20 cycles):
 time  s%     dt  s%  event
 2311  0%  +2311  0%  startup
 4464  1%  +2153  1%  chrome_exec
 5284  1%   +820  2%  login
```

## Using bootstat_summary

The `bootstat_summary` command is a device-resident command used to show the
timings of events since the last boot. The command is different from `bootperf`
and `showbootdata` in that it is run on the target device, and it can't show an
average across multiple boots. The command is present in all images; a test
image is not required to use the command.

Here's a sample:

```none
chronos@localhost ~ $ bootstat_summary
 time %cpu    dt  %dt  event
 1060  22%  1060  22%  pre-startup
 1390  37%   330  86%  post-startup
 3260  72%  1870  97%  x-started
 3370  72%   110  89%  chrome-exec
 4090  76%   720  92%  chrome-main
 5620  65%  1530  37%  login-prompt-ready
 6460  64%   840  57%  network-ethernet-configuration
 6490  64%    30  50%  network-ethernet-ready
 6500  64%    10  50%  network-ethernet-online
 6540  64%    40  56%  network-ethernet-ready
 6550  64%    10  75%  network-ethernet-online
 8310  60%  1760  43%  login-prompt-visible
 8340  60%    30  83%  boot-complete
chronos@localhost ~ $                                                                                                                                                                          
```

The listed events include the events reported in `showbootdata` (using slightly
different names), plus some additional events. The meaning of the columns is
covered below:

*   `time`: Timestamp for the event, measured in milliseconds since
            boot.
*   `%cpu`: Total CPU utilization since boot.
*   `dt`: Time in milliseconds since the previous event
*   `%dt`: CPU utilization in the interval since the previous event.
*   `event`: The name of the event.

The events reported in `showbootdata` are a subset of the events reported by
`bootstat_summary`. Here's the correspondence between the two:

<table>
<tr>
<td> `bootstat_summary`</td>
<td> `showbootdata`</td>
</tr>
<tr>
<td> pre-startup</td>
<td>startup</td>
</tr>
<tr>
<td>post-startup</td>
<td>startup_done</td>
</tr>
<tr>
<td>x-started</td>
<td>x_started</td>
</tr>
<tr>
<td>chrome-exec</td>
<td>chrome_exec </td>
</tr>
<tr>
<td>chrome-main</td>
<td>chrome_main </td>
</tr>
<tr>
<td>boot-complete</td>
<td>login</td>
</tr>
<tr>
<td>network-ethernet-ready</td>
<td>network </td>
</tr>
</table>

N.B. The CPU utilization values can be inaccurate for short intervals (less than
a few hundred milliseconds). In some cases, one may even see negative
utilization. This is a limitation in the underlying kernel code that produces
the statistics.
