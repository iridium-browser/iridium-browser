---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: tab-discarding-and-reloading
title: Tab Discarding and Reloading
---

**Why are my tabs reloading?**

Your device is out of memory. Like your Android phone or tablet, Chrome is
silently closing background tabs in order to make memory available. When you
click on one of those tabs it reloads.

**What's taking up all my memory?**

Open the Task manager (under Tools &gt; Tools &gt; Task manager, or hit
Shift-Esc) to see the usage of tabs and extensions. about:memory has more
detailed information.

**How can I make it stop?**

Close some tabs or uninstall extensions that take a lot of memory. If there's a
specific tab you don't want discarded, right-click on the tab and pin it.

**How does Chrome choose which tab to discard?**

You can go to about:discards to see the current ranking of your tabs. It
discards in this order:

1.  Internal pages like new tab page, bookmarks, etc.
2.  Tabs selected a long time ago
3.  Tabs selected recently
4.  Tabs playing audio
5.  Apps running in a window
6.  Pinned tabs
7.  The selected tab

**How often are users affected by this?**

The majority of Chrome OS users have only one or two tabs open. Even users with
more tabs open rarely run out of memory. Some users who run out of memory never
see a reload (they log out without ever looking at a discarded tab). We have
internal UMA metrics on all these conditions. Googlers tend to have large
numbers of tabs open to complex web sites and tend to hit reloads more often.

**Why doesn't Chrome do this on Mac / Windows / Goobuntu?**

Those machines swap memory out to disk when they get low on resources. Changing
tabs then slows down as the data is loaded from disk.

**Why doesn't Chrome OS use swap?**

This was a very early design decision. My understanding of the rationale is:

\* Chrome OS doesn't want to wear out the SSD / flash chips by constantly
writing to them.

\* To preserve device security it would have to encrypt the swap, which
increases the CPU and battery usage.

\* Rather than slowing down by swapping, Chrome OS should run fast until it hits
a wall, then discard data and keep running fast.

**Why doesn't Chrome OS tell me that I'm out of memory?**

This is an intentional UI design decision. My understanding of the rationale is:

\* Phones and tablets silently discard pages

\* Users shouldn't have to worry about managing memory

**Why do we discard tabs instead of doing something else?**

Well, we used to do something worse. When the machine ran out of memory the
kernel out-of-memory killer would kill a renderer, which would kill a
semi-random set of tabs. Sometimes this included the selected tab, so the user
would see a sad tab page (specifically, He's dead, Jim.). The tab discarder only
drops one tab at a time and tries to be smart about what it discards. We tried
other approaches without much success. For example, forcing JavaScript garbage
collection is too slow. Dropping various graphics caches only frees memory for a
short amount of time. Part of the difficulty is that the response to low memory
conditions needs to be fast, reliable and lead to a persistent decrease in
memory consumption.

**Is there a long-term plan for this issue?**

Sometimes these issues are caused by memory leaks or bloat, which we fix.
davemoore@ has been particularly good at finding Chrome OS memory problems. We
also need to systematically reduce the amount of memory Chrome uses. People
across the browser, renderer and graphics teams are working on this. We're also
investigating zram (a kind of in-memory swap). At some point we may want to
revisit the design decisions above.

**Where can I find more detailed information about what we do in low memory
situations?**

See the [out of memory design
document](/chromium-os/chromiumos-design-docs/out-of-memory-handling).
