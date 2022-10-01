---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: security-overview
title: Security Overview
---

[TOC]

## Abstract

*   Chromium OS has been designed from the ground up with security in
            mind.
*   Security is not a one-time effort, but rather an iterative process
            that must be focused on for the life of the operating system.
*   The goal is that, should either the operating system or the user
            detect that the system has been compromised, an update can be
            initiated, and—after a reboot—the system will have been returned to
            a known good state.
*   Chromium OS security strives to protect against an opportunistic
            adversary through a combination of system hardening, process
            isolation, continued web security improvements in Chromium, secure
            autoupdate, verified boot, encryption, and intuitive account
            management.

We have made a concerted effort to provide users of Chromium OS-based devices
with a system that is both practically secure and easy to use. To do so, we've
followed a set of four guiding principles:

*   The perfect is the enemy of the good.
*   Deploy defenses in depth.
*   Make devices secure by default.
*   Don't scapegoat our users.

In the rest of this document, we first explain these principles and discuss some
expected use cases for Chromium OS devices. We then give a high-level overview
of the threat model against which we will endeavor to protect our users, while
still enabling them to make full use of their cloud device.

## Guiding principles

**The perfect is the enemy of the good.** No security solution is ever perfect.
Mistakes will be made, there will be unforeseen interactions between multiple
complex systems that create security holes, and there will be vulnerabilities
that aren't caught by pre-release testing. Thus, we must not allow our search
for some mythical perfect system to stop us from shipping something that is
still very good.
**Deploy defenses in depth.** In light of our first principle, we will deploy a
variety of defenses to act as a series of stumbling blocks for the attacker. We
will make it hard to get into the system, but assume that the attacker will.
We'll put another layer of defenses in place to make it difficult to turn a user
account compromise into root or a kernel exploit. Then, we'll also make it
difficult for an attacker to persist their presence on the system by preventing
them from adding an account, installing services, or re-compromising the system
after reboot.
**Make it secure by default.** Being safe is not an advanced or optional
feature. Until now, the security community has had to deploy solutions that cope
with arbitrary software running on users' machines; as a result, these solutions
have often cost the user in terms of system performance or ease-of-use. Since we
have the advantage of knowing which software should be running on the device at
all times, we should be better able to deploy solutions that leave the user's
machine humming along nicely.
**Don't scapegoat our users.** In real life, people assess their risk all the
time. The Web is really a huge set of intertwined, semi-compatible
implementations of overlapping standards. Unsurprisingly, it is difficult to
make accurate judgments about one's level of risk in the face of such
complexity, and that is *not* our users' fault. We're working to figure out the
right signals to send our users, so that we can keep them informed, ask fewer
questions, require them to make decisions only about things they comprehend, and
be sure that we fail-safe if they don't understand a choice and just want to
click and make it go away.

### Use cases and requirements

We are initially targeting the following use cases with Chromium OS devices:

*   Computing on the couch
*   Use as a lightweight, secondary work computer
*   Borrowing a device for use in coffee shops and libraries
*   Sharing a second computer among family members

Targeting these goals dictates several security-facing requirements:

*   The owner should be able to delegate login rights to users of their
            choice.
*   The user can manage their risk with respect to data loss, even in
            the face of device loss or theft.
*   A user's data can't be exposed due to the mistakes of other users on
            the system.
*   The system provides a multi-tiered defense against malicious
            websites and other network-based attackers.
*   Recovering from an attack that replaces or modifies system binaries
            should be as simple as rebooting.
*   In the event of a security bug, once an update is pushed, the user
            can reboot and be safe.

### Our threat model

When designing security technology for Chromium OS systems, we consider two
different kinds of adversaries:

*   An **opportunistic adversary**
*   A **dedicated adversary**

