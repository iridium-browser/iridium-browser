---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/packages
  - packages
- - /chromium-os/packages/crash-reporting
  - Crash Reporting (Chrome OS System)
page_name: debugging-a-minidump
title: Debugging a Minidump
---

[TOC]

Reference:
<https://sites.google.com/a/google.com/chrome-msk/dev#TOC-Crash-dump-analysis>

This page discusses how to debug a **ChromiumOS minidump**.

*   If you instead have a **Linux Chrome minidump**, see [Linux Minidump
            To
            Core](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux/minidump_to_core.md).
*   If you instead have a **Windows Chrome minidump**, just load the
            minidump into Visual Studio or windbg, set up the Chrome symbol
            server and Microsoft symbol server, and enable source indexing.
            Instructions can be found on the [Debugging Chromium on
            Windows](http://www.chromium.org/developers/how-tos/debugging-on-windows)
            page.
*   For other thoughts on crash analysis see [Crash
            Reports](http://www.chromium.org/developers/crash-reports).

## ==Use minidump_stackwalk to show a stack trace==

TODO(mkrebs): ...

## ==Use gdb to show a backtrace==

### Generate core file

Convert a minidump to a core file.

```none
sh -c '~/chromiumos/chroot/usr/bin/minidump-2-core -v upload_file_minidump-7adc2ee0079cb374.dmp > minidump.core 2>minidump.core.out'
```

For a minidump from a 32-bit executable, use `minidump-2-core.32` instead.

For an ARM minidump, you have to work a little bit harder to get a core file.
The easiest way is probably to do the conversion using qemu within your chroot.

```none
# Convert minidump to a core file in /tmp/
SYSROOT=/build/daisy
qemu-arm \
  ${SYSROOT}/lib/ld-linux-armhf.so.3 \
  --library-path ${SYSROOT}/lib:${SYSROOT}/usr/lib \
  ${SYSROOT}/usr/bin/minidump-2-core \
    -v ~/test/upload_file_minidump-de1f11232d825812.dmp >/tmp/minidump.core 2>/tmp/minidump.core.out
```

Reference: <https://crbug.com/217064>

### Calculate address for symbols

First, setup the necessary files for the debugger. You need both the original
executables and/or libraries whose symbols you wish to calculate, and you will
need their corresponding debug information. Googlers: See [Setup files for
debugger](https://sites.google.com/a/google.com/mkrebs/references/chrome-os#TOC-Setup-files-for-debugger)
for how to get these for official images.

Calculate the base address of the crashing executable's .text section.

```none
# Determine base address where the crashing executable was mapped
set t1=`grep -w GUID minidump.core.out | grep '"/opt/google/chrome/chrome"' | sed -e 's/-.*//'`
# Determine offset of .text within the executable (run objdump with sudo if setuid program like Xorg)
set t2=`objdump -h /usr/local/google/home/mkrebs/tmp/test/sigabrt/2913.45.0/image/rootfs/opt/google/chrome/chrome | \grep '\.text'|awk '{print $4}'`
# Print out address of .text in memory
perl -e 'die unless $ARGV[0] && $ARGV[1]; printf("%#x\n", hex($ARGV[0]) + hex($ARGV[1]))' $t1 $t2
```

You may also want to do this for shared libraries that the executable loaded.

```none
# Determine base address where the library was mapped
set t1=`grep -w GUID minidump.core.out | grep '"/lib/libc-2.15.so"' | sed -e 's/-.*//'`
# Determine offset of .text within the executable (run objdump with sudo if setuid program like Xorg)
set t2=`objdump -h /usr/local/google/home/mkrebs/tmp/test/issue35349/2913.84.5/image/rootfs/lib/libc-2.15.so | \grep '\.text'|awk '{print $4}'`
# Print out address of .text in memory
perl -e 'die unless $ARGV[0] && $ARGV[1]; printf("%#x\n", hex($ARGV[0]) + hex($ARGV[1]))' $t1 $t2
```

Googlers: Alternatively, you can try my generate_gdb_command_file script to
automatically generate a gdb command file for adding all the symbol files. For
the "--top" command-line option, specify the path to where your image is
mounted. Then, pass the path to the file to which you redirected
minidump-2-core's stderr.

```none
/home/mkrebs/bin/scripts/generate_gdb_command_file --top 2913.84.5/image/rootfs/ ~/checkouts/cros/cros6/chroot/tmp/minidump.core.out > minidump.gdb
```

### Start debugger

Run gdb on the core file.

```none
gdb --core minidump.core
armv7a-cros-linux-gnueabi-gdb --core minidump.core
```

Map executable's symbol file to base address of .text section.

```none
(gdb) add-symbol-file <path to top-level "debug" directory>/<path to .debug file> <address from calculation>
```

Googlers: Alternatively, if you tried my generate_gdb_command_file script, use
the generated gdb command file to add all the symbol files.

```none
gdb --core minidump.core --command minidump.gdb
```

### Example

```none
grep -w GUID minidump.core.txt | grep '"/opt/google/chrome/chrome"' | sed -e 's/-.*//'
0x72CC9000
objdump -h /usr/local/google/home/mkrebs/tmp/test/sigabrt/2913.45.0/image/rootfs/opt/google/chrome/chrome | \grep '\.text'|awk '{print $4}'
002a73e0
perl -e 'printf("%#x\n", hex($ARGV[0]) + hex($ARGV[1]))' 0x72CC9000 002a73e0
0x72f703e0
```

```none
gdb --core=minidump.core
(gdb) add-symbol-file /usr/local/google/home/mkrebs/tmp/test/sigabrt/2913.45.0/debug/opt/google/chrome/chrome.debug 0x72f703e0
```

## ==If backtrace in gdb did not help==

Sometimes, the gdb backtrace command ([Use gdb to show a
backtrace](http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/crash-reporting/debugging-a-minidump#TOC-Use-gdb-to-show-a-backtrace))
doesn't show a stack trace any better than that of minidump_stackwalk ([Use
minidump_stackwalk to show a stack
trace](http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/crash-reporting/debugging-a-minidump#TOC-Use-minidump_stackwalk-to-show-a-stack-trace)).
If you think there's more to it than what those two are showing you, try this
method to naively dump all the known symbol addresses seen on the stack. You'll
see some false positives, but you may just find the name of a function that
seems like a plausible place to look.

First, start from the above step of [using gdb to show a
backtrace](http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/crash-reporting/debugging-a-minidump#TOC-Use-gdb-to-show-a-backtrace).
We can reuse the gdb command file that was generated for it. Googlers: If you
didn't use my generate_gdb_command_file script, you can manually create the gdb
command file containing any "add-symbol-file" commands you ran.

Second, determine the base address of the stack. The easiest way is to just look
at the stack address in the minidump. This command will set the $sp_addr
variable to the stack address of the first thread in the minidump:

```none
set sp_addr=`~/checkouts/cros/cros6/chroot/usr/bin/minidump_dump upload_file_minidump-62893577d8a73070.dmp | perl -ne 's/^  stack.start_of_memory_range = // && print && exit'`
```

If you want to be smarter, you can run minidump_stackwalk on the minidump (using
the appropriate Breakpad symbols) and skip past any stack frames you can assume
are accurate. You would do this by looking for the stack pointer of the last
consecutive call frame found by "call frame info". For example, for issue
<https://crbug.com/217636> the stack pointer value would be **0x7e8b3440** given
the following top-most (i.e. higher address) frames:

```none
  7  Xorg!OsSigHandler [osinit.c : 146 + 0x11]
      r4 = 0x76fc9f7c    r5 = 0x7e8b3440    r6 = 0x76bfb094    r7 = 0x00000000
      r8 = 0x00000000    r9 = 0x00000020   r10 = 0x00000004    fp = 0x00000008
      sp = 0x7e8b3430    pc = 0x76f90949
     Found by: call frame info
  8  libc-2.15.so + 0x25e6e
      r4 = 0x774ec898    r5 = 0x76e9aeb8    r6 = 0x76bfb094    r7 = 0x00000000
      r8 = 0x00000000    r9 = 0x00000020   r10 = 0x00000004    fp = 0x00000008
      sp = 0x7e8b3440    pc = 0x76b40e70
     Found by: call frame info
  9  libc-2.15.so!new_do_write [fileops.c : 537 + 0x11]
      sp = 0x7e8b345c    pc = 0x76b6abe5
     Found by: stack scanning
```

Finally, run the following gdb command to dump 1024 words of the stack (i.e.
potential addresses). In the output, for any value that corresponds to a known
symbol, gdb will annotate it with the symbol's name in angle brackets. Knowing
that, we can simply pipe the output to something (in this case, perl) to pull
out any annotation it finds in the stack dump.

```none
gdb --core minidump.core --batch --command minidump.gdb --eval-command "x/1024aw $sp_addr" |& perl -ne '$re=qr{(<((?:[^<>]++|(?-2))*+)>)}; print "$2\n" while (s/0x[0-9a-f]+ $re//)'
```

Reference: <https://crbug.com/217640#c2>
