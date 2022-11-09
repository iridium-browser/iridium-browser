---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: lucid-sleep
title: Lucid Sleep
---

[TOC]

# Overview

Some recent Chrome devices have the ability to wake up from a suspended state to
perform work. Potential tasks include:

*   Associating with a known WiFi access point that has just come into
            range
*   Updating user data in response to a received push message
*   Synchronizing with app servers at the request of an app
*   Performing system maintenance like trimming SSDs or checking the
            battery level

To minimize power consumption while performing tasks and external indications
the device is performing work, the device uses a hybrid power state called Lucid
Sleep. Effectively using this state requires coordination between all parts of
the system, from the kernel to the system daemons to the Chrome browser.

This page describes the interactions between the system components to coordinate
Lucid Sleep. It is intended to help with firmware and kernel implementations and
troubleshooting of this feature on Chrome devices that support it.

# Kernel

When a Chrome device is suspending, the kernel performs the following steps:

1.  Userspace processes are frozen. Every userspace process is removed
            from the scheduler’s run queue and no more CPU cycles are allocated.
2.  Select kernel threads are frozen, depending on the device. Exactly
            which threads are frozen depends on the hardware configuration; in
            some cases, no kernel threads are frozen.
3.  Hardware components are suspended, for example the display panel is
            turned off, the hard drive is stopped, and USB ports are powered
            down.
4.  All non-boot CPUs are disabled.
5.  The kernel programs the underlying hardware with a list of interrupt
            sources that are allowed to wake up the system, for example the
            keyboard or power button.
6.  The kernel puts the boot CPU into a low-power state so that no code
            is running on that CPU.

Resuming a Chrome device is essentially the reverse of the suspend sequence:

1.  The hardware determines that a wakeup event has occurred and
            delivers an interrupt to the boot CPU.
2.  The boot CPU starts running the kernel code.
3.  Non-boot CPUs are re-enabled.
4.  Hardware components are resumed.
5.  Any frozen kernel threads are thawed.
6.  Userspace processes are thawed and the system resumes normal
            activity.

To support Lucid Sleep, the kernel allows userspace to expand the list of
interrupt sources that can trigger wakeup events. It is also possible to specify
sources that trigger Lucid Sleep versus a full system resume. Hardware
components that can trigger wakeup events must run independently in a low-power
state without kernel support. Conversely, kernel drivers for components that do
not trigger wakeup events must not re-initialize the hardware during Lucid
Sleep.

For example, a device receives a push message over the network. The power
manager adds the Network Interface Card (NIC) to the list of hardware components
that can trigger a wakeup interrupt. It also adds the display panel and USB
ports to the list of components that must not re-initialize during Lucid Sleep.
At suspend time the NIC driver puts the NIC into a low-power state rather than
turning it off. Other suspend operations proceed as normal.

After a wakeup event is triggered and before hardware components are resumed,
the kernel determines if the wakeup event should trigger Lucid Sleep or a full
resume and saves this value. When the hardware components are resumed, the
device drivers query the kernel to determine whether the wakeup is for Lucid
Sleep. If so, and userspace has requested that that hardware remain off, the
driver skips the remainder of re-initialization steps. After userspace processes
thaw, a process can also query the kernel via the sysfs interface to determine
if the wakeup event triggered Lucid Sleep.

# Power Manager

The power manager provides a mechanism for notifying the system daemons and
Chrome when the device is in Lucid Sleep. This mirrors the mechanism used for
notifying the system daemons when the Chrome device is about to suspend. A
process can register with the power manager on start up. When the system enters
Lucid Sleep, the power manager notifies registered processes by sending out a
DarkSuspendImminent D-Bus signal. This signal is the only external indication
that the system is in Lucid Sleep.

When a device enters Lucid Sleep, the power manager also detects that the wakeup
event was not user-triggered and it should suspend the device again as soon as
possible. The DarkSuspendImminent signal both announces Lucid Sleep and signals
that suspension is imminent.

When a suspend operation successfully completes (and userspace processes have
been thawed by the kernel), the power manager queries the kernel to see if the
current resume operation is from Lucid Sleep. If so, it sends out a
DarkSuspendImminent signal and waits for all applications that have registered
to receive the signal to report on readiness to suspend. After all registered
applications have reported readiness or the maximum delay has passed, the power
manager again instructs the kernel to suspend the system.

## Handling user activity

Occasionally, a user may start to interact with the Chrome device while it is in
the middle of Lucid Sleep. To ensure that all hardware components are properly
resumed, in this case the power manager instructs the kernel to perform a test
suspend. The normal suspend sequence is interrupted after suspending hardware
components, and the kernel immediately jumps to the step of resuming hardware
components in the resume sequence. This ensures that all hardware components are
ready before the user starts interacting with the system.

# Shill

Shill, the connection manager, decides what WiFi-related actions—such as
programming the wireless NIC and setting RTC timers—to take before the system
goes into suspend, when the system wakes up in Lucid Sleep, and when the system
fully resumes. The primary goals of these actions are to maintain WiFi
connectivity while the system is suspended or in Lucid Sleep and to allow the
system to respond to important events (e.g. GCM push notifications).

Before the system suspends from a fully awake or Lucid Sleep state, shill
verifies whether the system’s WiFi interface is connected to a network. If the
system is connected, shill programs the wireless NIC to wake the system if the
NIC disconnects from that network, and starts an RTC timer to wake the system in
Lucid Sleep to renew its IPv4 DHCP lease (or obtain a new IPv6 configuration)
when it is about to expire. If the system is not connected, shill programs the
wireless NIC to independently perform periodic scans while the system is
suspended, and wake the system if any of its preferred networks are found. If
the number of preferred networks is greater than the NIC whitelist capacity, a
separate RTC wake timer is started to wake the system in Lucid Sleep to perform
shill-initiated scans. Shill also programs the wireless NIC to wake the system
upon receiving packets from whitelisted IP addresses registered by Chrome (more
details in next section).