The **opportunistic adversary** is just trying to compromise an individual
user's machine and/or data. They are not targeting a specific user or
enterprise, and they are not going to steal the user's machine to obtain the
user's data. The opportunistic adversary will, however, deploy attacks designed
to lure users on the web to sites that will compromise their machines or to web
apps that will try to gain unwarranted privileges (webcam access, mic access,
etc). If the opportunistic adversary *does* steal a device and the user's data
is in the clear, the opportunistic adversary may take it.

The **dedicated adversary** *may* target a user or an enterprise specifically
for attack. They are willing to steal devices to recover data or account
credentials (not just to re-sell the device to make money). They are willing to
deploy DNS or other network-level attacks to attempt to subvert the Chromium OS
device login or update processes. They may also do anything that the
opportunistic adversary can do.

For version 1.0, we are focusing on dangers posed by opportunistic adversaries.
We further subdivide the possible threats into two different classes of attacks:
**remote system compromise** and **device theft.**

## Mitigating remote system compromise

There are several vectors through which an adversary might try to compromise a
Chromium OS device remotely: an exploit that gives them control of one of the
Chromium-based browser processes, an exploit in a plugin, tricking the user into
giving a malicious web app unwarranted access to HTML5/Extension APIs, or trying
to subvert our autoupdate process in order to get some malicious code onto the
device.

As in any good security strategy, we wish to provide defense in depth:
mechanisms that try to prevent these attacks and then several more layers of
protection that try to limit how much damage the adversary can do provided that
he's managed to execute one of these attacks. The architecture of Chromium
browsers provides us with some very nice process isolation already, but there is
likely more that we can do.

### OS hardening

The lowest level of our security strategy involves a combination of OS-level
protection mechanisms and exploit mitigation techniques. This combination limits
our attack surface, reduces the the likelihood of successful attack, and reduces
the usefulness of successful user-level exploits. These protections aid in
defending against both opportunistic and dedicated adversaries. The approach
designed relies on a number of independent techniques:

*   Process sandboxing
    *   Mandatory access control implementation that limits resource,
                process, and kernel interactions
    *   Control group device filtering and resource abuse constraint
    *   Chrooting and process namespacing for reducing resource and
                cross-process attack surfaces
    *   Media device interposition to reduce direct kernel interface
                access from Chromium browser and plugin processes
*   Toolchain hardening to limit exploit reliability and success
    *   NX, ASLR, stack cookies, etc
*   Kernel hardening and configuration paring
*   Additional file system restrictions
    *   Read-only root partition
    *   tmpfs-based /tmp
    *   User home directories that can't have executables, privileged
                executables, or device nodes
*   Longer term, additional system enhancements will be pursued, like
            driver sandboxing

Detailed discussion can be found in the [System
Hardening](/chromium-os/chromiumos-design-docs/system-hardening) design
document.

### Making the browser more modular

The more modular the browser is, the easier it is for the Chromium OS to
separate functionality and to sandbox different processes. Such increased
modularity would also drive more efficient IPC within Chromium. We welcome input
from the community here, both in terms of ideas and in code. Potential areas for
future work include:

*   Plugins
    *   All plugins should run as independent processes. We can then
                apply OS-level sandboxing techniques to limit their abilities.
                Approved plugins could even have Mandatory Access Control (MAC)
                policies generated and ready for use.
*   Standalone network stack
    *   Chromium browsers already sandbox media and HTML parsers.
    *   If the HTTP and SSL stacks were isolated in independent
                processes, robustness issues with HTTP parsing or other behavior
                could be isolated. In particular, if all SSL traffic used one
                process while all plaintext traffic used another, we would have
                some protection from unauthenticated attacks leading to full
                information compromise. This can even be beneficial for cookie
                isolation, as the HTTP-only stack should never get access to
                cookies marked "secure." It would even be possible to run two
                SSL/HTTP network stacks—one for known domains based on a large
                whitelist and one for unknown domains. Alternately, it could be
                separated based on whether a certificate exception is required
                to finalize the connection.
