---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: system-hardening
title: System Hardening
---

[TOC]

Note: A practical guide for hardening individual services can be found
[here](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/sandboxing.md).

## Abstract

*   Chromium OS strives to make remote attacks more difficult by using
            multiple techniques ranging from privilege minimization to
            compile-time hardening.
*   A phased approach to system hardening is proposed to iteratively
            reduce the exposed attack surfaces and increase the number of
            defensive layers.

This document lays out a technical vision for making Chromium OS-based systems
difficult for remote attackers to compromise using various system-level
mechanisms. Three objectives guide this vision:

*   Reducing surface area exposed to attack
*   Reducing ability to successfully and reliably exploit any exposed,
            vulnerable software
*   Reducing benefit of a successful exploitation

Efforts to secure Linux environments tend to revolve around the [principle of
least privilege](http://web.mit.edu/Saltzer/www/publications/protection/) and
applying exploit mitigation tactics wherever possible. While the exploit
mitigation techniques are effective, they are never a perfect defense and often
the specific techniques deployed vary from distribution to distribution. In
addition, the principle of least privilege is excellent in a server environment
and for locking down system services on desktops. However, desktop systems are
meant to be general purpose. This makes it incredibly difficult to determine the
least privilege needed if a program has not ever been seen on the system before
(or was written since the system was installed!). The end result is that the
risks from interactively executed applications are addressed only using exploit
mitigations and not as comprehensively as desired.

Chromium OS has an advantage. All native programs run by the end user are known
in advance since all general purpose applications are web applications. We use
this knowledge to apply comprehensive access control enforcement in addition to
the well-known exploit mitigation techniques. This combination allows Chromium
OS to benefit from the great work securing Linux in both end-user and server
enviroments!

## Technology

[Control Groups
(cgroups)](http://lxr.linux.no/linux+v2.6.30/Documentation/cgroups/) are a
somewhat recent addition to the Linux kernel. They are a hierarchical collection
of tasks that can be arbitrarily created. Once a task has been associated with a
hierarchy, it falls under runtime constraints ranging from limited device node
access to limited CPU and memory usage. cgroups restrictions can be specified in
terms of percentages or as constants which provide an intuitive means of
*"guaranteeing"* that processes operate within their given bounds. This feature
is great for constraining denial of service attacks and general robustness
issues (and combines well with rlimit). The device filtering is useful for
limiting /dev access in constrained namespaces.

[Namespacing](http://lxr.linux.no/linux+v2.6.30/Documentation/namespaces/)
provide a means of isolating processes, and process trees, from other running
processes on a system. Their use has been driven largely by the [Linux
VServer](http://wiki.linux-vserver.org/). Namespaces can be created only when a
new process is started using clone(). The following namespaces are currently
available in the upstream kernel:

*   PID: The process called with a new PID namespace becomes pid=1 and
            is treated as a local *init* for the purposes of reparenting
            orphaned processes. When this process terminates, the namespace is
            shut down. Processes in this namespace can only "see" other
            processes in the namespace and a custom /proc can be mounted to
            provide visibility. A custom mount should be paired with a VFS
            namespace.
*   UTS: UTS namespacing allows for a custom host and domain names to be
            set for a given namespace.
*   User: User namespacing remaps users to new, localized UIDs. At
            present, this functionality is immature in the kernel as many
            guarantees are not met, such as similar UIDs in different namespaces
            sharing a global UID.
*   IPC: This isolates SystemV IPC ids to the processes in this
            namespace. This means that globally shared memory, semaphores, and
            message queues are accessible only to other processes in the same
            namespace.
*   [VFS](http://www.ibm.com/developerworks/linux/library/l-mount-namespaces.html):
            VFS namespacing allows for a custom view of the locally mounted file
            systems. In addition, any file systems mounted in a VFS namespace
            will automatically be unmounted when all the processes in the
            namespace have terminated. In addition, mount namespacing also comes
            with interesting new sharing attributes (make-private,
            make-unbindable, make-slave, make-shared).
*   [NET](http://lxc.sourceforge.net/network.php): This namespace
            isolates the processes from the network interfaces, defaulting to
            having access only to the loopback interface.

In addition to namespacing and cgroups, Linux supports [parceling superuser
privileges using
capabilities](http://www.ibm.com/developerworks/aix/library/l-posixcap.html).
Privileges that were once limited to uid=0 are now available in a coarse-grained
fashion using runtime and file system (extended attribute)-based labeling. In
addition, process tree capabilities inheritance is possible with a [lightweight
kernel patch](http://marc.info/?l=linux-kernel&m=125026482525494&w=2). With file
system capabilities enabled, specific process trees can also disable uid=0 from
having any default privilege (other than that granted by file system
permissions) using the securebits SECURE_NOROOT and SECURE_NO_SETUID_FIXUP. All
processes in the subtree could then be locked into this pure capability-based
superuser privilege mode barring a kernel privilege escalation vulnerability.
All capabilities are broken into effective, permitted, and inherited sets, which
can be applied to a file or process. In addition, processes all have a starting
bounding set that places an upper limit on which capabilities can be used, even
if in one of the other sets. At present, we're not aware of any Linux
distributions that make heavy use of capabilities.

[Linux Security Modules](http://lwn.net/Articles/154277/) (LSM) is a subsystem
of the Linux Kernel Modules that implements a number of kernel task-based hooks.
It allows for a security module to be implemented that enforces mandatory access
controls and can be stacked (if supported by the module) with other security
modules. [Tomoyo](http://tomoyo.sourceforge.jp/) 2.x,
[SELinux](http://www.nsa.gov/research/selinux/index.shtml), and
[SMACK](http://schaufler-ca.com/) are examples of these systems.

[grsecurity](http://grsecurity.net/) is a standalone kernel patch that provides
role-based access controls, kernel hardening, and bug fixes, as well as
extensive detection functionality.

The kernel supports a notification system to inform userland of process events:
fork, id changes, so on. The [process events](http://lwn.net/Articles/157150/)
subsystem allows for low-cost monitoring of task creation and removal as well as
other pertinent runtime information. Communication is handled over a netlink
connection provided by the CONFIG_CONNECTOR option.

## Detailed design

This project will be undertaken iteratively to increase both the amount of
process isolation and overall system security as the implementation proceeds.
To reduce the surface area exposed to attack, we aggressively isolate processes
to access only what they require to function. Applying the [principle of least
privilege](http://web.mit.edu/Saltzer/www/publications/protection/) reduces the
total amount of code exposed at any one point and, ideally, minimizes the risk
that the exposed code contains a vulnerability.
Since it's not practically possible to isolate any software so completely as to
guarantee that vulnerable code isn't exposed, we use runtime and compile-time
exploit mitigations to increase the difficulty of a successful attack. In
addition, the mitigations used should make viable exploits behave unreliably
across Chromium OS systems.
Again, we assume that a *perfect* defense is unattainable, but providing a very
good defense will be on-going, iterative work which we all must contribute to.
To that end, we want to make successful exploitation less beneficial for the
attacker. For successful user-level exploits, this is done using the same
isolation techniques used to minimize exposed kernel attack surfaces. Successful
kernel-level exploits may be partially contained through some kernel hardening
patches, but more extensive fault isolation approaches have not yet been
explored.

### Containment options

Two main approaches will be supported for containment:

*   minijail: a tiny, custom launcher that handles namespacing, control
            groups, chroot'ing, and more.
*   A mandatory access control mechanism with automated learning mode,
            like Tomoyo or grsecurity.

#### minijail

*minijail* is a small executable that can be used by root and non-root users to
perform a range of behaviors when launching a new executable:

*   Dropping capabilities from the bounding set
*   Enable/disable capabilities
*   chroot
*   Dropping root (uid+gid)
*   Setting securebits (SECURE_NOROOT, SET_DUMPABLE)
*   Setting rlimits
*   Process namespacing:
    *   Will act as init for any pid namespaces
    *   Will mount /proc in any child vfs namespaces
    *   Will support uts namespacing (not required)
    *   Will support user namespacing (in the future)
    *   Will support IPC namespacing (binary decision)
    *   Will support net namespacing (for locking down network
                interfaces w/veth)
*   A generic [suid
            sandbox](http://src.chromium.org/viewvc/chrome/trunk/src/sandbox/linux/suid/sandbox.c?revision=25019&view=markup)
            (using the features above)

The final implementation should include a standalone library, libminijail, and a
command-line tool that supports individual feature specification or the loading
of specific, preconfigured profiles.

Regardless of the implementation, the binary will need the cap_setpcaps extended
attribute set (+ep) to be able to operate without root privileges. (If we move
to a root file system that doesn't support extended attributes, any specialized
binaries can be mounted off of an ext3 loopback, or the kernel-based inheritance
patch can be used.)

#### Mandatory access controls (MAC)

If we are able to use kernel hardening patches (discussed later), we should
seriously consider using grsecurity. If not, we will use the in-kernel solution
Tomoyo. grsecurity supplies an effective automatic rule generator and includes a
large number of kernel hardening features and bug fixes that will otherwise be
missed.
Tomoyo will provide mandatory access controls to ensure that processes don't
exceed their expected boundaries. Initially, the coverage will be largely
limited to additional file system enforcements. Protection around ptrace, ioctl,
and other areas are needed before this will be considered a good solution.
Whichever MAC solution we choose will be configured to run in enforcement mode
for all processes, except when running in development mode. In that case, the
development mode parent process runs in permissive mode to allow the developer
to take whatever actions he wishes.

### Deployment

System hardening will proceed in roughly three phases. The first phase applies
comprehensive userland isolation using the two main containment options.

### Phase 1: Surface changes

Phase 1 focuses largely on putting existing services (and user logins) in to
SECURE_NOROOT+namespaced jails with as little system impact as possible. This
can be done by tweaking initialization scripts and modifying file extended
attributes. In particular, this will be done in our custom upstart configuration
files and other boot scripts.

#### Nobody puts user in a corner: Or, taking root from the user.

It turns out that this is pretty easy as long as we don't focus on making all of
Xorg non-root-based. For example, there is a command run by the login manager
upon successful user authentication:

/bin/bash --login /etc/X11/Xsessionrc %session

which we could change to

/sbin/capsh --secbits=0x2f --drop=\[all\] -- --login /etc/X11/Xsessionrc
%session

in order to run the user's session in SECURE_NOROOT, if we were using the capsh
tool (capabilities-aware shell).

However, since we have minijail, we can use it to do more than just run in
SECURE_NOROOT:

/sbin/minijail --init --namespace=pid,vfs --cgroup=chromeuser --secbits=0x2f
--drop=\[all\] --exec=/bin/bash -- --login /etc/Xsessionrc %s

This will dump the new process in its own namespace where minijail acts as pid
1. The entire X session will only be able to see a /proc that is related to its
pid namespace, and it will have access only to devices enabled for the cgroup:
chromeuser. No SUID binaries or binaries with any additional extended capability
attributes set will be executable in this session.

The biggest impact is that when it comes time to perform screen unlocking, the
xscreensaver process will not be able to do anything privileged.

**Note:** We have not chroot'd or net-namespaced the binaries. At present, /dev/
is limited by cgroup filtering (discussed below) and by mounting a fresh /proc
with the namespace view. While we will have stripped power from any simple root
privilege escalation attack, a user running as uid=0 will still have normal
discretionary access controls to a crazy number of files and devices as the root
user. In Phase 2, we will look at further segmenting access.

Big Note: Any privileged actions must be brokered by preconfigured, preexecuted
binaries.

With this Big Note in mind, we can look back at our slim.conf. If we find that
we need to launch some processes with some privileges, we can tone down how
aggressive the first call to minijail is. It can set up the namespace and lock
down root, but it can leave the bounding set a bit wider; that way, we can
launch utilities that may need capabilities like pulseaudio. This can be done
inside the Xsessionrc by calling minijail on all subsequent binaries with
specific bounding set changes, etc. They can all, thankfully, live in their new
pid namespace unless we lock them down further (chroot, etc). Initially, we'll
start with the above configuration and tweak as problems are introduced.

Guiding resource utilization with control groups

Control groups (cgroups) will be used to segment the population with respect to
device access and resource utilization. To that end, we can preconfigure a few
control groups at start via a simple /etc/init.d/cgroups script:
mkdir /cgroup
chown root:root /cgroup
chmod 700 /cgroup
mount none /cgroups -t cgroup -o cpu,memory,dev
mkdir /cgroups/services/network
mkdir /cgroups/services/autoupdate
mkdir /cgroups/user/chrome
mkdir /cgroups/user/chrome/sandbox
mkdir /cgroups/user/chrome/plugins/flash
mkdir /cgroups/user/xorg
With that done, we can leave it to minijail invocations to add the pid to the
/cgroups/&lt;cgroup&gt;/task file. The biggest challenge will be nested cgroups
like chrome/sandbox, since we will not mount /cgroups in the chrome user
namespace and users will be unable to see the /cgroups file system. In Phase 1,
we'll just have to let renderers live in the chrome cgroup and hope for the
best. Segmenting chrome user processes from the system services should be enough
to guarantee that a Chromium browser CPU DoS won't peg the system too badly, but
we'll see. If it can tie up xorg, the user experience will be the same. However,
in Phase 2, we will introduce a cgroupsd daemon. This daemon will monitor new
process creation (via a TBD mechanism with _low_ power/cpu needs) and
automatically add them to the appropriate cgroup.

Devices will be added quite simply with:

echo 'c 1:3 mr' &gt; /cgroups/1/devices.allow

Memory per group can be determined based on system memory. Below, limit chrome
to using 80 percent of available memory:
total_mem=$(free -b | grep Mem | tr -s ' ' | cut -f2 -d' ')
echo $((total_mem / 5 \* 4)) &gt; /cgroups/user/chrome/memory.limit_in_bytes
CPU usage can also be determined using the system total, which is available in
cpu.shares. Below, we give all chrome processes 80 percent of the CPU shares:
total_cpu=$(cat /cgroups/cpu.shares)
echo $((total_cpu / 5 \* 4)) &gt; /cgroups/user/chrome/cpu.shares
Of course, we can tweak the total number of shares to make specific allocations.
The allocations should then be used for fair scheduling.
Longer term, we may be able to use 'freezer' support to freeze all processes
prior to suspend or use cpusets to ensure that the Chromium browser, or perhaps
even an [extension](http://code.google.com/chrome/extensions/), is privately
allocated an entire CPU core (using *cpusets*). In addition, if any of these
items imply too much overhead, it is possible to achieve similar (and even more
focused) results using RLIMITS.

#### Locking down existing daemons with extended attributes and a bit of luck

#### At present, there are a number of processes running as root:

*   SLiM and X: both run with privilege. SLiM starts Xorg, and Xorg
            needs ioctl and ioperm access, which equates to root access in most
            cases. We will explore a non-root Xorg in Phase 2.
*   connman will need CAP_NET_ADMIN, CAP_NET_RAWIO at most in its
            bounding set so that they may be used by properly annotated files:
            ping, dhclient, wpa_supplicant.
*   dhclient is used for acquiring a network address and configuring an
            existing network device. It needs to broadcast UDP and change
            network device parameters.
*   wpa_supplicant is used for configuring the wireless device to
            properly associate with wireless access points.
*   acpid handles power management and other events: lid close, low
            battery, etc.
*   getty handles the standalone console, which will most likely be
            disabled in nondeveloper installs.
*   udev handles firmware loading and hot-pluggable device.

The final goal is to move to a pure capability-based system which means that no
service should *need* root access. To this end, we'll need to modify the startup
process for these daemons. The easiest approach is to just wrap their
start-stop-daemon calls with calls to minijail, either in the control panel or
in /etc/init.d. Each one should get its own namespacing with chroot'ing if
possible. (If it seems difficult to chroot a specific binary, then we should
consider doing so with the Chromium OS project's
[LinuxSUIDSandbox](http://code.google.com/p/chromium/wiki/LinuxSUIDSandbox).)
Capabilities required will be determined using *strace | grep 'EPERM|EACCESS'*
while locking down the binaries. Each binary will have the capabilities it needs
added to its extended attributes. For example, dhclient needs
CAP_NET_BIND_SERVICE|CAP_NET_BROADCAST|CAP_NET_ADMIN:

capset cap_net_bind_service,cap_net_broadcast,cap_net_admin=ep /sbin/dhclient3

Then, dhclient will be called with minijail dropping all capabilities except
those three (and cap_setcaps if needed). If dhclient is called from connman,
then that process can already be running with a restricted capability bounding
set.

At present, our root file system supports extended attributes. If we move to
squashfs or another file system without xattr**, we will have to work around the
restriction. This can be done using the patch referenced in the Technology
section or by mounting a loopback file system with the desired binaries with the
appropriate xattrs.**

#### Supporting development mode before we finish designing it

Once we dump the user in SECURE_NOROOT land, sudo and su become useless. To
allow continued tinkering, we can use the secondary console if enabled or a
loopback ssh daemon. As long as we don't lock these alternative entry points the
same way as the primary user session, they will be perfectly sane ways to
implement a secure but useful development mode.

#### Adding fine-grained controls over coarse-grained capabilities with mandatory access controls

For our Mandatory Access Control (MAC) needs, we are considering the external
grsecurity kernel patch as well as the currently in-kernel Tomoyo module and
accompanying tomoyo-ccstools package. In either case, a similar approach
applies. Once installed, an initial policy can be configured using learning
mode. We can then enable enforcement on process trees which shouldn't change:
dhclient, wpa_supplicant, etc. The Chromium browser itself will likely run in a
permissive mode as we explore extensions and other changes. However, as we
approach releases, we can configure a final policy using learning mode both with
active users and through automated testing that exercises all the expected use
cases for the system.

The Tomoyo tool for editing the policy is ccs-editpolicy. Initially, there
should be no need to perform any special customization other than converting
process trees from disabled to learning or permissive. In addition, we will need
to make sure that the development mode runs in permissive or disabled mode.

#### Locking down the file system (discretionary access controls)

Discretionary access controls have been satisfactory at implementing basic
security in Linux for years. An audit of the root file system will ensure that
no end user can read files or directories he has no need to read, execute files
he has no need to access, or chdir to directories he has no need to enter. This
effort will be further expanded by the Phase 2 efforts around changing file
ownership to limit root's discretionary access—even when its capabilities have
been revoked.
In addition to the normal file system, /dev and /proc can be dangerous for an
unprivileged root user. While we are filtering devices per-cgroup, we should
ensure that CONFIG_STRICT_DEVMEM is set to limit /dev/mem usefulness. If we
patch Xorg, we may be able to get rid of /dev/mem entirely. In addition, a
review of what is available in /proc and /dev for each group will be crucial.
Whenever we place a process tree in a new VFS, we can mount --bind in only the
files we want. This is harder with /proc, but doable if needed. /proc may be
remounted read-only at the very least. We may be able to make use of the Linux
VServer's 'setattr' tool to hide /proc entries on namespace mount. If so, this
would be done in the minijail code but would require that we support the vserver
kernel patches. However, namespacing and chroot'ing will hopefully cover a lot
of ground.
User home directories and the /home partition should be mounted nosuid, nodev,
and ideally, noexec. We should attempt to limit user access to included
scripting engines if possible to to aid in enforcing noexec (dash, bash, or any
others).

#### Deploying the firewall

The netfilter/iptables infrastructure provides a number of interesting
approaches for limiting network access, both inbound and outbound. While a basic
inbound-only policy will work for general TCP and UDP level attack protection,
it would be nice to limit the OUTPUT chain as well. We can also consider using
network namespaces and VETH devices, but for Phase 1, the added complexity and
potential robustness issues make it questionable (see Phase 2 for more detail).

#### Xorg without the root user

On a testing system, we have restricted Xorg to CAP_SYS_RAWIO and seen
everything work except ioctl() access to the graphics device. We believe there
to be patches that deal with this, but getting them working within our codebase
may require some work. In addition, we can't just drop privilege because that
will make returning from suspend quite painful.

#### SLiM and pam_google with limited capabilities

Any privileged behavior needed by pam_google will need to be moved into a
standalone daemon. For instance, any encrypted volume management will need to be
shifted into a daemon that handles it for the user. A simple daemon that checks
SO_PEERCRED and the current mount state would do the trick.

### Phase 2: Diving deeper

Phase 2 is where we start making changes that are farther reaching.

#### cgroupsd

cgroupsd is a simple daemon that will automagically add new processes to a
control group specified in a libcg-style configuration file. The only useful
design point is that it will do so by using CONFIG_CONNECTOR and
CONFIG_PROC_EVENTS to be notified by the kernel of new processes via netlink. It
will need access to the /cgroups mountpoint, but otherwise, will not need any
additional privileges. In particular, if cgroupsd is used *only* for Chromium
browser sandbox processes, it can run with privileges to modify only
/cgroups/user/chrome/sandbox/tasks.
cgroupsd will also be used to enforce device filtering and resource management
on plugins like Flash.

#### Other local system management daemons (network, sound, removable storage, ?)

The further we lock down the user's session, the more work we'll have to put in
to brokering access to system resources. The control panel approach is great for
system management brokering (via a network loopback), but we will need to make
sure that udev and hal access can be handled safely.
We're probably also going to see some pain with Adobe Flash and other binary
plugins unless we give them access. Reviewing and integrating plugins with this
design will be critical to avoiding introducing a trivial backdoor through the
protections.

#### Put browser instances in their own namespaces

Chromium browsers can make use of the measures deployed in Phase 1. If the
existing sandbox isn't isolating rendered processes in their own pid namespace,
then it should be done here. In addition, every browser process itself can be
dropped into its own namespace when launched.

#### Applying net namespaces

One possible approach to isolating processes further is putting them all in
their own [net namespace](http://lxc.sourceforge.net/network/configuration.php).
We can then expose to the system a virtual interface with a virtual, internal,
IP address. We could even optionally enforce userland proxy use (for truly dodgy
inmates). However, this may introduce robustness issues if a user is assigned a
physical address that is the same or in the same netmask as the virtual ones.
Given that we can't control the eth0 address, we have delayed pursuing this
until Phase 2. When we get there, it will be worth investigating and deploying
if possible to keep any process from being able to bind to an external port.

#### Re-chowning the file system, or why root shouldn't own *everything*.

Even in a SECURE_NOROOT environment, root still has discretionary access control
to a large number of files and devices, and that access may be used to escalate
privileges or make system changes. One possible approach is to create multiple
system accounts that are responsible for files along some logical domain. One
option would be to use a var user and a home user and a bin user for each of the
areas they may own. A privileged user can always override such discretionary
access control mechanisms, but it will stop any accidental root access from
being completely detrimental.
That change may be overkill or add more complexity than the gain. Another option
would be to add a new secure bit along the lines of SECURE_UNSAFE. If that
secure bit is set on a tree, then no process in the tree can change to UID/GID
0.

#### Device interposition

Given that Chromium browsers and plugins need access to the webcam and audio (in
and out), it's important to isolate the kernel device drivers from software that
may be attacker controlled. To this end, access to /dev/video0 will be brokered
via a userland daemon. The work may be based on
[GSTFakeVideo](http://code.google.com/p/gstfakevideo/). Not only will this avoid
direct attacks on random webcam drivers, it will also mean we can later offer an
interface for doing real-time video stream filtering: custom effects, etc.
In addition to /dev/video, we'll want to position userland code for audio
interception (e.g, /dev/dsp, etc). This can be done using something like esound
or pulseaudio. If we go with one of those daemons anyway, we can get this for
free.
After the audio/video experience, we're left with one major exposed surface for
plugins which require video card device access. Since we will want to support
accelerated 3D and other fast rendering, we'll be exposing (possibly
binary-only) video card drivers via X/DRI. This is a larger problem that will be
addressed in a more detailed design document on the issue.

#### Monitoring

If we can monitor our system for any clear signs of compromise without seriously
affecting battery life or performance, then we should! This area should be
branched out into a full standalone design document. But within the scope of
this document, it is worth considering a very simple system.
A single daemon can monitor process creation and uid changes via the proc events
kernel interface. If it sees any process become uid/euid==0, then we have
someone running a privilege escalation exploit. If we determine an
exceptional-event user interface or a reboot path that will notify the user to
put-a-paperclip-in to reset the device, then we can trigger it immediately.
While an exploit can target this behavior, it is just one more layer of defense.
The Linux Auditing Framework may be very useful for doing detection, but its
cost may outweigh the benefit. Since we are not expecting a huge number of
process creation events, we can monitor system calls ranging from ptrace to
clone(2) to fork. If we avoid high traffic system calls, we should be able to
enforce some basic system call detection without sandboxing explicitly. In
addition, if we don't use auditd, but instead a [custom
listener](http://people.redhat.com/sgrubb/audit/audit-rt-events.txt), we can
immediately react to an event—such as terminating the calling process or
triggering a reboot into the recovery system.

### Phase 3: Don't forget your snorkel!

Phase 3 is where we get to explore additional innovations in the security space
that will require the most long-term investment. We're excited about the
possibilities around integrating new kernel and user space hardening techniques
and figuring out how to properly isolate drivers in kernel-space.
Here are some of the ideas we have, but there is a lot of area to research:

*   Retrofit /sbin/init and remove root everywhere.
*   Isolate all running Xorg windows in Xephyr transparently to mitigate
            keystroke theft, etc. (is the benefit worth the extra code?)
*   Custom Linux Security Module for doing nested runtime sandboxes.
*   Device driver security: We need to analyze device drivers and
            determine a plan for isolation and or robustness.
    *   Using [KLEE](http://hci.stanford.edu/cstr/reports/2008-03.pdf)
                or other automated dynamic analysis suite
    *   Consider vt-d/trustzone approaches for driver isolation
                (l4linux, 64-bit friendly KERNSEAL/KERNEXEC?, Nooks)
*   Harden the kernel heap management.
*   Chromium-browser-based isolation of user data and processes by site
            domain.
*   Chromium browser http and https network stack isolation (as
            processes) to protect secure cookies from evil sites accessed over
            HTTP.
*   Further harden userland/kernel interfaces.

### Designing and developing for security

Future software written for and included in Chromium OS-based devices must not
work around or otherwise undermine any of the features implemented to ensure
system security. In addition, it is important that software used in Chromium OS
receive sufficient peer code reviews, manual and automated security testing, as
well as security code audits. Security testing and code review processes should
be discussed in more detail in another document. Obviously, we feel that all
Chromium OS devices should only run software that follows these guidelines, but
we can only ensure that this is so for our official builds.

### Exploit mitigation

In addition to the containment plan, there are steps we can take to hardening
our user and kernel toolchains and harden the kernel itself. Toolchain hardening
and kernel patching should be performed opportunistically and are not tied to
the phased implementation laid out above.

#### Kernel toolchain and patches

There are a huge number of potential changes to the kernel we can pursue. We'll
start with known approaches and then expand as permitted into newer areas as
possible:

*   [PaX](http://pax.grsecurity.net/docs/) provides a number of useful
            exploit mitigation techniques as a standalone patch to the Linux
            kernel. grsecurity and PaX are available in one bundled patch. If at
            all possible, PaX and grsecurity should be applied to the kernel.
*   Ensure that we don't optimize out NULL pointer checks as mmap(0)
            tricks are the current trend in kernel exploitation.
    -fno-delete-null-pointer-checks
*   The kernel should be compiled so that it is relocatable:
    CONFIG_RELOCATABLE=y
*   Compile out the vdso mapping unless we are really using something
            that needs it:
    COMPAT_VDSO=n
*   A minimum mmap address must be enforced (&gt; NULL):
    CONFIG_SECURITY_DEFAULT_MMAP_MIN_ADDR
*   The kernel should be compiled with stack smashing detection support:
    CONFIG_CC_STACKPROTECTOR=y CONFIG_CC_STACKPROTECTOR_ALL=y
*   System call patching: we should patch out, or severely restrict,
            system calls that are new or potentially risky (e.g., ptrace,
            vmsplice).
*   Rebooting on soft panic: Currently, the kernel will try to continue
            after a soft panic. To avoid a soft panic being caused by a
            less-than-robust compromise, it'd be better to just reboot—which
            leads into the next point.
*   [kexec-on-panic](http://lwn.net/Articles/108596/): While
            [kdump](http://lxr.linux.no/#linux+v2.6.30/Documentation/kdump/kdump.txt)
            is meant to aid in enterprise-grade kernel debugging, we can
            automatically boot over to a standby kernel if something goes wrong
            with our primary kernel. This means that we will gain the long term
            option of uploading kernel crash dumps, but at the very least, we
            could boot a kernel that displays "Awww, snap!" and safely
            reboots—or calls the autoupdater. While an attacker could target
            modifying the kexec backup kernel, it seems an unlikely target if
            the attacker can modify the running kernel (but we expect someone
            will do it for fun). Note, this is not an option for ARM.
*   Additional kernel configuration audit: remove all unneeded features.
*   Disable automodule loading after the system is brought up. One
            approach that was suggested is running sysctl -q
            kernel.modprobe="/usr/bin/logger" and then rmmod -a after the system
            is up if we can't move to a module-less kernel build.
*   Enforce that system calls can't be called directly from writable
            memory segments, like the heap.

#### Userland toolchain

The userland toolchain should enforce a number of requirements (which will need
benchmarking):

*   -fno-delete-null-pointer-checks
*   -fstack-protector: stack canary
*   -pie: relocatable executables (and marked ET_DYN)
*   -Wl,-z,relro: read-only relocations
*   FORTIFY_SOURCE

Longer term, we'll also look at:

*   Additional glibc heap hardening
*   Extending stack protector to obscure return addresses on the stack

### Quantifying effectiveness

In order to quantify effectiveness, we should take the time to truly enumerate
the attack surfaces yielded by these hardening steps:

*   Available files
*   Files with privileges (capabilities, suid, /proc, /dev, /sys)
*   System calls
*   User id separation

In addition to enumeration of attack surfaces, we should also implement
proof-of-concept attacks for each layer of our defenses by placing vulnerable
code at various points and directly testing the security properties at that
point. To supplement explicit tests, we will also pull in new exploits for newly
vulnerable code and evaluated if it is successful, how reliable it is, and what
we can do to protect against something like it in the future.

## Plan

The phases as laid out will be followed with the end goal of having Phases 1 and
2 completed fully in a few months -- including toolchain hardening.
Attack-oriented testing may be performed as part of the functional testing
during deployment, but if not, it should be completed immediately after phase 2
along with a full system review. In addition to retrospective analysis of the
security, as new attacks emerge for Linux and Chromium OS, we will need to
evaluate their behavior and determine how to continue to futureproof the system
from a security perspective!
