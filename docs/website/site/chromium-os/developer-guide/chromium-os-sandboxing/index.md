---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/developer-guide
  - Chromium OS Developer Guide
page_name: chromium-os-sandboxing
title: Chromium OS Sandboxing
---

**This document can now be found here:**
**<https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md>**

**The content below is a (possibly out-of-date) copy for easier searching.**

# Sandboxing Chrome OS system services

## Contents

*   [Best practices for writing secure system
            services](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Best-practices-for-writing-secure-system-services)
*   [Just tell me what I need to
            do](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Just-tell-me-what-I-need-to-do)
*   [User
            ids](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#User-ids)
*   [Capabilities](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Capabilities)
*   [Namespaces](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Namespaces)
*   [Seccomp
            filters](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Seccomp-filters)
    *   [Detailed instructions for generating a seccomp
                policy](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Detailed-instructions-for-generating-a-seccomp-policy)
*   [Securely mounting cryptohome daemon store
            folders](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Securely-mounting-cryptohome-daemon-store-folders)
*   [Minijail
            wrappers](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Minijail-wrappers)

In Chrome OS, OS-level functionality (such as configuring network interfaces) is
implemented by a collection of system services, and provided to Chrome over
D-Bus. These system services have greater system and hardware access than the
Chrome browser.

Separating functionality like this prevents an attacker exploiting the Chrome
browser through a malicious website to be able to access OS-level functionality
directly. If Chrome were able to directly control network interfaces, a
compromise in Chrome would give the attacker almost full control over the
system. For example, by having a separate network manager, we can reduce the
functionality exposed to an attacker to just querying interfaces and performing
pre-determined actions on them.

Chrome OS uses a few different mechanisms to isolate system services from Chrome
and from each other. We use a helper program called Minijail (executable
`minijail0`). In most cases, Minijail is used in the service's init script. In
other cases, [Minijail
wrappers](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Minijail-wrappers)
are used if a service wants to apply restrictions to the programs that it
launches, or to itself.

## Best practices for writing secure system services

Just remember that code has bugs, and these bugs can be used to take control of
the code. An attacker can then do anything the original code was allowed to do.
Therefore, code should only be given the absolute minimum level of privilege
needed to perform its function.

Aim to keep your code lean, and your privileges low. Don‘t run your service as
`root`. If you need to use third-party code that you didn’t write, you should
definitely not run it as `root`.

Use the libraries provided by the system/SDK. In Chrome OS,
[libchrome](http://www.chromium.org/chromium-os/packages/libchrome) and
[libbrillo](http://www.chromium.org/chromium-os/packages/libchromeos) (née
libchromeos) offer a lot of functionality to avoid reinventing the wheel,
poorly. Don‘t reinvent IPC, use D-Bus or Mojo. Don’t open listening sockets,
connect to the required service.

Don't (ab)use shell scripts, shell script logic is harder to reason about and
[shell command-injection
bugs](http://en.wikipedia.org/wiki/Code_injection#Shell_injection) are easy to
miss. If you need functionality separated from your main service, use normal C++
binaries, not shell scripts. Moreover, when you execute them, consider further
restricting their privileges (see section [Minijail
wrappers](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Minijail-wrappers)).

## Just tell me what I need to do

*   Create a new user for your service: <https://crrev.com/c/225257>
*   Optionally, create a new group to control access to a resource and
            add the new user to that group: <https://crrev.com/c/242551>
*   Use Minijail to run your service as the user (and group) created in
            the previous step. In your init script:
    *   `exec minijail0 -u <user> /full/path/to/binary`
    *   See section [User
                ids](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#User-ids).
*   If your service fails, you might need to grant it capabilities. See
            section
            [Capabilities](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Capabilities).
*   Use as many namespaces as possible. See section
            [Namespaces](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Namespaces).
*   Consider reducing the kernel attack surface exposed to your service
            by using seccomp filters, see section [Seccomp
            filters](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md#Seccomp-filters).

## User ids

The first sandboxing mechanism is user ids. We try to run each service as its
own user id, different from the `root` user, which allows us to restrict what
files and directories the service can access, and also removes a big chunk of
system functionality that‘s only available to the root user. Using the
`permission_broker` service as an example, here’s its Upstart config file (lives
in `/etc/init`):

[`permission_broker.conf`](https://chromium.googlesource.com/chromiumos/platform2/+/HEAD/permission_broker/permission_broker.conf)

```
start on starting system-services
stop on stopping system-services

respawn

env PERMISSION_BROKER_GRANT_GROUP=devbroker-access

# Run as 'devbroker' user.
exec minijail0 -u devbroker -c 'cap_chown,cap_fowner+eip' -- \
  /usr/bin/permission_broker --access_group=${PERMISSION_BROKER_GRANT_GROUP}
```

Minijail's `-u` argument forces the target program (in this case
`permission_broker`) to be executed as the devbroker user, instead of the root
user. This is equivalent of doing `sudo -u devbroker`.

The user (`devbroker` in this example) needs to first be added to the build
system database. An example of this (for a different user) can be found in the
following CL: <https://crrev.com/c/361830>

Next, the user needs to be *installed* on the system. An example of this (again
for a different user) can be found in the following CL:
<https://crrev.com/c/383076>

See the [Chrome OS user accounts
README](https://chromium.googlesource.com/chromiumos/overlays/eclass-overlay/+/HEAD/profiles/base/accounts/README.md)
for more details.

There‘s a test in the CQ that keeps track of the users present on the system
that request additional access (e.g. listing more than one user in a group). If
your user does that, the test baseline has to be updated at the same time the
accounts are added with another CL (e.g. <https://crrev.com/c/894192>). If
you’re unsure whether you need this, the PreCQ/CQ will reject your CL when the
test fails, so if the tests pass, you should be good to go!

You can use CQ-DEPEND to land the CLs together (see [How do I specify the
dependencies of a
change?](http://www.chromium.org/developers/tree-sheriffs/sheriff-details-chromium-os/commit-queue-overview)).

## Capabilities

Some programs, however, require some of the system access usually granted only
to the root user. We use [Linux
capabilities](http://man7.org/linux/man-pages/man7/capabilities.7.html) for
this. Capabilities allow us to grant a specific subset of root's privileges to
an otherwise unprivileged process. The link above has the full list of
capabilities that can be granted to a process. Some of them are equivalent to
root, so we avoid granting those. In general, most processes need capabilities
to configure network interfaces, access raw sockets, or performing specific file
operations. Capabilities are passed to Minijail using the `-c` switch.
`permission_broker`, for example, needs capabilities to be able to `chown(2)`
device nodes.

[`permission_broker.conf`](https://chromium.googlesource.com/chromiumos/platform2/+/HEAD/permission_broker/permission_broker.conf)

```
start on starting system-services
stop on stopping system-services

respawn

env PERMISSION_BROKER_GRANT_GROUP=devbroker-access

# Run as <devbroker> user.
# Grant CAP_CHOWN and CAP_FOWNER.
exec minijail0 -u devbroker -c 'cap_chown,cap_fowner+eip' -- \
  /usr/bin/permission_broker --access_group=${PERMISSION_BROKER_GRANT_GROUP}
```

Capabilities are expressed using the format that
[cap_from_text(3)](http://man7.org/linux/man-pages/man3/cap_from_text.3.html)
accepts.

## Namespaces

Many resources in the Linux world can be isolated now such that a process has
its own view of things. For example, it has its own list of mount points, and
any changes it makes (unmounting, mounting more devices, etc...) are only
visible to it. This helps keep a broken process from messing up the settings of
other processes.

For more in-depth details, see the [namespaces
overview](http://man7.org/linux/man-pages/man7/namespaces.7.html).

In Chromium OS, we like to see every process/daemon run under as many unique
namespaces as possible. Many are easy to enable/rationalize about: if you don't
use a particular resource, then isolating it is straightforward. If you do rely
on it though, it can take more effort.

Here‘s a quick overview. Use the command line option if the description below
matches your service (or if you don’t know what functionality it‘s talking about
-- most likely you aren’t using it!).

*   `--profile=minimalistic-mountns`: This is a good first default that
            enables mount and process namespaces. This only mounts `/proc` and
            creates a few basic device nodes in `/dev`. If you need more things
            mounted, you can use the `-b` (bind-mount) and `-k` (regular mount)
            flags.
*   `--uts`: Just always turn this on. It makes changes to the host /
            domain name not affect the rest of the system.
*   `-e`: If your process doesn't need network access (including UNIX or
            netlink sockets).
*   `-l`: If your process doesn't use SysV shared memory or IPC.

The `-N` option does not work on Linux 3.8 systems. So only enable it if you
know your service will run on a newer kernel version otherwise minijail will
abort for the older kernels ([Chromium bug 729690](https://crbug.com/729690)).

*   `-N`: If your process doesn't need to modify common [control groups
            settings](http://man7.org/linux/man-pages/man7/cgroups.7.html).

## Seccomp filters

Removing access to the filesystem and to root-only functionality is not enough
to completely isolate a system service. A service running as its own user id and
with no capabilities has access to a big chunk of the kernel API. The kernel
therefore exposes a huge attack surface to non-root processes, and we would like
to restrict what kernel functionality is available for sandboxed processes.

The mechanism we use is called
[Seccomp-BPF](https://www.kernel.org/doc/Documentation/prctl/seccomp_filter.txt).
Minijail can take a policy file that describes what syscalls will be allowed,
what syscalls will be denied, and what syscalls will only be allowed with
specific arguments. The full description of the policy file language can be
found in the [`syscall_filter.c`
source](https://chromium.googlesource.com/aosp/platform/external/minijail/+/HEAD/syscall_filter.c#239).

Abridged policy for [`mtpd` on amd64
platforms](https://chromium.googlesource.com/chromiumos/platform2/+/HEAD/mtpd/mtpd-seccomp-amd64.policy):

```
# Copyright 2022 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
read: 1
ioctl: 1
write: 1
timerfd_settime: 1
open: 1
poll: 1
close: 1
mmap: 1
mremap: 1
munmap: 1
mprotect: 1
lseek: 1
# Allow socket(domain==PF_LOCAL) or socket(domain==PF_NETLINK).
socket: arg0 == 0x1 || arg0 == 0x10
# Allow PR_SET_NAME from libchrome's base::PlatformThread::SetName().
prctl: arg0 == 0xf
```

Any syscall not explicitly mentioned, when called, results in the process being
killed. The policy file can also tell the kernel to fail the system call
(returning -1 and setting `errno`) without killing the process:

```
# execve: return EPERM
execve: return 1
```

To write a policy file, run the target program under `strace` and use that to
come up with the list of syscalls that need to be allowed during normal
execution. The [generate_syscall_policy.py
script](https://chromium.googlesource.com/aosp/platform/external/minijail/+/HEAD/tools/generate_seccomp_policy.py)
will take `strace` output files and generate a policy file suitable for use with
Minijail. On top of that, the `-L` option will print the name of the first
syscall to be blocked to syslog. The best way to proceed is to combine both
approaches: use `strace`and the script to generate a rough policy, and then use
`-L` if you notice your program is still crashing. Note that the `-L` option
should NOT be used in production.

The policy file needs to be installed in the system, so we need to add it to the
ebuild file:

[`mtpd-9999.ebuild`](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/chromeos-base/mtpd/mtpd-9999.ebuild)

```
# Install seccomp policy file.
insinto /usr/share/policy
use seccomp && newins "mtpd-seccomp-${ARCH}.policy" mtpd-seccomp.policy
```

And finally, the policy file has to be passed to Minijail, using the `-S`
option:

[`mtpd.conf`](https://chromium.googlesource.com/chromiumos/platform2/+/HEAD/mtpd/mtpd.conf)

```
# use minijail (drop root, set no_new_privs, set seccomp filter).
# Mount /proc, /sys, /dev, /run/udev so that USB devices can be
# discovered. Also mount /run/dbus to communicate with D-Bus.
exec minijail0 -i -I -p -l -r -v -t -u mtp -g mtp -G \
  -P /var/empty -b / -b /proc -b /sys -b /dev \
  -k tmpfs,/run,tmpfs,0xe -b /run/dbus -b /run/udev \
  -n -S /usr/share/policy/mtpd-seccomp.policy -- \
  /usr/sbin/mtpd -minloglevel="${MTPD_MINLOGLEVEL}"
```

### Detailed instructions for generating a seccomp policy

*   Generate the syscall log: `strace -f -o strace.log <cmd>`
*   Cut off everything before the following for a smaller filter that
            can be used with `LD_PRELOAD`:
```
rt_sigaction(SIGRTMIN, {<sa_handler>, [], SA_RESTORER|SA_SIGINFO, <sa_restorer>}, NULL, 8) = 0
rt_sigaction(SIGRT_1, {<sa_handler>, [], SA_RESTORER|SA_RESTART|SA_SIGINFO, <sa_restorer>}, NULL, 8) = 0
rt_sigprocmask(SIG_UNBLOCK, [RTMIN RT_1], NULL, 8) = 0 getrlimit(RLIMIT_STACK, {rlim_cur=8192\*1024, rlim_max=RLIM64_INFINITY}) = 0
brk(NULL) = 0x7f8a0656e000
brk(<addr>) = <addr>
```
*   Run the policy generation script:
    *   `~/chromiumos/src/aosp/external/minijail/tools/generate_seccomp_policy.py
                strace.log > seccomp.policy`
*   Test the policy (look at /var/log/messages for the blocked syscall
            when this crashes):
    *   `minijail0 -S seccomp.policy -L <cmd>`
*   To find a failing syscall without having seccomp logs available
            (i.e., when minijail0 was run without the `-L` option):
    *   `dmesg | grep "syscall="` to find something similar to:
```
NOTICE kernel: [ 586.706239] audit: type=1326 audit(1484586246.124:6): ...
comm="<executable>" exe="/path/to/executable" sig=31 syscall=130 compat=0
ip=0x7f4f214881d6 code=0x0
```
*   Then do:
    *   `minijail0 -H | grep <nr>`, where `<nr>` is the `syscall=`
                number above, to find the name of the failing syscall.

## Securely mounting cryptohome daemon store folders

Some daemons store user data on the user‘s cryptohome under
`/home/.shadow/<user_hash>/mount/root/<daemon_name>` or equivalently
`/home/root/<user_hash>/<daemon_name>`. For instance, Session Manager stores
user policy under `/home/root/<user_hash>/session_manager/policy`. This is
useful if the data should be protected from other users since the user’s
cryptohome is only mounted (and therefore decrypted) when the user logs in. If
the user is not logged in, it is encrypted with the user's password.

However, if a daemon is already running inside a mount namespace (`minijail0 -v
...`) when the user's cryptohome is mounted, it does not see the mount since
mount events do not propagate into mount namespaces by default. This propagation
can be achieved, though, by making the parent mount a shared mount and the
corresponding mount inside the namespace a shared or slave mount, see the
[shared
subtrees](https://www.kernel.org/doc/Documentation/filesystems/sharedsubtree.txt)
doc.

To set up a cryptohome daemon store folder that propagates into your daemon‘s
mount namespace, add this code to the src_install section of your daemon’s
ebuild:

```
local daemon_store="/etc/daemon-store/<daemon_name>"
dodir "${daemon_store}"
fperms 0700 "${daemon_store}"
fowners <daemon_user>:<daemon_group> "${daemon_store}"
```

This directory is never used directly. It merely serves as a secure template for
the chromeos_startup script, which picks it up and creates
`/run/daemon-store/<daemon_name>` as a shared mount.

In your daemon's init script, mount that folder as slave in your mount
namespace. Be sure not to mount all of `/run` if possible.

```
minijail0 -Kslave \
  -k 'tmpfs,/run,tmpfs,MS_NOSUID|MS_NODEV|MS_NOEXEC' \
  -b /run/daemon-store/<daemon_name> \
  ...
```

During sign-in, when the user's cryptohome is mounted, Cryptohome creates
`/home/.shadow/<user_hash>/mount/root/<daemon_name>`, bind-mounts it to
`/run/daemon-store/<daemon_name>/<user_hash>` and copies ownership and mode from
`/etc/daemon-store/<daemon_name>`to the bind target. Since
`/run/daemon-store/<daemon_name>` is a shared mount outside of the mount
namespace and a slave mount inside, the mount event propagates into the daemon.

Your daemon can now use `/run/daemon-store/<daemon_name>/<user_hash>` to store
user data once the user's cryptohome is mounted. Note that even though
`/run/daemon-store` is on a tmpfs, your data is actually stored on disk and not
lost on reboot.

**Be sure not to write to the folder before cryptohome is mounted**. Consider
listening to Session Manager's `SessionStateChanged` signal or similar to detect
mount events. Note that `/run/daemon-store/<daemon_name>/<user_hash>` might
exist even though cryptohome is not mounted, so testing existence is not enough
(it only works the first time).

The `<user_hash>` can be retrieved with Cryptohome's `GetSanitizedUsername`
D-Bus method.

The following diagram illustrates the mount event propagation:

[<img alt="image"
src="/chromium-os/developer-guide/chromium-os-sandboxing/mount_event_propagation.png">](/chromium-os/developer-guide/chromium-os-sandboxing/mount_event_propagation.png)
