---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: tsc-resynchronization
title: TSC resynchronization
---

# Sergiu Iordache - sergiu@chromium.org / sergiu.iordache@gmail.com

with the help of Terry Lambert - tlambert@chromium.org

## 1. Introduction & goal

This document describes the work done in order to enable TSC as a clocksource on
Intel Chromebooks (affects Mario, Alex and ZGB as they have constant TSC).

## 2. Background

The Linux kernel uses different time sources. The most interesting are the HPET
(High Precision Event Timer) and the TSC (Time Stamp Counter).
The TSC is the preferred clocksource between the two counters, as it is the
fastest one, however it can only be used if it is stable. Currently there are 4
types of TSC present:

1.  Constant. Constant TSC means that the TSC does not change with CPU
            frequency changes, however it does change on C state transitions.
2.  Invariant. As described in the Intel manual: “The invariant TSC will
            run at a constant rate in all ACPI P-, C- and T-states”
3.  Non-stop. The Non-stop TSC has the properties of both Constant and
            Invariant TSC.
4.  None of the above. The TSC changes with the C, P and S state
            transitions.

By looking into the flags entry in /proc/cpuinfo we can determine what type of
TSC the current processor has (possible values we check for are constant_tsc,
nonstop_tsc)
The constant TSC is the type of TSC currently present on our Chromebooks (future
Chromebooks may have non-stop TSC). Therefore the TSC stops during C-state
transitions and must be resynchronized in order to track time again.
[A patch](https://lkml.org/lkml/2008/9/25/451) written by Michael Davidson
(md@google.com) currently exists which enables TSC resynchronization for
userspace uses on AMD architectures. In that patch the TSC gets synchronized
only for userspace calls.

## 3. My work

Although the patch by md@ does a lot of the heavy work, it can’t be applied
cleanly on our configuration for 2 reasons:

1.  it was built for 64 bit architectures;
2.  it was built solely for user-space users of the TSC (the clock
            source would still be set to something else).

My work to date updates the patch so that it can be used on our architecture, by
modifying the code to work on 32 bit and making the necessary changes so that
the TSC is used as a kernel clock source.
I’ve also managed to improve synchronize the TSCs between the CPUs within 300
cycles of each other.
The current results and limitations are:

*   The TSC on CPU 0 initially gets synchronized to the HPET.
*   The TSC is synchronized across CPUs as they are brought up (relative
            to CPU 0), resulting in a difference of less than 300 cycles between
            the TSC value on each of the CPUs;
*   The TSC is set as the main clock source; The drift requirements had
            to be relaxed in order for this to work from not permitting any
            drift to permitting a drift of a few hundred cycles.
*   The TSC syncs using the HPET on transitions from C0 to C3+ (The
            Linux kernel groups all C-state larger than C3 into C3) ;
*   The TSC can be set to periodically resync (by default every 100ms)
            to the HPET.

The current problem is that we’re the synchronization induces a warp of a few
thousand cycles, which is unacceptable. I’ve been working very hard with Terry
during the last week of my internship to make this go away and apparently the
scaling factor is one of the causes, along with the way the TSC gets updated
from the HPET. However, I am still working to find a complete solution to this
problem.

## 4. Benchmarks

Although using the TSC as a clocksource all the time is not completely
implemented, I was able to benchmark its use by ignoring C-state transitions(by
keeping the power adapter plugged while testing). While benchmarking the TSC as
a clock source, 2 main aspects were examined:

1.  boot time;
2.  improvements to gettimeofday and clock_gettime.

### 4.1. Boot time

<img alt="image"
src="https://lh4.googleusercontent.com/goxMnnDRXuu_j_4aHBRP_B4aOPiwFUWKYF_EbCUJq4xldKLH0_ro8VRt0SeMlbYz6MOogYrk-e424V9Cvx3Md1oHS-b9y3qUYd3Wms1Xi0i5ZH27tz8"
height=371px; width=600px;>

Boot time was measured using the bootperf autotests. The setup was the
following:

*   x86-alex overlay
*   Top of the tree on Monday, 08/26/2011 for HPET
*   The same + TSC patch for TSC
*   3 sets of 10 runs for each

The results are presented on the right;
The boot time with the TSC counter is just a little bit slower on these 30 runs,
with an average of 5.77 for the TSC version versus 5.73 for the HPET one. This
is explicable as the extra initialization (there is also an extra calibration
loop for a few ms) adds time which might cancel any improvements.

### 4.2. gettimeofday benchmark

The efficiency of gettimeofday was measured by doing a simple benchmark, calling
gettimeofday for 100.000.000 times. The benchmark was run on the same Alex using
a HPET and a TSC clocksource and the results were:

*   HPET: 140 s
*   TSC: 46 s

### 4.3. Number of calls to gettimeofday

Philip Guo, another intern, created a strace++ tool which counts system calls.
He used it to run Chrome on ChromeOS and get the number of times gettimeofday
gets called. On his run of Chrome with a 90 second YouTube video playing it got
called 18195 times. These calls, according to my benchmark would take 0.0254
seconds using the HPET and 0.008 seconds using the TSC (which means that 0.017
seconds were saved during a 90 second period, or just 0.01 % improvement). It
would be interesting to see what happens for non-optimized flash websites.
However real benchmarks would need to be run after the synchronization works
completely.

## 5. Future work

Here are the things that still need to be done after my internship is over:

*   Fixing the TSC resync such that it works. Terry suggested the
            following steps:
    *   Making the HPET to TSC routine that works when SMIs happen.
    *   Finding out the cost of the HPET read (keeps working when SMIs
                happen)
    *   Wait for the HPET clock to tick before syncing
    *   Set the TSC to the corresponding value + cost of HPET
*   Formally verify that the precision of the clock source is preserved;
*   Terry suggested syncing the value of the TSC with the real time from
            the epoch so that you would only need to apply a scale to it and not
            an offset -&gt; you could move it to user space
