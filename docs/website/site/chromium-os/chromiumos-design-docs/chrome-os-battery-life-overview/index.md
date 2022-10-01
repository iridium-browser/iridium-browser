---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: chrome-os-battery-life-overview
title: Chrome OS Battery Life Overview
---

We design Chromium OS with the philosophy that devices provide users “all-day
battery life.” Because the notion of all-day battery life can be subject to
interpretation, we use the philosophy to define a usage model for
battery-powered devices. This model helps us define user expectations, and
provide battery recommendations for Chromium OS devices.

The Battery Usage Model

We don’t have a say in how users use their device. Users can and will use their
devices in diverse situations, with variable access to charging power. Moreover,
as devices and their uses change, the power requirements for systems evolve. As
such, Chromium OS provides tests and diagnostics which allow both users and
device manufacturers to ensure that they can achieve our notion of all-day
battery life.

Active use and standby-time are different. While some devices, notably mobile
phones, blend battery life usage between active and standby states, we believe
these states are separate, important elements of all-day battery life. We define
active use as situations in which a user is “working away from the plug.” We
define standby time as situations in which the user is “idle away from the
plug.” We expect a user’s typical day to include periods of both, but that the
blend of states may vary day to day.

Typical active-use should leave reasonable standby available. Importantly,
adhering to our notion of all-day battery life means that a user should be able
to actively work for many hours and still have sufficient standby time to reach
a power source and recharge the device. Our belief is that standby power
consumption should be at least an order of magnitude less that active-use power
consumption. Thus, if a device has been used for 80% of active-use time, the
remaining 20% should translate into a substantial amount of standby time in
which the device can be recharged without forced shutdown.

Shutdown provides longevity beyond typical use. While a device is shutdown, it
can conserve most of its battery life. While our notion of all-day battery life
does not assume shutdowns, we believe that the fast-booting nature of Chromium
OS should make it easy for users to strongly preserve battery life when they
desire.

A Typical Use Case

Our philosophy, and its derived battery usage model, is well illustrated by an
example. Consider a typical Chromium OS user, whose device achieves the average
10 hours of active-use battery life. Our usage model expects that this user:

    Can use their device in active mode for an 8-hour workday, and have 20% of
    battery remaining.

    The remaining 20% of battery life should translate into more than a day of
    standby time; roughly 30 hours.

Battery Recommendations

Given the philosophy and usage model, Chromium OS recommends that device
batteries be sized such that an average of 10 active hours can be achieved on
the device before recharging. Thus, if a device consumes 4 Watts for each hour
of active time, a 40 Watt-Hour battery is recommended.
