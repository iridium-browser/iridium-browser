---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: chromium-os-cgroups
title: Chromium OS Cgroups
---

All cgroups are optional, but highly-recommended and if present will be used by
Chrome and Chrome OS

top-level cgroups created by /etc/init/cgroups.conf

*   cpu - Uses CPU time accounting to set limits on certain tasks
        *   chrome_renderers - Set by Chrome code in &lt;fixme&gt;
        *   session_manager_container - set by session manager in
                    &lt;fixme&gt;
        *   update-engine - set by update-engine in &lt;fixme&gt;
*   freezer
    *   group of threads which stop running when we suspend or resume
    *   this limits the amount of work that happens at resume
    *   done by &lt;fixme&gt;
*   devices
    *   limits the devices available to a process
    *   done by &lt;fixme&gt;
*   cpuacct
    *   keeps track of resources consumed by a given group

There are also cgroups which are board-specific.

*   cpuset

For big.LITTLE systems with different types of CPU cores, we set up cpusets for
background tasks called cpuset/chrome/non-urgent/ which will be pinned to little
CPUs and another for performance-sensitive tasks called cpuset/chrome/urgent/

It's up to Chrome to put the correct tasks (threads) into the {non-,}urgent
group as appropriate.
