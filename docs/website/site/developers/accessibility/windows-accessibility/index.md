---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
page_name: windows-accessibility
title: Windows accessibility
---

Microsoft Windows is the most widely used desktop operating system in the world,
and while there are some basic built-in features for users with disabilities,
there's a much larger and richer ecosystem of third-party products that provide
alternative ways to interact with Windows.

### Keyboard accessibility

Windows has a tradition of being fully keyboard-accessible without any extra
configuration needed, and Chromium should be no exception. There shouldn't be
anything you can do with a mouse that you can't also do with the keyboard. Where
possible, Chromium should use standard discoverable keyboard shortcuts, but
where that's impossible or difficult, custom keyboard shortcuts are fine.

Most users are familiar with using Tab to cycle between focusable controls. Here
are some other keys that are widely supported in Windows applications (and
should be working great in Chromium, too):

*   F6 moves focus between various focusable panes. In Chromium, F6
            provides access to the toolbar, bookmarks bar, infobars, and more.
*   Shift+F10 (or the Context Menu key on some keyboards) opens a
            context menu.
*   Space and Enter should press buttons and links. (In some dialogs,
            Enter presses the default button rather than the current button.)
*   F10 focuses the menu bar. In Chromium, it focuses the Chromium menu
            in the toolbar (but does not open it). You can also press and
            release Alt.
*   Alt+shift+&lt;letter&gt; places focus directly on a container (i.e.
            alt-shift-t places focus on the first item of the toolbar).

If you're adding new UI features, first see if you can make them discoverable -
put them in the tab order for an existing pane. If that's impossible, make sure
they have their own keyboard shortcut. Alternatively, for features containing
menu items, you can let users learn about the shortcut from their exploration of
the main Chromium menu.

### High Contrast Mode

Windows has a special high contrast mode intended for people with low vision.
The most common variation is white-on-black text instead of black on white.
While Mac OS X and Linux provide this at the OS compositor layer (so apps don't
have to do anything), Windows implements this by changing system colors and
expects apps to use these colors.

To try High Contrast mode, press Alt+Shift+PrintScreen. (Note: must be the left
alt and left shift key.)

All UI drawn by Chromium that's outside of the webpage should either be 100%
themable, or it should respect the system colors. So when choosing colors for
text and backgrounds in particular, be sure to use system colors, not hardcoded
colors. See GetSysSkColor in ui/gfx/color_utils.h as a starting point.

On Windows, users who have high-contrast mode enabled will see a bubble the
first time they open Chrome, prompting them to install the [High
Contrast](https://chrome.google.com/webstore/detail/high-contrast/djcfdncoelnlbldjfhinnjlhdjlikmph)
extension and a white-on-black theme from the web store (both optional). The
High Contrast extension improves the readability of web content, and the themes
improve the look of the rest of Chrome.

### Accessibility API support

[Microsoft Active Accessibility
(MSAA)](http://msdn.microsoft.com/en-us/library/ms971310.aspx) is a COM
interface that Windows programs can implement in order to allow access to their
interface from assistive technology such as screen readers, magnifiers,
voice-control applications, and more. Chromium has good support for MSAA now and
it's important to keep it that way! When writing native Windows UI code, think
about whether you might need to expose any additional information to make it
accessible.

To do a quick accessibility check, download
[AViewer](http://www.paciellogroup.com/blog/2011/06/aviewer-beta-updated/) and
press F6 to track focus. Then switch to Chromium and use the keyboard to
navigate any area of the UI you want to test. Then check the Role, Name, Value,
and other properties in AViewer and make sure they're correct.

Note: Chrome doesn't enable accessibility for the main web area by default
unless it detects a screen reader or other advanced assistive technology. To
override this, run chrome.exe with the --force-renderer-accessibility flag.

To do a more involved check, learn to use [NVDA](http://www.nvda-project.org/),
an open-source screen reader. Google employees also have access to site licenses
for some of the most popular commercial tools, including JAWS and ZoomText.

Most of the time you probably won't be writing native Windows code, you'll be
writing Views code. Views has its own accessibility layer, which should make
things very easy - in most cases it will take only 2 or 3 lines of additional
code to make a new control accessible. See [Views
Accessibility](/developers/accessibility/views-accessibility) for more
information.

If you are writing native Windows code, you may be able to update the
accessibility information for a particular widget using [dynamic
annotation](http://msdn.microsoft.com/en-us/windows/cc307286) - that's the
easiest. Here's an example that sets the accessible name of **hwnd** to
**name**:

base::win::ScopedComPtr&lt;IAccPropServices&gt; acc_prop_services;

HRESULT hr = CoCreateInstance(CLSID_AccPropServices, NULL, CLSCTX_SERVER,

IID_IAccPropServices, reinterpret_cast&lt;void\*\*&gt;(&acc_prop_services));

hr = acc_prop_services-&gt;SetHwndPropStr(hwnd, OBJID_CLIENT,

CHILDID_SELF, PROPID_ACC_NAME, name.c_str());

If all else fails, you'll have to implement a custom IAccessible interface for
your widget. Hopefully not! So far Chrome doesn't have any examples of this,
everything is either handled by the Views accessibility code in
ui/views/accessibility, the web content accessibility code in
content/browser/accessibility, or a few tiny annotations like the example above.
