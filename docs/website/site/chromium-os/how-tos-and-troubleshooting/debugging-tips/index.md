---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: debugging-tips
title: Chromium on Chromium OS Debugging Tips
---

[TOC]

These instructions will help you debug programs, including the chromium browser
on your chromium os netbook.

### Compiling the browser

See the "[simple
chrome](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/simple_chrome_workflow.md)"
workflow.

## Setting up the device

We recommend that developers use gdb to debug their Chromium-based OS. The gdb
is already included in the chromium os image. You can chroot into your mounted
image or open a new terminal in an existing system with an installed copy of
Chromium OS.
You can talk to your unit over a serial connection, and you can use this to get
network access / ssh working.

To get this code on the netbook you can put it someplace accessible via http and
use wget on the netbook.

# Remount the root drive read / write
sudo mount -o remount,rw /

Each time you reboot you'll need to get access to your built program. You could
copy it from your build directory, but it's easier to use sshfs to mount that
directory directly on the netbook. Also, chrome will be too large to debug
natively on the netbook. Instead you'll need to use gdbserver. So do this:
[Set up sshfs
here](/chromium-os/how-tos-and-troubleshooting/debugging-tips/host-file-access).

# Open port so that gdbserver can be reached

sudo /sbin/iptables -A INPUT -p tcp --dport 1234 -j ACCEPT

The built chrome will be at /tmp/chrome/src/out/Debug/chrome. You can run it
from there to test, or within gdbserver. Make sure you run chrome/gdbserver at
the netbook's command prompt, not from your remote ssh session.

# Run gdb server, listening on port 1234 (opened in iptables command above)

sudo gdbserver :1234 /tmp/chrome/src/*BOARD-NAME*_out/Debug/chrome

## Talking to the device from your development machine

**For more up-to-date instructions, see [the Remote GDB section of the Simple
Chrome
workflow](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/simple_chrome_workflow.md#Remote-GDB).**

First build a cross-gdb on your host machine (inside the chroot).
`sudo USE=expat emerge `cross-i686-pc-linux-gnu/gdb (x86)
`sudo USE=expat emerge `cross-armv7a-cros-linux-gnueabi/gdb (arm)
On your host machine run gdb and make sure you set the sysroot to point to the
top of your chroot so that gdb can find the proper shared objects.

i686-pc-linux-gnu-gdb` *BOARD-NAME*_out/Debug/chrome`

`(gdb) set sysroot /build/*BOARD-NAME*/`

On your host machine you can connect to this gdb server:

(gdb) target remote *IP.ADDR.OF.NETBOOK*:1234

The program is now paused and you have control over it from your host machine.
To start execution:

(gdb) continue

If you want to pass command-line arguments, do so on the target machine at the
end of the gdbserver command line.

You should follow the [Linux debugging
tips](http://code.google.com/p/chromium/wiki/LinuxDebugging) for help debugging
Chromium.

You can debug smaller programs directly. By default, we strip out many of the
debugging symbols when creating our debian packages. To ensure that you have the
debugging symbols for a particular package, check the rules file under
package_source_dir/debian/rules ... and ensure dh_strip is not located anywhere
in this file (generally in binary-arch or build-arch rules).

To get started, run `gdb` from a terminal. Type `help` to get an idea of the
commands.

To debug an already running program use:

`(gdb) attach *pid*`

To debug a new program use:
(gdb) file *file-path*

*(gdb) start *filename**

For commands while you have a running program:

(gdb) help running

If you step through a program and start getting jibberish (read: addresses as
opposed to source code and line numbers) then you need to include the symbols
when building the package. Go back to the debian rules file and make sure -g is
in the compiler flags and dh_strip is not anywhere in there.

## **Getting the Chrome logs**

On test images, Chrome always logs to `/var/log/chrome` (see [this
thread](https://groups.google.com/a/chromium.org/d/topic/chromium-os-dev/ms0xBblGP1c/discussion)).
On other images, only logs from the login screen can be found there; after
login, they are written to `/home/chronos/user/log` (this location is on the
cryptohome, so it's only available while the user is logged in). `stderr` from
Chrome, and messages that it logs before it initializes its log file, appear in
`/var/log/ui`.

## **Enabling core dumps**

See the [Crash Reporting
FAQ](../../packages/crash-reporting/faq/#how-can-i-get-the-core-file-for-a-crash)
for how to get core files.

Once you have a core file, you can load it to gdb.

gdb /opt/google/chrome/chrome core.chrome.1234

Note: Core files can easily fill up your stateful partition, which can cause
various issues. Please clean them up regularly.

## **Getting stack trace with symbols**

emerge/cros deploy strips debug symbols by default, and loading a core file into
gdb may not give you much info without these symbols.

To preserve debug symbols, compile chrome for chromeos with the following
environment variables (in chroot).

export KEEP_CHROME_DEBUG_SYMBOLS=1

export REMOVE_WEBCORE_DEBUG_SYMBOLS=1

The 2nd one will exclude webkit debug symbols which is quite large and probably
not

useful for debugging chrome on chromeos. If you want to have webkit debug
symbols, you may skip it.

If you are planning to use gmerge to build and copy a debug chrome image with
symbols, you will need to create an image with a larger rootfs size, e.g.:

``` bash
cros build-image --adjust-part='ROOT-A:+1G' --no-enable-rootfs-verification test
```

(Run `cros build-image --help` for default values)

If you are using a pre-built test image (ex, from builder artifacts), download
the "debug" archive file (which should contain files like debug/\*.debug),

extract to /usr/local/lib (ex, Chrome's debug file should be
/usr/local/lib/opt/google/chrome/chrome.debug), and start GDB.

## **Set session manager not to restart chrome when chrome is killed (or crashed)**

You may want to kill currently running chrome and run your own copy (via sshfs
for example), or want to leave

the state when chrome crashes. To do that, you need to tell session manager not
to restart chrome by creating a file:

touch /var/run/disable_chrome_restart

Note that this is temp file, so you need to re-create the file when you restart
chromium os.
