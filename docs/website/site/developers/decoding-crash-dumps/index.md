---
breadcrumbs:
- - /developers
  - For Developers
page_name: decoding-crash-dumps
title: Decoding Crash Dumps
---

This document describes how to process Breakpad minidumps on Linux.

### Breakpad tools needed

The tools from Breakpad needed to process crash dumps manually are
minidump_stackwalk and dump_syms. It is possible to build these tools from
source from within a Chromium checkout on Mac and Linux by running, for example,
ninja -C out/Release minidump_stackwalk dump_syms. To build these tools from
source in a Breakpad checkout, check out the source from
<http://code.google.com/p/google-breakpad/> and follow the included
instructions.

### Get the crash dump

Crash dumps (.dmp files) usually come from a crash server i.e. <http://crash/>
or from the crash reports directory: /path/to/profile/Crash Reports. I.e.
~/.config/google-chrome/Crash Reports/ on Linux.

For Linux crash dumps that are in the crash reports directory, one must strip
off the headers before processing it with minidump_stackwalk. Just open the file
in a text editor and delete all the lines up until the line that starts with
MDMP followed by binary data.

### Getting the stacktrace (without symbols)

Run minidump_stackwalk foo.dmp. For 32-bit, minidump_stackwalk will display the
stacktrace without symbols. For 64-bit, it will only display the top frame.

### Get the debugging symbols

To get symbols or more frames, one needs to have the symbols for the libraries
and executables that are part of the stacktrace.

The easiest way is to run a tool that will generate the right directory
structure:

components/crash/content/tools/generate_breakpad_symbols.py
--build-dir=out/gnand --symbols-dir=/tmp/my_symbols/
--binary=out/gnand/lib.unstripped/libchrome.so --clear --verbose

To do the same thing manually, start by running:

minidump_stackwalk foo.dmp /tmp/my_symbols 2&gt;&1 | grep my_symbols

This will print out lines like:

\[time stamp\] simple_symbol_supplier.cc:150: INFO: No symbol file at
/tmp/my_symbols/libfoo/hash/libfoo.sym.

In order to get the symbol file for libfoo, one needs to have a copy of the
exact libfoo binary from the system that generated the crash and its
corresponding debugging symbols. Oftentimes, Linux distros provide libfoo and
its debugging symbols as two separate packages. In the chrome build, you'll need
an unstripped binary -- official builds generate these by default somewhere.
After obtaining and extracting the packages, use dump_syms to extract the
symbols. Assuming the library in question is /lib/libfoo.so and its debugging
symbol is /usr/debug/lib/libfoo.so, run:

dump_syms /lib/libfoo.so /usr/debug/lib &gt; /tmp/libfoo.so.sym

To verify it's the correct version of libfoo, look at the hash from the
minidump_stackwalk output and compare it to the hash on the first line. If they
match, move /tmp/libfoo.sym to /tmp/my_symbols/libfoo.so/hash/libfoo.so.sym and
minidump_stackwalk will load it on future runs to give better stacktraces.

Repeat this process for other libraries until minidump_stackwalk outputs the
required information.

### Decoding Windows crash dumps on Linux

Windows crash dumps can be decoded the same way as Linux crash dumps. The issue
is mainly getting the debugging symbols as a .sym file instead of a .pdb file.

To convert a .pdb file to a .sym file:

1.  Obtain the .pdb file and put it on a Windows machine. (It may be
            possible to do this with Wine, YMMV.)
2.  Download
            [dump_syms.exe](http://google-breakpad.googlecode.com/svn/trunk/src/tools/windows/binaries/dump_syms.exe).
3.  Run: dump_syms foo.pdb &gt; foo.sym
    *   If no error messages, then go to the last step
    *   If you get: CoCreateInstance CLSID_DiaSource failed (msdia80.dll
                unregistered?), go to step 4.
4.  Get a copy of msdia80.dll and put it in c:\\Program Files\\Common
            Files\\Microsoft Shared\\VC\\.
5.  As Administrator, run: regsvr32 c:\\Program Files\\Common
            Files\\Microsoft Shared\\VC\\msdia80.dll.
    *   On success, retry step 3.
    *   If you get error 0x80004005, you did not run as Administrator.
6.  Create a symbol-server directory layout
    1.  Find the GUID/age descriptor (printed on the "No symbols" line
                if symbols can't be found)
    2.  Create a directory of the form symbols/foo.pdb/GUIDandAge,
                similar to symbol-server layout, and put foo.sym in that
                directory. Note that the directory name is foo.pdb but the file
                name is foo.sym. For instance:
        *   0x7ff77ead0000 - 0x7ff77eb6efff main.exe ??? (main)
                    (WARNING: No symbols, main.pdb,
                    B7B61AB08C8345248B45D99552B0100C1)
        *   mkdir -p symbols/main.pdb/B7B61AB08C8345248B45D99552B0100C1
        *   cp main.sym
                    symbols/main.pdb/B7B61AB08C8345248B45D99552B0100C1
    3.  Run minidump_stackwalk on Linux specifying the crash dump and
                the symbols directory:
        *   minidump_stackwalk foo.dmp symbols

### Decoding Mac crash dumps

See [Working with
Minidumps](/developers/crash-reports#TOC-Working-with-Minidumps).

If you've built Chromium.app with symbols, the easiest way to symbolize a crash
is to let Crashpad forward the crash to the system crash reporter. See
[set_system_crash_reporter_forwarding](https://cs.chromium.org/chromium/src/third_party/crashpad/crashpad/client/crashpad_info.h?l=135).