*   Per-domain local storage
    *   If it were possible to isolate renderer access per domain, then
                access to local storage services could similarly by isolated—at
                a process level. This would mean that a compromise of a renderer
                that escapes the sandbox would still not be guaranteed access to
                the other stored data unless the escape vector were a
                kernel-level exploit.

### Web app security

As we enable web applications to provide richer functionality for users, we are
increasing the value of web-based exploits, whether the attacker tricks the
browser into giving up extra access or the user into giving up extra access. We
are working on multiple fronts to design a system that allows Chromium OS
devices to manage access to new APIs in a unified manner, providing the user
visibility into the behavior of web applications where appropriate and an
intuitive way to manage permissions granted to different applications where
necessary.

*   User experience
    *   The user experience should be orthogonal to the policy
                enforcement mechanism inside the Chromium browser and, ideally,
                work the same for HTML5 APIs and Google Chrome Extensions.
*   HTML5/Open Web Platform APIs and Google Chrome Extensions
    *   We're hopeful that we can unify the policy enforcement
                mechanisms/user preference storage mechanisms across all of the
                above.
*   Plugins
    *   We are developing a multi-tiered sandboxing strategy, leveraging
                existing Chromium browser sandboxing technology and some of the
                work we discuss in our [System
                Hardening](/chromium-os/chromiumos-design-docs/system-hardening)
                design document.
    *   Long term, we will work with other browser vendors to make
                plugins more easily sandboxed.
    *   Full-screen mode in some plugins could allow an attacker to mock
                out the entire user experience of a Chromium OS device. We are
                investigating a variety of mitigation strategies in this space.

As HTML5 features like persistent workers move through the standards process, we
must ensure that we watch for functionality creeping in that can poke holes in
our security model and take care to handle it appropriately.

### Phishing, XSS, and other web vulnerabilities

Phishing, XSS, and other web-based exploits are no more of an issue for Chromium
OS systems than they are for Chromium browsers on other platforms. The only
JavaScript APIs used in web applications on Chromium OS devices will be the same
HTML5 and Open Web Platform APIs that are being deployed in Chromium browsers
everywhere. As the browser goes, so will we.

### Secure autoupdate

Attacks against the autoupdate process are likely to be executed by a dedicated
adversary who would subvert networking infrastructure to inject a fake
autoupdate with malicious code inside it. That said, a well supported
opportunistic adversary could attempt to subvert the update process for many
users simultaneously, so we should address this possibility here. (For more on
this subject, also see the [File
System/Autoupdate](/chromium-os/chromiumos-design-docs/filesystem-autoupdate)
design document.)

*   Signed updates are downloaded over SSL.
*   Version numbers of updates can't go backwards.
*   The integrity of each update is verified on subsequent boot, using
            our Verified Boot process, described below.

### Verified boot

Verified boot provides a means of getting cryptographic assurances that the
Linux kernel, non-volatile system memory, and the partition table are untampered
with when the system starts up. This approach is not "trusted boot" as it does
not depend on a TPM device or other specialized processor features. Instead, a
chain of trust is created using custom read-only firmware that performs
integrity checking on a writable firmware. The verified code in the writable
firmware then verifies the next component in the boot path, and so on. This
approach allows for more flexibility than traditional trusted boot systems and
avoids taking ownership away from the user. The design is broken down into two
stages:

*   Firmware-based verification
    (for details, see the [Firmware Boot and
    Recovery](/chromium-os/chromiumos-design-docs/firmware-boot-and-recovery)
    design document)
    *   Read-only firmware checks writable firmware with a permanently
                stored key.
    *   Writable firmware then checks any other non-volatile memory as
                well as the bootloader and kernel.
    *   If verification fails, the user can either bypass checking or
                boot to a safe recovery mode.
