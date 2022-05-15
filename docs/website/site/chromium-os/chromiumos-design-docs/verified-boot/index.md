---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: verified-boot
title: Verified Boot
---

[TOC]

## Abstract

*   The Chromium OS team is implementing a verified boot solution that
            strives to ensure that users feel secure when logging into a
            Chromium OS device. Verified boot starts with a read-only portion of
            firmware, which only executes the next chunk of boot code after
            verification.
*   Verified boot strives to ensure that all executed code comes from
            the Chromium OS source tree, rather than from an attacker or
            corruption.
*   Verified boot is focused on stopping the opportunistic attacker.
            While verified boot is not expected to detect every attack, the goal
            is to be a significant deterrent which will be improved upon
            iteratively.
*   Verification during boot is performed on-the-fly to avoid delaying
            system start up. It uses stored cryptographic hashes and may be
            compatible with any trusted kernel.

This document extends and expands on the [Firmware Boot and
Recovery](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery),
[Verified Boot
Crypto](/chromium-os/chromiumos-design-docs/verified-boot-crypto), and [Verified
Boot Data
Structures](/chromium-os/chromiumos-design-docs/verified-boot-data-structures)
documents.

Verified Boot should provide a mechanism that aids the user in detecting when
their system is in need of recovery due to boot path changes. In particular, it
should meet these requirements:

*   Detect non-volatile memory changes from expected state (rw firmware)
*   Detect file system changes relevant to system boot (kernel, init,
            modules, fs metadata, policies)
*   Support functionality upgrades in the field

It is important to note that restraining the boot path to only
Chromium-project-supplied code is not a goal. The focus is to ensure that when
code is run that is not provided for or maintained by upstream, that the user
will have the option to immediately reset the device to a known-good state.
Along these lines, there is no dependence on remote attestation or other
external authorization. Users will always own their computers.

## Goals of verified boot

The primary attacker in this model is an opportunistic attacker. This means that
the attacker has accessed the system using:

*   a remote vector, such as the Chromium-based browser or a browser
            plugin
*   a local vector, such as booting to a USB drive and changing files
            (but **not** by replacing the write-protected firmware)

If we assume attackers access the system via a remote vector and bypass all
run-time defenses, then they will have access to modify the root partition
(kernel, modules, browser, ...), update read-write firmware, inject code into
SMRAM, and so on. In addition, the attacker can now access any data in the
currently-logged-in user's account such as locally stored media and website
cookies. The attacker may collect passwords when typed by the user into the
Chromium-based browser or the screenlocker.

An opportunistic *local* attacker will have a completely different level of
access. Access will be achieved using a USB boot drive, or other out-of-band
bootable material supported by the firmware. Once the system is running the
attacker's operating system, she will be able to modify the root file system and
encrypted user-data blobs. She won't have any visibility into the encrypted
information but may copy it or modify it.

While Chromium OS does as much as possible to guard against such remote and
local breaches, no software system is impervious to successful attacks.
Therefore, it is important that the attacker cannot continue to "own" a machine
through permanent, local changes. To that end, on boot, the firmware and other
accessible regions of the system internals are verified to be in a known good
state. If they aren't, then the firmware recovery process will be initiated (or
the user can request permission to proceed, which would make sense in the case
of a development install, for example).

The important factor to consider with the attackers considered above is that if
an attacker gains access via the Chromium browser, they can presumably modify
the Chromium browser's startup (or bookmarks or server-side settings) to
re-attack the machine at next reboot. This is why it is important to be able to
ensure that a safe recovery/reinstall is possible outside of what can be done by
an attacker on the machine. (Obviously, this is no deterrent for a
**determined** attacker willing to modify the system physically.)

## Getting to the kernel safely

