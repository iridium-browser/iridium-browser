---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: multi-process-ram-usage-analysis-on-android
title: Multi-Process RAM usage analysis on Android
---

## Overview

Measuring RAM usage of a program should take in account not only memory heap,
but also other memory, such as memory-mapped files or memory shared among the
processes. There is a built-in Android debug tool **procrank** that displays one
useful number for a process: "Uss" (Unique Set Size), rough amount of *private*
memory (such as C++/V8/Dalvik heaps). Procrank also reports "Pss" (Proportional
Set Size) that takes into account shared memory, but it is hard to make any
conclusions based on this number. Chrome on Android is a multi-process
application where browser process shares quite a lot of memory with the renderer
processes (for example, read-only code).

The new set of tools in the chromium tree estimate the overall RAM usage of all
processes (browser and renderers), as well as provides more fine-grained
analysis for the running browser by type or memory mapping: private, shared
read-only, shared read-write, etc. The HOWTO below briefly describes the steps
to use the tools.

## HOWTO

**Step 1:** build the memdump tool and deploy it on the device. The tools will
be used to analyse memory mappings by looking at /proc/PID/pagemap,
/proc/PID/maps for given process IDs, and also /proc/kpagecount.

<pre><code>
adb remount
adb root
ninja -C out/Release <b>memdump</b>
adb push out/Release/memdump /data/local/tmp/
</code></pre>

**Step 2:** Build the browser with the 'dlmalloc hack'
(<https://codereview.chromium.org/16514009>). It is the allocator used in
Android by default, except it is modified to perform all internal mappings using
ashmem, which makes it easier to distinguish the heap memory from other
mappings.

Given a checkout of chromium sources patch them with the hack, enable the build
flag and build the browser:

<pre><code>
git cl patch 16514009
GYP_DEFINES="<b>android_use_dlmalloc=1</b> $GYP_DEFINES" gclient runhooks
# Build and deploy the browser apk as usual.
</code></pre>

Steps 1 and 2 should be performed only once for a browser build.

**Step 3:** when the browser is started, use a little shell scripting to select
the PIDs of all processes belonging to the browser (kill all other browser
instances prior to this step), feed the memdump report to the analyser:

```none
adb shell /data/local/tmp/memdump $(adb shell ps | grep chrome | awk '{ print $2; }') | python tools/android/memdump/memreport.py
```

An example output it produces:

<pre><code>
,Process 1,private,shared_app,shared_other,
,Total,91.7734375,54.71875,46.06640625,
,Read-only,2.94140625,1.13671875,9.36328125,
,Read-write,84.80078125,45.58203125,31.984375,
,Executable,4.03125,8.0,4.71875,
,File read-write,78.28125,45.58203125,31.21875,
,Anonymous read-write,6.47265625,0.0,0.76171875,
,File executable,4.03125,8.0,4.71875,
,Anonymous executable (JIT'ed code),0,0,0,
,chromium mmap,0,0,0,
,chromium TransferBuffer,0,0,0,
,Galaxy Nexus GL driver,65.73828125,0.0,20.05859375,
,Dalvik,5.67578125,0.0,10.84375,
,Dalvik heap,3.55078125,0.0,8.0625,
,Native heap (jemalloc),0,0,0,
,Native heap (dlmalloc),<b>4.50390625</b>,0.0,0.0,
,System heap,0,0,0,
,Ashmem,2.01171875,45.58203125,0.0,
,libchromeview.so total,5.234375,7.9453125,0.0,
,libchromeview.so read-only,1.2421875,0.0,0.0,
,libchromeview.so read-write,0.14453125,0.0,0.0,
,libchromeview.so executable,3.84765625,7.9453125,0.0,
,Process 2,private,shared_app,shared_other,
,Total,20.5859375,54.71875,21.16796875,
,Read-only,1.3125,1.13671875,5.71484375,
,Read-write,11.6796875,45.58203125,13.1640625,
,Executable,7.59375,8.0,2.2890625,
,File read-write,8.45703125,45.58203125,12.140625,
,Anonymous read-write,3.83203125,0.0,1.0078125,
,File executable,6.9765625,8.0,2.2890625,
,Anonymous executable (JIT'ed code),0.6171875,0.0,0.0,
,chromium mmap,0,0,0,
,chromium TransferBuffer,0,0,0,
,Galaxy Nexus GL driver,0,0,0,
,Dalvik,1.5,0.0,11.7734375,
,Dalvik heap,0.64453125,0.0,8.41796875,
,Native heap (jemalloc),0,0,0,
,Native heap (dlmalloc),6.76171875,0.0,0.0,
,System heap,0,0,0,
,Ashmem,0.0,45.58203125,0.0,
,libchromeview.so total,8.35546875,7.9453125,0.0,
,libchromeview.so read-only,1.2421875,0.0,0.0,
,libchromeview.so read-write,0.14453125,0.0,0.0,
,libchromeview.so executable,6.96875,7.9453125,0.0,
,Total,167.078125,
,Read-only,5.390625,
,Read-write,142.0625,
,Executable,19.625,
,File read-write,132.3203125,
,Anonymous read-write,10.3046875,
,File executable,19.0078125,
,Anonymous executable (JIT'ed code),0.6171875,
,chromium mmap,0,0
,chromium TransferBuffer,0,0
,Galaxy Nexus GL driver,65.73828125,
,Dalvik,7.17578125,
,Dalvik heap,4.1953125,
,Native heap (jemalloc),0,0
,Native heap (dlmalloc),11.265625,
,System heap,0,0
,Ashmem,47.59375,
,libchromeview.so total,21.53515625,
,libchromeview.so read-only,2.484375,
,libchromeview.so read-write,0.2890625,
,libchromeview.so executable,18.76171875,
</code></pre>

Only information about the **pages resident in memory** is printed. The "Process
1" is typically the browser process, others are renderer processes. For example,
the value in bold shows that the browser process has allocated ~4.5 MB heap
memory. The browser also has loaded almost 4MB of executable code during
execution.