*   Kernel-based verification
    (for details, see the [Verified
    Boot](/chromium-os/chromiumos-design-docs/verified-boot) design document)
    *   This approach extends authenticity and integrity guarantees to
                files and metadata on the root file system.
    *   All access to the root file system device traverses a
                transparent layer which ensure data block integrity.
    *   Block integrity is determined using cryptographic hashes stored
                after the root file system on the system partition.
    *   All verification is done on-the-fly to avoid delaying system
                startup.
    *   The implementation is not tied to the firmware-based
                verification and may be compatible with any trusted kernel.

When combined, the two verification systems will perform as follows:

*   Detects changes at boot-time
    *   Files, or read-write firmware, changed by an opportunistic
                attacker with a bootable USB drive will be caught on reboot.
    *   Changes performed by a successful runtime attack will also be
                detected on next reboot.
*   Provides a secure recovery path so that new installs are safe from
            past attacks.
*   Doesn't protect against
    *   Dedicated attackers replacing the firmware.
    *   Run-time attacks: Only code loaded from the file system is
                verified. Running code is not.
    *   Persistent attacks run by a compromised Chromium browser: It's not possible to verify the browser's configuration as safe using this technique.

It's important to note that at no point is the system restricted to code from
the Chromium project; however, if Google Chrome OS is used, additional
hardware-supported integrity guarantees can be made.

### Rendering pwned devices useless

We do not intend to brick devices that we believe to be hacked. If we can
reliably detect this state on the client, we should just initiate an update and
reboot. We could try to leverage the abuse detection and mitigation mechanisms
in the Google services that people are using from their Chromium OS devices, but
it seems more scalable to allow each service to continue handling these problems
on its own.

## Mitigating device theft

A stolen device is likely to have a higher value to a dedicated adversary than
to an opportunistic adversary. An opportunistic adversary is more likely to
reset the device for resale, or try to log in to use the device for himself.

The challenges here are myriad:

*   We want to protect user data while also enabling users to opt-in to
            auto-login.
*   We want to protect user data while also allowing users to share the
            device.
*   We especially want to protect user credentials without giving up
            offline login, auto-login, and device sharing.
*   Disk encryption can have real impact on battery life and performance
            speed.
*   The attacker can remove the hard drive to circumvent OS-level
            protections.
*   The attacker can boot the device from a USB device.

### Data protection

Users shouldn't need to worry about the privacy of their data if they forget
their device in a coffee shop or share it with their family members. The easiest
way to protect the data from opportunistic attackers is to ensure that it is
unreadable except when it is in use by its owner.

The [Protecting Cached User
Data](/chromium-os/chromiumos-design-docs/protecting-cached-user-data) design
document provides details on data protection. Key requirements for protecting
cached user data (at rest) are as follows:

*   Each user has their own encrypted store.
*   All user data stored by the operating system, browser, and any
            plugins are encrypted.
*   Users cannot access each other's data on a shared device.
*   The system does not protect against attacks while a user is logged
            in.
*   The system will attempt to protect against memory extraction (cold
            boot) attacks when additional hardware support arrives.
*   The system does not protect against root file system tampering by a
            dedicated attacker (verified boot helps there).

### Account management

Preventing the adversary from logging in to the system closes one easy pathway
to getting the machine to execute code on their behalf. That said, many want
this device to be just as sharable as a Google Doc. How can we balance these
questions, as well as take into account certain practical concerns? These issues
are discussed at length in the [User Accounts and
Management](/chromium-os/chromiumos-design-docs/user-accounts-and-management)
design document, with some highlights below.

*   What *are* the practical concerns?
    *   Whose wifi settings do you use? Jane's settings on Bob's device
                in Bob's house don't work.
    *   But using Bob's settings no matter where the device is doesn't
                work either.
    *   And if Bob's device is set up to use his enterprise's wifi, then
                it's dangerous if someone steals it!
*   "The owner"
    *   Each device has one and only one owner.
    *   *User preferences* are distinct from *system settings.*
    *   *System settings*, like wifi, follow the owner of a device.
