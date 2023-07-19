---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/aura
  - Aura
page_name: multi-desktop
title: Multi-desktop
---

Aura now makes it possible for the same browser process to render to multiple
desktops simultaneously (e.g., Native Desktop and Metro Desktop on Windows 8).

To enable this, the Chromium codebase had to be made multi-desktop aware (i.e.,
chrome::HostDesktopType parameters were added in several locations).

So... a bunch of methods now take chrome::HostDesktopType, but **how do you get
the right HostDesktopType when you need to provide one?**

There are many ways:

1.  If you have a Browser: take its host_desktop_type() member.
2.  If you have a gfx::NativeView: use
            chrome::GetHostDesktopTypeForNativeView().
3.  If you have a gfx::NativeWindow: use
            chrome::GetHostDesktopTypeForNativeWindow().
4.  If you have a WebContents\* w: you can use (2) passing in
            w-&gt;GetNativeView() OR you might be able to get a Browser with
            chrome::FindBrowserWithWebContents() and use (1)
5.  If all else fails and you have absolutely NO way of getting desktop
            context (e.g., a background extension that wants to open a new
            window...): you can use chrome::GetActiveDesktop() to get the type
            of the last user-activated Chrome desktop, but be aware that this is
            inherently racy (i.e., the user can switch desktops at any time) --
            If you are in a test, it is OK to use GetActiveDesktop() since
            almost all tests run on a single desktop making GetActiveDesktop()
            constant.
6.  You should almost never hardcode HOST_DESKTOP_TYPE_X constants.
7.  If you work on Chrome OS and your code is under ash/, please use
            HOST_DESKTOP_TYPE_ASH (this is the same as HOST_DESKTOP_TYPE_NATIVE
            on Chrome OS, but not on other platforms).

Most of the methods mentioned above are found in
src/chrome/browser/ui/host_desktop.h.

Note: If you hardcode HOST_DESKTOP_TYPE_NATIVE it will *look* like it's
*working* if you're in a single-desktop environment (which most of you are), but
what it will really do is: one day someone will trigger your feature from the
Ash desktop (e.g., in Windows 8 Metro) and it will open a window/tab on the
native desktop (invisible to the user who is immersed in the Ash environment)...
