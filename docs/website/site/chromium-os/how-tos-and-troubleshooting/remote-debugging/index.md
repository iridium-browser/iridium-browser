---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: remote-debugging
title: Remote Debugging in ChromiumOS
---

### See the [CrOS developer guide](http://www.chromium.org/chromium-os/developer-guide#TOC-Remote-Debugging) for instructions on using a script that automates most of the below steps.

### Device Setup

For details on copying a binary over to your developer device in order to debug
it, see [Host File
Access](/chromium-os/how-tos-and-troubleshooting/debugging-tips/host-file-access).

You will need to open a port on your device so that gdbserver can be reached by
gdb:

```none
sudo /sbin/iptables -A INPUT -p tcp --dport 1234 -j ACCEPT
```

On the target device, run gdbserver. You can run it with a new process or attach
to an existing process:

```none
gdbserver :1234 /bin/ls        # Debug a new instance of /bin/ls
gdbserver --attach :1234 6301  # Attach to existing process with pid: 6301
gdbserver --attach :1234 $(pgrep chrome -P $(pgrep session_manager))  # Attach to primary chrome browser process
```

### Host Setup

Build a cross-gdb on your host machine (inside the chroot).

```none
sudo USE=expat emerge cross-i686-pc-linux-gnu/gdb         # (x86)
sudo USE=expat emerge cross-x86_64-cros-linux-gnu/gdb     # (amd64)
sudo USE=expat emerge cross-armv7a-cros-linux-gnueabi/gdb # (arm)
```

Run gdb and set the appropriate paths to sysroot and debug symbols:

```none
i686-pc-linux-gnu-gdb
(gdb) set sysroot /build/x86-alex
(gdb) set solib-absolute-prefix /build/x86-alex
```

```none
(gdb) set solib-search-path /build/x86-alex
(gdb) set debug-file-directory /build/x86-alex/usr/lib/debug
(gdb) file /build/x86-alex/bin/ls
(gdb) target remote 12.34.56.78:1234
```

If you want to debug a chrome build with cros_chrome_make, first install to a
temporary directory:

```none
cros_chrome_make --install
```

`And use the following instead (the binary specified in the file command is the
one you want to copy over):`

`` ```none
(gdb) set debug-file-directory /build/x86-alex/tmp/portage/chromeos-base/chromeos-chrome-9999/image/usr/lib/debug
(gdb) file /build/x86-alex/tmp/portage/chromeos-base/chromeos-chrome-9999/image/opt/google/chrome/chrome
(gdb) target remote 12.34.56.78:1234
``` ``

The program is now paused and you have control over it from your host machine.
To start execution:

```none
(gdb) continue
```

Helpful hint: you can place the above commands into a file and use the GDB
"source" command to read it:

```none
set sysroot /build/x86-alex
set solib-absolute-prefix /build/x86-alex
```

```none
set solib-search-path /build/x86-alex
```

```none
set debug-file-directory /build/x86-alex/usr/lib/debug
file /build/x86-alex/opt/google/chrome/chrome
# You can include setting breakpoints you know you're going to need.
b ChromeMain
target remote 12.34.56.78:1234
```

Then save that as a file (e.g. "remote.gdb") and do this:

```none
i686-pc-linux-gnu-gdb
(gdb) source remote.gdb
```

### Additional notes for remote debugging Chrome on ChromeOS

To get line number info in chrome builds (**Note**: don't use FEATURES="nostrip"
- the resulting binary will not fit on the device!)

```none
export KEEP_CHROME_DEBUG_SYMBOLS=1 REMOVE_WEBCORE_DEBUG_SYMBOLS=1
```

To debug the installed chrome from outside the chroot (e.g. from emacs) using
/usr/bin/gdb + gdbserver on the device as described above, in .gdbinit (or
sourced file):

```none
set sysroot ~/chromeos/chroot/build/x86-alex
set solib-absolute-prefix ~/chromeos/chroot/build/x86-alex
set debug-file-directory ~/chromeos/chroot/build/x86-alex/usr/lib/debug
file ~/chromeos/chroot/build/x86-alex/opt/google/chrome/chrome
target remote 172.31.55.210:1234
```

To debug chrome built with emerge or cros_chrome_make without installing, as
above except:

```none
file ~/chromeos/chroot/var/cache/chromeos-chrome/chrome-src/src/out_x86-alex/Release/chrome
```
