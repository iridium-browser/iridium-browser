---
breadcrumbs:
- - /developers
  - For Developers
page_name: linux-technical-faq
title: Linux Technical FAQ
---

****Q**. Why doesn't Chromium do *\[insert feature X\]* better?**

**A. Frequently, the answer is that just that we haven't had time to implement
it. If you're so inspired, [come learn how to contribute to the
project](/developers).**

**Q**. Why does Chromium warn me that downloading a .exe is dangerous? I'm not
on Windows!

**A**. When you click on the downloaded file from within Chromium, we open it
with the default viewer. On some systems the default viewer for .exe files is
Wine, which means a malicious .exe can be as dangerous as on Windows.

### System Integration

**Q**. Why doesn't Chromium integrate with fontconfig/GTK/Pango/X/etc. better?

**A**. Frequently, the answer is due to our
[sandbox](/developers/design-documents/sandbox/Sandbox-FAQ) (Windows-specific
doc but the concepts apply to all platforms), which attempts to prevent web
content from having any access to your system. Features that other browsers get
for free often take extra implementation effort. For example, since the entire
web content area is rendered as a bitmap in a separate process that doesn't have
access to the filesystem, we can't draw native widgets in the renderer (which
are blobs of binary code and image files). See the answer to the topmost
question on this page.

**Q**. How do I control what program Chromium uses to open downloads?

**A**. Rather than have its own set of settings separate from your system,
Chromium calls xdg-open to open files. xdg-open is supposed to handle the shared
subset of settings of Gnome/KDE/XFCE, with a fallback for users who don't use a
desktop environment. Consult your favorite search engine for information on how
to configure xdg-open.

**Q**. Why doesn't Chromium put its tabs into the title bar on my window
manager? (Or: why don't Chromium's minimize/maximize/close buttons theme
properly?)

**A**. The way X works is that the "title bar" of the app is drawn by a
completely separate program (the window manager) than the app itself, without
any API between the application and the window manager for doing these sorts of
tricky behaviors. This leaves us with two options: either let the window manager
draw the ordinary title bar (in which case our tabs don't extend up into it) or
we tell the window manager to not draw a title bar at all and we draw the entire
title bar ourselves (which means we have to just make a guess at how your window
manager looks). We attempt to guess which of these two behaviors to use at
startup; you can toggle between these modes on the second tab of the options
window.

### Google Chrome

**Q**. What is the difference between Chromium and Google Chrome?

**A**. See [our separate page about
this](http://code.google.com/p/chromium/wiki/ChromiumBrowserVsGoogleChrome).

**Q**. Why do the Google Chrome packages automatically add the Google Chrome
repository and repository key to my distribution's package manager?

**A**. We wanted installing the packages and getting updates to require a single
click: download the package, click OK, and it's installed. Requiring users to
add an extra sources line, update the package list, then find the package in the
updated list is too much effort. If you don't want this behavior, Create an
empty /etc/default/google-chrome before installing. You can also add the line:
repo_add_once=false to /etc/default/google-chrome to achieve the same effect.

**Q**. Why does the Google Chrome package install a cron job on RPM systems?
(i.e. Fedora, OpenSUSE, Mandriva)

****A**. On some RPM-based distros, the RPM / repo database is locked while the
installer is running. As a result, we cannot add the repository and/or the
repository key during the install. Instead, a cron job does this at a later
time. See previous entry for instructions on how to disable this cron job.**

**Q**. Why does the Google Chrome package install a cron job on Debian systems?
(i.e. Debian, Ubuntu)
**A**. We found some Debian-based distros will remove the Google Chrome source
when doing an upgrade, meaning users will silently stop getting updates. All the
cron job does is verify the above sources line still exists; it doesn't do any
updating itself. Again, /etc/default/google-chrome controls this behavior. You
can either create an empty /etc/default/google-chrome or explicitly add the
line: repo_reenable_on_distupgrade=false.