When the system wakes up in Lucid Sleep, shill receives a wakeup reason from the
kernel. If the reason is a disconnect or detection of a preferred network, shill
triggers a passive scan and attempts to establish a connection to any networks
found. If the reason is the receipt of a packet from a whitelisted IP address
(i.e. a GCM push notification), shill does nothing and allows Chrome to handle
the packet. For any other reason, including the aforementioned RTC timers, shill
either renews DHCP leases if the system is already connected, or performs a scan
and if the system is not connected, attempts to establish a connection.

When the system fully resumes, shill deprograms all wake on WiFi triggers on the
wireless NIC, and stops any RTC timers that shill might have started before
suspend or during Lucid Sleep. Shill will then scan for networks if the system
wakes up not connected, and will resume its normal functionality thereafter.

# Chrome

The Chrome browser software handles push messages that arrive over the network.
It registers with the power manager so that the power manager will wait for it
to report readiness before suspending from Lucid Sleep. The progress of all push
messages is tracked from arrival to completion. If Chrome receives a
DarkSuspendImminent signal while one or more push messages are active, it waits
until no more active push messages remain before informing the power manager
that it is ready to suspend. Additionally, any network requests started by a
message consumer during processing can further delay Chrome before it reports
readiness to the power manager.

Chrome also tells the NIC which connections to watch for incoming messages.
Currently, connections to Google Cloud Messaging (GCM) servers are supported.
After Chrome establishes a connection to the GCM server, it informs the NIC of
its IP address. While the system is suspended the NIC continues listening for
network activity and when a packet arrives from the IP address of the GCM
server, the NIC sends an interrupt to the boot CPU to wake the system into Lucid
Sleep.

To maintain a connection with the GCM servers, Chrome sends a heartbeat message
to the server at regular intervals. A specialized timer (the
[AlarmTimer](https://code.google.com/p/chromium/codesearch#chromium/src/components/timers/alarm_timer_chromeos.h))
is used to set a Real Time Clock (RTC) alarm. The RTC is a hardware timer that
runs even while the system is suspended and sends an interrupt to the boot CPU
to wake the system into Lucid Sleep.

## Freezing Renderers

If improperly handled, frequently waking up the system can consume a significant
amount of battery resources. In addition to conserving power by not resuming
power-hungry components like the display panel, while in Lucid Sleep Chrome
minimizes its usage of the CPU.

Chrome has a multi-process architecture (for more details see
[here](http://www.chromium.org/developers/design-documents/multi-process-architecture).)
All apps, extensions, and web pages run inside renderer processes, which
represent the biggest consumers of the CPU in the system. To minimize the power
they consume (for example by running the CPU at a high load), Chrome freezes
renderer processes at the initial suspend time and thaws them only after a full
resume.

The Linux kernel mechanism of control groups
([cgroups](https://www.kernel.org/doc/Documentation/cgroups/cgroups.txt)) is
used. Freezing processes in a cgroup works the same way as freezing userspace
processes at suspend time: the kernel simply removes the processes from the
scheduler’s run queue. Since Chrome freezes its renderer processes before the
system suspends, those processes remain frozen even after the system resumes and
are thawed only when Chrome explicitly thaws them. Processes stay frozen for the
duration of Lucid Sleep and CPU load is minimized.

### Properly handling the lock screen

Either users or an enterprise policy can set a Chrome preference that locks the
screen when the system suspends. Like most parts of the system user interface,
the lock screen runs inside a renderer process. Common side effects of freezing
the lock screen operation include a noticeable lag when the system fully resumes
and dropped keystrokes. In some cases, users may need to re-type their
passwords.

To address this, the lock screen renderer is given special treatment. If a user
requests locking the screen when the system suspends, Chrome puts the lock
screen renderer process in a separate cgroup from the other renderers. The lock
screen renderer remains unfrozen and responsive when the system fully resumes.

### Dealing with renderers that may receive push messages

Since all web applications as well as Chrome apps and extensions run inside a
renderer, a push message cannot be fully handled if the destination renderer has
no CPU cycles. So some renderers do need to remain active during Lucid Sleep.

Chrome decides whether or not to freeze a renderer process when the renderer is
first spawned. For example, for Chrome apps and extensions, Chrome looks at the
permissions that the app/extension has requested in its manifest. If it has
requested the “gcm” permission, Chrome keeps the renderer out of the group of
renderers to freeze at suspend time. This ensures that when a push message
arrives it can be handled as quickly as possible.

## Interacting with the GPU process

Chrome uses a dedicated process to interact with the GPU on the system. This
process is not frozen during Lucid Sleep and can continue to respond to requests
to render objects to the screen. Though the renderers themselves may be frozen
and unable to make requests to the GPU process, the main browser process remains
active and can issue updates.

This has implications for the display panel component, which is off during Lucid
Sleep. Any attempts by the GPU process to update the contents of the screen
result in errors, causing loss of context and forced reallocation of resources.
In addition to affecting GPU performance, repeatedly losing and allocating a
context (often multiple times per second) add a significant load to the CPU,
increasing the amount of power consumed during Lucid Sleep and reducing the
system’s overall battery life.

To avoid consuming unnecessary resources, Chrome stops its compositor from
updating the screen and prevents rendering requests from reaching the GPU
process. If the user has configured the screen to lock when the system suspends,
Chrome waits until the contents of the lock screen are visible before directing
the compositor to stop rendering. The delay allows the contents of the user’s
screen to remain invisible and secure when the system resumes.