*   Whitelisting
    *   The owner can whitelist other users, who can then log in.
    *   The user experience for this feature should require only a few
                clicks or keystrokes.
*   Promiscuous mode
    *   The owner can opt in to a mode where anyone with a Google
                account can log in.
*   Guest mode
    *   Users can initiate a completely stateless session, which does
                not sync or cache data.
    *   All system settings would be kept out of this session, including
                networking config.

### Login

For design details, see the [Login](/chromium-os/chromiumos-design-docs/login)
design document.

At a high level, here is how Chromium OS devices authenticate users:

1.  Try to reach Google Accounts online.
2.  If the service can't be reached, attempt to unwrap the
            locally-stored, TPM-wrapped keys we use for per-user data
            encryption. Successful unwrap means successful login.
3.  For every successful online login, use a salted hash of the password
            to wrap a fresh encryption key using the TPM.

There are a number of important convenience features around authentication that
we must provide for users, and some consequences of integrating with Google
Accounts we must deal with. These all have security consequences, and we discuss
these (and potential mitigation) here. Additionally, we are currently working
through the various tradeoffs of supporting non-Google OpenID providers as an
authentication backend.
**CAPTCHAs**

Rather than strictly rate limiting failed authentication attempts, Google
Accounts APIs respond with CAPTCHAs if our servers believe an attack is
underway. We do not want users to face CAPTCHAs to log in to their device; if
the user has correctly provided their credentials, they should be successfully
logged in.
Furthermore, including HTML rendering code in our screen locker would introduce
more potential for crashing bugs, which would give an attacker an opportunity to
access the machine. That said, we cannot introduce a vector by which attackers
can brute force Google Accounts.
To work around this right now, we do offline credential checking when unlocking
the screen and ignore problems at login time, though we realize this is not
acceptable long-term and are considering a variety of ways to address this issue
in time for V1.

**Single signon**

As we discuss in the [Login](/chromium-os/chromiumos-design-docs/login) design
document, we want to provide an SSO experience on the web for our users. Upon
login, we decrypt the user's profile and perform a request for her Google
Accounts cookies in the background. As a result, her profile gets fresh cookies
and she is logged in to all her Google services.

### Future work

**Auto-login**

When a user turns on auto-login, they are asserting that they wish this device
to be trusted as though it had the user's credentials at all times; however, we
don't want to store actual Google Account credentials on the device—doing so
would expose them to offline attack unnecessarily. We also don't want to rely on
an expiring cookie; auto-login would "only work sometimes," which is a poor user
experience. We would like to store a revokable credential on the device, one
that can be exchanged on-demand for cookies that will log the user in to all of
their Google services. We're considering using an OAuth token for this purpose.

**Biometrics, smartcards and Bluetooth**

We expect to keep an eye on biometric authentication technologies as they
continue to become cheaper and more reliable, but at this time we believe that
the cost/reliability tradeoff is not where it needs to be for our target users.
We expect these devices to be covered in our users' fingerprints, so a low-cost
fingerprint scanner could actually increase the likelihood of compromise. We
were able to break into one device that used facial recognition authentication
software just by holding it up to the user's photo. Bluetooth adds a whole new
software stack to our login/screenlocker code that could potentially be buggy,
and the security of the pairing protocol has been [criticized in the
past](http://www.schneier.com/blog/archives/2005/06/attack_on_the_b_1.html).
Smart cards and USB crypto tokens are an interesting technology, but we don't
want our users to have to keep track of a physically distinct item just to use
their device.

**Single signon**

For third-party sites, we would like to provide credential generation and
synced, cloud-based storage.

## Wrapup

In this document, we have aimed only to summarize the wide-ranging efforts we are undertaking to secure Chromium OS at all levels. For more detail, please read the rest of the [design documents.](/chromium-os/chromiumos-design-docs)