As outlined in the Firmware Boot and Recovery document, verification will occur
in several places. Initially, the small read-only stub code will compute a SHA-2
hash (either with internal code or using a provided SHA-2 accelerator) of the
read-write portion of the firmware. An RSA signature of this hash will then be
verified using a permanently stored public key (of,
[ideally](http://csrc.nist.gov/publications/nistpubs/800-57/sp800-57-Part1-revised2_Mar08-2007.pdf),
2048 bits or more).

The read-write firmware is then responsible for computing hashes of any other
non-volatile memory and the kernel that will be executed. It will contain its
own *subkey* and a list of cryptographic hashes for the data to be verified:
kernel, initrd, master boot record, and so on. These additional hashes will be
signed by the subkey so that they may be updated without requiring the
write-protected key to be used for every update. (Note, the kernel+initrd signed
hashes may be stored with the kernel+initrd on disk to avoid needing a firmware
update when they change.)

Once we're in the kernel, we've successfully performed a verified boot.

## Extending verification from the kernel on upward

In general, once we're running, integrity measurements become less useful. We
can ensure that the Chromium browser that we execute has not been tampered with,
but we can't guarantee that the same attack that compromised it the first time
won't compromise it a second time without updating.

To ensure that an update is possible, the executables, modules, and
configuration files that allow the system to receive updates must be authentic
and untampered with. Getting to that point requires network access and a running
autoupdater. Given that Chromium OS keeps a very minimal root file system, it's
easier to just verify everything on it.

While that sounds great in theory, in practice it is hard to guarantee an intact
file system without paying the cost for upfront checks. If the read-write
firmware were to verify the entire root partition before proceeding to boot, it
would add at least 5 seconds to the boot-time on current netbooks. This delay is
untenable.

Instead of performing full file system verification in advance, it can be done
on demand from a verified kernel. A transparent block device will be layered
between the run-time system and all running processes. It will be configured
during kernel startup using either in-kernel code changes or from a
firmware-verified initial ramdisk image. Each block that is accessed via the
transparent block device layer will be checked against a cryptographic hash
which is stored in a central collection of hashes for the verifiable block
device. This may be in a standalone partition or trailing the filesystem on the
verifiable block device.

Initially, blocks will be 4KB in size. For a root file system of roughly ~75MB,
there will be roughly 19,200 SHA-1 hashes. On current x86 and ARM based systems,
computing the SHA-1 hash of 4KB takes between 0.2ms and 0.5ms. There will be
additional overhead (TBD) incurred by accessing the correct block hash and
comparing the cryptographic digests. Once verified, blocks should live in the
page cache above the block layer. This will mean that verification does not
occur on every read event. To further amortize time-costs, the block hashes will
be segmented into logical bundles of block hashes. Each bundle will be hashed.
The subsequent list of bundle hashes will then be hashed. This layering can be
repeated as needed to build a tree. The final, single hash will be hard coded
into the kernel or supplied through a device interface from a trusted initial
RAM disk.

![](/chromium-os/chromiumos-design-docs/verified-boot/diag2.png)

Note that SHA-1 is considered to be [unsafe after 2010 by
NIST](http://csrc.nist.gov/groups/ST/toolkit/secure_hashing.html) for general
use. The biggest risk here is that specific block collisions can be found and
made such that they provide an alternate execution path. We could use any
hashing algorithm supported by the Linux kernel in our implementation. SHA-1 is
just a specific example.

## Known weaknesses of verified boot

While verified boot can ensure that the system image (i.e. firmware, kernel,
root file system) are protected against tampering by attackers, it can't protect
data that must inherently be modifiable by a running system. This includes user
data, but also system-wide state such as system configuration (network, time
zone, keyboard layout, etc.), cached data maintained by the system (VPD
contents, metrics and crash reporting data, etc.). This state is generally kept
on the writable stateful file system. In some cases, it is consumed by the boot
process and may affect the behavior of the (verified) software. If an attacker
manages to place malicious data on the stateful file system that will cause the
verified code to "take a wrong turn", they may cause inadvertent side effects
that may ultimately lead to the system getting exploited and thus defeating
verified boot. The [Hardening against malicious stateful
data](/chromium-os/chromiumos-design-docs/hardening-against-malicious-stateful-data)
document discusses details and mitigations.

## Mitigating potential bottlenecks

Loading hashes off the same disk as the data for each block would affect
performance during a read. Right now, the plan is to read in signed bundles of
hashes as blocks in that bundle range are accessed. Once a bundle is loaded into
memory, it is kept there. If we assume that we're looking at something like
20k-hashes, then that will require around 400KB of memory. Even if the needed
hashes grow to twice that, allocating 1MB of kernel memory doesn't seem too
onerous. In addition, keeping the block hashes in memory will provide for easy
linear addressing of the hashes since they will be in block-order.

## Handling updates

For Chromium OS, the autoupdater will update the collection of hashes specific
to the partition it is updating. In general, the complete collection of hashes
for a specific partition will be generated by running a lightweight utility
directly on the filesystem image. It will walk the origin block device and emit
an image file that contains the properly formatted hash collection. In addition,
it will emit the SHA-1 hash of the bundle hashes. This will be the authoritative
hash that will need to be either signed or stored in a signed/verified location,
such as in the kernel. The resulting file can either be appended to the
filesystem image or stored in a standalone partition (hash partition). On
update, a direct difference of the new hash collection can be taken using bsdiff
against the last versions. However, it may be that more efficient difference
generation approaches may be used as long as the end result is the same.

## The implementation

Post-firmware verified boot will most likely be implemented as a device mapper
target. It will provide the transparent, verifying block layer. Initially, it
will be assumed that there will be a verified initrd that can be used to set up
the root partition using the dm device. A simple utility tool will be written
that will directly hash a given block device and emit a compatible binary blob
that contains the collection of hashes. It will take the format:

```
hashblock_1 . . . hashblock_n
hashbundle_1. . . hashbundle_m
```

This data will live either in its own partition or be appended to the verified
partition (aligned on a block boundary). Its location, the hash algorithm used,
and the hash of bundle hashes should be passed in as arguments to the device
mapper setup process (either using dmsetup from an initrd or directly in the
kernel).

Living implementation documentation can be found
[here](https://chromium.googlesource.com/chromiumos/third_party/kernel/+/HEAD/Documentation/device-mapper/dm-verity.txt)
and
[here](https://chromium.googlesource.com/chromiumos/third_party/kernel/+/HEAD/Documentation/device-mapper/dm-bht.txt).

## Other issues, ideas, and notes

*   All verification performed after the kernel is running should be
            independent of the firmware verification. This allows for developers
            to run their own builds as well as for the boot-time verification to
            be compatible with a TCG-style boot.
*   The partition table should be validated to some extent by the
            read-write firmware, but if there are any kernel/firmware partition
            parsing bugs, we may be able to catch them with audits as well.
*   Verified Boot can play nicely with the TPM. Once a signed kernel is
            up, it can initialize the TPM's PCR registers and use those for
            measurement tracking. In particular, we have the option of using it
            (Linux-IMA style) to perform disk encryption key protection and
            possibly even other pre-login state protection where the key becomes
            available only if we've booted without modification.
*   The hash partition will be subject to replay attack unless the
            kernel version that pairs with it is included in the file and the
            kernel is upgraded when the hashes are. However, the kernel+initrd
            will suffer the same attack for the next level up. Avoiding
            replay/rollback attacks is non-trivial since the firmware can't
            guarantee the local clock is not changed. A local TPM tickstamp blob
            would need to be included in the signed hash payloads to solve the
            problem to some degree. If autoupdates were customized per-download,
            this may be possible, but at present, this is not planned.
*   Key management is of utmost importance for the key used to sign the
            read-only firmware. That key should only be used on the R/W firmware
            which should be updated much less frequently than the rest of the
            system. If possible, this key should never be exposed on a
            network-enabled machine, but that is out of scope for this design.
*   Having a fully tamper-evident root file system means that if
            desired, a manifest of service-specific public keys/certificates
            could be stored on the root partition. These keys could then be used
            to verify the authenticity and integrity any data stored on the
            stateful partition that was signed by a remote server (Google or
            another provider).
*   There is no plan to support any remote inspection of whether a
            Chromium OS installation is using a 'verified boot' or a development
            version.
*   The autoupdate process currently has a file-centric view. This means
            that it could be possible for file system layout to diverge across
            machines. If this is the case, block hashes may still be used, but a
            more file-centric view may be needed. If the updater moves towards
            file system image differencing, this design will work as is.

## Attack cases

This section only discusses the current threat model, but many of the attacks
can be generalized to other attack vectors. In addition, these scenarios ignore
all other attack mitigation techniques not included in the document above. In
reality, various system-level and Chromium-level defenses should aid in making
run-time attacks difficult and unreliable.

*Vector*: Opportunistic local attacker with a USB stick or bootable SD card.
*Scenario*: Attacker boots the system off of an external boot device. The
attacker then changes files and copies the entire system.
*Coverage*: Verified Boot will detect this tampering. Encrypted user data will
still be protected.
*Exposure:* None. User will need to recover their system.

*Scenario*: Attacker boots the system off of an external boot device and leaves
the system running a "fake" Chromium OS to phish user data.
*Coverage*: Verified Boot will not impact this *unless*the user reboots the
system before logging in.
*Exposure:* None to complete depending on if the user reboots prior to logging
in. If the user left the machine at the screenlocker, a fake screenlocker could
be used in the phishing OS since it is unlikely a user will reboot before
unlocking. This may be addressed in the future with clever authentication use
(PCR+TPM, ?). However, a paranoid user that left their machine in an unsafe
place may just want to reboot to be safe.

*Vector*: Determined local attacker with a USB drive and a toolkit
*Scenario*: Attacker opens the system up and enables the write-pin on the
write-protected firmware. The attacker then boots the system off of an external
boot device. The root file system is changed along with the formerly
write-protected and read-write firmware.
*Coverage*: Verified Boot will operate normally and will not detect any
variance.
*Exposure:* Complete; a determined attacker that will physically modify the
machine cannot be easily stopped. They may also install hardware keyboard
sniffers, microphones, cameras, etc.

*Vector*: Chromium or a Chromium plugin
*Scenario*: No superuser privileges are gained, but the attacker can modify
Chromium data. The attacker changes bookmarks, starting pages, marks their pages
as ok for popups, and disables safe browsing. In addition, cookies and stored
passwords are harvested and posted to a remote server.
*Coverage*: Verified Boot has no impact.
*Exposure:* Only the initially compromised user is exposed.

*Scenario*: Attacker gains superuser privileges. The attacker remounts root
partition read-write directly. The attacker then replaces
/usr/bin/chromeos-chrome with their own build of Chromium that includes
malware/illegal ad revenue and password/credit card sniffing.
*Coverage*: Verified Boot will detect after the next reboot (not after a suspend
to RAM).
*Exposure:* Until reboot, any user that logs in is exposed (password, cookies,
encrypted data).

*Scenario*: Attacker gains superuser privileges. The attacker remounts root
partition read-write directly. The attacker then adds a kernel module in the
form of a rootkit for the system to load on next reboot.
*Coverage*: Verified Boot will detect after the next reboot.
*Exposure:* Until reboot, any user that logs in is exposed (password, cookies,
encrypted data).

*Scenario*: Attacker gains superuser privileges. The attacker then modifies the
encrypted file system metadata which exploits a file system bug in the kernel on
next login.
*Coverage*: Verified Boot has no direct impact. However, if the autoupdater runs
before next login, the vulnerability may be patched.
*Exposure*: On next login, the tampered with encrypted file system metadata
attack will trigger.

*Scenario*: Attacker gains superuser privileges. The attacker then replaces the
hash-partition with an older version and replaces system image with one that has
known vulnerabilities (which may be easier to exploit reliably than the vector
used for attack). The attacker will then change the current user's configuration
to auto-open an attack URL to re-exploit the system immediately after reboot. If
the attacker can gain superuser privileges repeatedly, then it will be difficult
for autoupdate to repair.
*Coverage*: Verified Boot will not be able to detect hash-partition replay
attacks easily. It may be possible to retroactively detect then by the
autoupdater after the network is up, but an attacker will always be able to the
system appear to just be out-of-date.
*Exposure*: Any user that logs in is exposed across reboots.
*Notes*: Downgrade/replay attacks of this nature will be less dangerous if
autoupdater is able to run prior to Chromium starting, but there will most
likely be a race between the two. It may make sense to include a version check
early in the Chromium startup process to detect seriously out-of-date
browsers/systems prior to their opening dangerous pages.

*Exposure summary:*

*   Run-time attacks against unsignable data: Chromium configuration
*   Persisted attacks by defaulting Chromium to launch the attack site
            on next start
*   Logged-in user data is exposed immediately after compromise.
    *   With partial signing, a Chromium replacement would result in
                cross-user exposure after reboot.
    *   With full signing, other system users would be notified prior to
                exposure
*   Metadata attacks against the kernel will not be caught by signing
            per-file.
*   Downgraded manifest file attacks are difficult to detect since there
            is no current way to encoded tamperproof system time into manifest
            files.
