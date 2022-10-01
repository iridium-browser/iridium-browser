---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: hardening-against-malicious-stateful-data
title: Hardening against malicious stateful data
---

## Motivation

Chrome OS has Verified Boot, which is designed to make sure that the system will
only run binaries that are trusted, starting at firmware and continuing all the
way through the boot process to Chrome. It turns out this is not sufficient, as
the data dependencies of code running during boot pull unverified data from the
stateful partition. This may be configuration data, device state indicators,
data caches etc. It turns out that data dependencies in the boot process pose a
security risk: Attackers that have a root exploit control the stateful
partition, allowing them to stage malicious file system state (file contents,
symlinks, directory layout, etc.) that will affect a subsequent boot in a way
that re-exploits the device, thus providing a vehicle for the attacker to get a
persistent exploit.

[Both](https://bugs.chromium.org/p/chromium/issues/detail?id=351788)
[examples](https://bugs.chromium.org/p/chromium/issues/detail?id=648971) of
persistent Chrome OS exploits that have been reported to us up to now exploit
this (symlink attacks) as their persistence mechanism. Thus, we need to:

    Eliminate data dependencies on stateful data as much as possible from the
    boot process.

    Mitigate any legit and required dependencies to move the bar to a point
    where exploiting them becomes really hard, if not impossible.

This document describes approaches to achieving these goals..

## Anatomy of a persistent attack

Here's a quick summary of the usual steps involving a persistent exploit. This
is useful for reference when assessing mitigations:

    Attacker exploits the running system. This might be in the form of Chrome
    exploit, a system daemon exploit, a full root exploit, or even a kernel
    exploit.

    Attacker manipulates state carried over across reboots. Depending on what
    privileges the attacker managed to acquire, they have different options
    here. A full root or kernel exploit will allow to make arbitrary changes to
    the stateful file system for example, as well as other stateful storage such
    as TPM and VPD. If the attacker has "only" a Chrome exploit, they may still
    manipulate the stateful file system subtrees that are writeable by user
    chronos. Similarly, if the attacker controls a system daemon, they can
    manipulate that process' state (assuming the system daemon is running within
    a sandbox). Some approaches to manipulation:

        Store malicious data in a file that is read and parsed, interpreted etc.
        after reboot.

        Manipulate the file system layout: Insert symlinks or hard links that
        will redirect write actions during boot (such as mkdir, writing cache
        files etc.).

        Manipulate file system state to trigger unintended execution paths
        (example: make files unreadable / wrong type to trigger
        untested/inappropriate error handling or fallback behavior).

    System reboots.

    Re-gaining code execution: Verified boot ensures that kernel and root file
    system remain intact, i.e. the attacker can't change code there to
    re-acquire control of the system. Instead, the attacker will have to arrange
    for the regular boot flow to "take a wrong turn", i.e. trick legit code
    running during boot to perform inadvertent actions that will give the
    attacker code execution again. This is where the manipulated stateful data
    comes in: It will be consumed by init scripts and system services and may
    affect their behavior in a variety of ways. Some of the more obvious are:

        Trigger execution of shell scripts stored on stateful.

        Exploiting weaknesses in config parsing to gain code execution within a
        system service.

        Manipulating user-installed code that the system will run automatically,
        such as extensions installed in a user profile.

## Mitigations

The remainder of the document discusses hardening measures that will prevent
certain classes of malicious stateful data to have an adverse effect. Note that
there is no comprehensive solution - any useful device will have to store some
data somewhere (in the cloud if not locally) and users expect to be able to see
their previous state after a reboot. This inherently implies that we need to
carry over state, so some risk will always remain. It makes sense to prioritize
mitigation work to reduce impact of successful exploits. To that effect, we
should start with addressing stateful data dependencies within highly-privileged
code (such as init scripts, system daemons), then work our way towards less
privileged code.

### Restricting symlink traversal

[Both](https://bugs.chromium.org/p/chromium/issues/detail?id=351788)
[cases](https://bugs.chromium.org/p/chromium/issues/detail?id=648971) of
persistent Chrome OS exploits that have been reported via the Chrome
vulnerability reward program relied heavily on placing symlinks on the stateful
file system to re-exploit the system after a reboot. The idea is to place a
symlink somewhere on a path written to by a privileged process during boot to
redirect the write to a different location. In some cases, the data that gets
written can also be controlled by the attacker, e.g. when the code in question
is copying files from one location in the stateful file system to another. It
turns out that intentional usage of symlinks is actually very rare on the
stateful file system. Given that and the relative success of symlink attacks, it
makes sense to generally disallow symlink traversal.

To prevent symlinks to be traversed on the stateful file system, there are a
variety of possible approaches:

1.  Change all code that accesses files to double-check it didn't follow
            a symlink inadvertently. Note that the O_NOFOLLOW open flag and
            symlink-aware version of system calls (such as lstat() in favor of
            stat()) are not sufficient, since they only cover the last path
            component. Symlinks in parent components are still followed. A
            working way of implementing a symlink traversal check is via
            readlink() on /proc/self/fd/N. Unfortunately, it's just not
            practical to do this for all file access in shell scripts, 3rd-party
            software etc., so while this approach is technically feasible, it
            doesn't fly in practice.
2.  Add a mount option in a kernel patch, so symlink traversal can be
            disabled on a per-mount basis. The BSDs have nosymfollow which
            implements this. We have [proposed the same solution for
            Linux](https://patchwork.kernel.org/patch/9434587/) upstream, but it
            was met with skepticism. Reality is that few Linux distributions be
            able to use this meaningfully (it only makes sense when you have a
            verified or at least read-only rootfs anyways). We could carry the
            patch in the Chrome OS kernel tree indefinitely, but that'll cause
            maintenance overhead as file system internals change over time.
3.  Scrub symlinks after mount. This would require making a full pass
            over the mounted file system and remove any unintended symlinks.
            This will adversely affect boot times since we'd have to do this
            before starting any init jobs that read stateful data.
4.  Use SELinux to apply policy to prevent symlink traversal. This would
            require the entire stateful file system to be re-labelled after
            mount to make sure there aren't any labels present that would allow
            symlink traversal in locations that shouldn't do so. This approach
            suffers from the same boot time issue as the previous one.
5.  Reject symlinks via the LSM inode_follow_link hook in the Chromium
            OS LSM. Implement the logic in a non-invasive way in the kernel to
            keep maintenance overhead low.

After considering the pros and cons of the approaches listed above, we've chosen
to go with the LSM inode_follow_link approach. Design highlights:

*   The inode_follow_link hook can look at the dentry and inode of the
            traversed symlink location. We thus need to hook up any information
            require to make a decision in such a way that it is accessible from
            the inode/dentry. Note that we can't simply fail all symlink
            traversal, since there are important cases of legit symlink usage on
            the root file system, e.g. the shared library symlinks in /usr/lib.
            We also want to be able to selectively allow symlinks for specific
            locations even though symlink traversal should generally be blocked
            on the stateful file system.
*   To achieve per-inode decisions, the code uses fsnotify
            infrastructure, which allows to attach "inode marks" to an inode.
            This is also used by other kernel subsystems (e.g. the audit code)
            and allows callbacks to be invoked when something happens. We only
            rely on the notification send on inode destruction so we can clean
            up tracked state, but we can use the inode mark to hold symlink
            traversal policy for the inode.
*   Symlink traversal policy is evaluated for a path in question by
            searching up the parent directory chain until an inode is found that
            carries a specific traversal policy. This allows setting the policy
            to block symlinks on the root inode, which will thus be inherited by
            the entire file system. This also allows exceptions to be granted by
            setting the policy to allow traversal for a specific directory,
            which will grant an exception for the subtree rooted at the
            directory.
*   Symlink traversal policy can be configured via userspace via
            securityfs.
*   Whenever an attempt at traversing a symlink runs into a policy that
            blocks the access, the system call wiil fail with EPERM and a kernel
            warning will get logged.
*   The kernel warning collector uploads symlink traversal blocking
            warnings via the crash reporter, so we have some insight whether
            devices hit restrictions inadvertently.
*   The code uses only "public" kernel APIs, i.e. it is entirely
            contained within the LSM module. No kernel changes elsewhere are
            needed. This keeps maintenance overhead at the absolute minimum.

To make use of the symlink traversal policy mechanism provided by the LSM, we'll
require a few userspace changes. chromeos_startup will be responsible to
configure symlink traversal to be blocked on the stateful and encrypted stateful
file systems. We'll need to allow a few exceptions:

*   /var/cache/vpd, /var/cache/echo: These directories contain backwards
            compatibility symlinks set up by dump_vpd_log. We should ideally
            clean up consumers of these symlinks and remove the exception.
*   /var/lib/timezone: This directory contains a symlink to the actual
            time zone file and is maintained by Chrome. This is a low-risk
            exception as this path is generally only sees read-only access by
            privileged code.
*   ~~/var/log: The log directory contains a number of convenience
            symlinks that point at the "latest" log file for ui, chrome,
            power_manager etc. logs. This is somewhat higher risk since
            privileged system daemons write to files in /var/log. We should
            consider restricting this further, but will grant an exception for
            now.~~ The risk of symlinks in /var/log being abused has been
            [mitigated](https://bugs.chromium.org/p/chromium/issues/detail?id=916152).
*   /home: When using ext4 encryption, encrypted user data actually
            resides within the stateful file system and encryption is handled on
            a per-file basis. To avoid blowing up scope considerably for initial
            roll-out, we'll explicitly allow /home so user data maintained by
            Chrome can still use symlinks. Note that this is low risk for the
            most part since the majority of the access are happening within
            chronos user context. There is some data that is access by
            privileged daemons in the /home/chronos/root/ bind-mounts though. We
            should eventually lock down symlink traversal either for the entire
            user data subtree, or at least for the locations access by system
            daemons.
*   /mnt/stateful_partition/dev_image: This directory contains utilties
            that are convenient for developers running in dev mode. We'll grant
            an exception for this when setting up this location for developer
            mode.
*   /mnt/stateful_partition/unencrypted/art-data: ARC++ uses symlinks in
            /mnt/stateful_partition/unencrypted/art-data to make host compiled
            code available in ARC++ container without copying.

Further exceptions for symlink traversal restriction can be added in justified
cases.

### Enabling generic per-inode access control policies and blocking FIFO access

Building upon the existing framework for adding an "inode mark" to enable
per-inode decisions about symlink traversal, we have extended the code to allow
for generic per-inode access control policies regarding other types of accesses
to the file system. In this way, we can support the use of additional hooks in
the Chromium OS LSM which consult the "inode mark" when making decisions about
other file system security policies. For example, one recent
[exploit](https://bugs.chromium.org/p/chromium/issues/detail?id=766253) modified
a file on the stateful file system to convert it from a normal file into a FIFO
in order to disable the execution progress of a program that opened the file for
reading. In light of this, we have added an additional policy to the "inode
mark" metadata that allows us to deny opening of FIFOs on the stateful file
system in addition to restricting symlink traversal. We use the file_open hook
in the LSM to check the inode metadata when a FIFO is being opened on the
system. All other details are the same as described above for restricting
symlink traversal. As FIFO usage is even more rare than usage of symlinks on the
stateful file system, the only exceptions to this policy are:

*   /mnt/stateful_partition/dev_image: Used for developers running in
            dev mode.
*   /mnt/stateful_partition/home: ARC++ /data directory is mounted under
            this path, and uses FIFOs (e.g. java.io.tmpdir is expected to
            support FIFOs, and is set by the Android framework to use
            /data/user/0/&lt;package_name&gt;/cache/).

As above, further exceptions for FIFO access can be added in justified cases.
