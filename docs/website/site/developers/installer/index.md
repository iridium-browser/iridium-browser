---
breadcrumbs:
- - /developers
  - For Developers
page_name: installer
title: A short trip through the Chromium installer's mind!
---

## Installed components

*   [Registry entries](/developers/installer#reg)
*   [Shortcuts](/developers/installer#shortcuts)

### Registry Entries

Syntax:

&lt;variable to be defined later&gt;

{for all things matching this definition}

\[RegistryValue\]

(Wow6432Node\\) -- Only on 64-bit versions of Windows.

Entry types:

*   *AppId entries* (Windows 8+ only):
    *   &lt;root&gt;\\Software\\Classes\\Chromium&lt;suffix&gt;
                ([AppUserModelId on
                MSDN](http://msdn.microsoft.com/library/windows/desktop/dd378459.aspx))
*   *ProgId entries*:
    *   &lt;root&gt;\\Software\\Classes\\ChromiumHTM&lt;suffix&gt;
                ([ProgId on
                MSDN](http://msdn.microsoft.com/en-us/library/aa911706.aspx))
*   *DelegateExecute COM registration entries* (Windows 8+ only):
    *   &lt;root&gt;\\Software\\Classes\\(Wow6432Node\\)CLSID\\{A2DF06F9-A21A-44A8-8A99-8B9C84F29160}
*   *App Registration entries*:
    *   &lt;root&gt;\\Software\\Microsoft\\Windows\\CurrentVersion\\App
                Paths\\chrome.exe (for ShellExecute to be able to find
                chrome.exe without needing to modify PATH;
                [MSDN](http://msdn.microsoft.com/library/windows/desktop/ee872121.aspx#appPaths)).
    *   &lt;root&gt;\\Software\\{file_assoc}\\OpenWithProgids\[ChromiumHTML&lt;suffix&gt;\]
                (to show up in "Open With" in the context menu for supported
                file types;
                [MSDN](http://msdn.microsoft.com/library/bb166549.aspx)).
*   *Shell Integration entries* (can only be registered in
            &lt;root&gt;==HKLM pre-Win8):
    *   &lt;root&gt;\\Software\\Clients\\StartMenuInternet\\Chromium&lt;suffix&gt;
                ([StartMenuInternet on
                MSDN](http://msdn.microsoft.com/library/windows/desktop/dd203067.aspx#internet))
        *   Capabilities (URL + File Associations) and other information
                    (put under this entry, but could theoretically be anywhere
                    in the registry;
                    [MSDN](http://msdn.microsoft.com/library/windows/desktop/cc144154.aspx#registration)).
    *   &lt;root&gt;\\Software\\RegisteredApplications\[Chromium&lt;suffix&gt;\]
                (to appear in "Default Programs" and have the ability to become
                default browser;
                [MSDN](http://msdn.microsoft.com/library/windows/desktop/cc144154.aspx#RegisteredApplications)).
*   *XP-style default browser entries*:
    *   &lt;root&gt;\\Software\\Classes\\{file_association |
                protocol_association}\[Default\] = ChromiumHTM&lt;suffix&gt;
        *   These are not necessary post-XP to be the default browser,
                    but many programs are still hardcoded to lookup these
                    values.
*   *Protocol specific entries*:
    *   &lt;root&gt;\\Software\\Client\\StartMenuInternet\\Chromium&lt;suffix&gt;\\Capabilities\\URLAssociations\[&lt;protocol&gt;\]
*   *Active Setup entries* (Google Chrome system-level installs only --
            [Active Setup design
            doc](https://docs.google.com/a/google.com/document/d/1yQdG1wMDKi_lf0i8bk6P2_BWqudRb-ap6Y2C5f1RO2w/edit)):
    *   HKEY_LOCAL_MACHINE\\Software\\(Wow6432Node\\)Microsoft\\Active
                Setup\\Installed
                Components\\{8A69D345-D564-463c-AFF1-A69D9E530F96}
*   *Uninstall entries*:
    *   &lt;root&gt;\\Software\\(Wow6432Node\\)Microsoft\\Windows\\CurrentVersion\\Uninstall\\Chromium
                (to show up in "Programs and Features" or "Add or Remove
                Programs" in the Control Panel)

Note: &lt;root&gt;\\Software\\Classes is also known as
[HKEY_CLASSES_ROOT](http://msdn.microsoft.com/library/windows/desktop/ms724475.aspx)

For *Google Chrome* replace the following values as indicated:

*   Chromium --&gt; Chrome (except for Protocol specific entries and
            Uninstall entries)
*   Chromium --&gt; Google Chrome (for Protocol specific entries and
            Uninstall entries)
*   ChromiumHTM --&gt; ChromeHTML
*   StartMenuInternet\\Chromium --&gt; StartMenuInternet\\Google Chrome
*   {A2DF06F9-A21A-44A8-8A99-8B9C84F29160} --&gt;
            {5C65F4B0-3651-4514-B207-D10CB699B14B}

#### System-level installs

*   &lt;suffix&gt; = ""
*   At Install:
    *   All entry types fully registered in &lt;root&gt; =
                HKEY_LOCAL_MACHINE
    *   If making Chromium default machine-wide:
        *   Register *XP-style default browser entries* in
                    &lt;root&gt;=HKEY_CURRENT_USER
*   + When making Chromium default for a user:
    *   Register *XP-style default browser entries* in
                &lt;root&gt;=HKEY_CURRENT_USER
    *   (on top of incanting Windows into making Chromium the default
                browser, but that doesn't require any further registrations
                post-XP).
*   + When making Chrome the default handler for some protocol:
    *   Register *protocol specific entries* with
                &lt;root&gt;=HKEY_LOCAL_MACHINE if not already registered
                (requires UAC)
    *   Register *XP-style default entries* for this protocol with
                &lt;root&gt;=HKEY_CURRENT_USER

#### User-level installs

*   &lt;suffix&gt; = .&lt;base32 encoding of the md5 hash of the user’s
            SID&gt; (which is always 26 characters (27 with the dot))
    *   Exception (for backward compatibility with old registration
                schemas):
        *   &lt;suffix&gt; = &lt;none&gt; if the user already has the
                    unsuffixed registration in HKLM from the old-style
                    registrations.
        *   &lt;suffix&gt; = .&lt;username&gt; if the user already has
                    the username suffixed registration in HKLM from the
                    old-style registrations.
        *   Note: *AppId Entries* are always suffixed with the new-style
                    suffixed (as they were introduced after the registration
                    schema changes).
*   At install:
    *   Install *AppId, ProgId, and App Registration entries* in
                &lt;root&gt;=HKEY_CURRENT_USER.
    *   On Windows 8+ also install *Shell Integration entries in*
                &lt;root&gt;=HKEY_CURRENT_USER.
    *   Before Windows 8, if |elevate_if_not_admin| (which is true when
                triggering the "make chromium default" flow):
        *   Elevate (UAC) and register all entry types in
                    &lt;root&gt;=HKEY_LOCAL_MACHINE.
*   When making Chromium default:
    *   If some registration are missing (i.e. *Shell Integration
                entries* before Windows 8 if the user has not yet elevated
                Chromium to complete registration):
        *   Elevate (UAC) and register all entry types in
                    &lt;root&gt;=HKEY_LOCAL_MACHINE.
    *   Register *XP-style default browser entries* in
                &lt;root&gt;=HKEY_CURRENT_USER
*   When making Chrome the default handler for some protocol:
    *   Register *protocol specific entries* with
                &lt;root&gt;=HKEY_LOCAL_MACHINE (HKEY_CURRENT_USER on Win8+) if
                not already registered (requires UAC before Win8)
    *   Register *XP-style default entries* for this protocol with
                &lt;root&gt;=HKEY_CURRENT_USER

### Shortcuts

#### User-level installs

*   At install:
    *   Create Start Menu shortcut (Start Screen on Win8)
        C:\\Users\\&lt;username&gt;\\AppData\\Roaming\\Microsoft\\Windows\\Start
        Menu\\Programs\\Chromium\\Chromium.lnk
    *   Pin shortcut to taskbar (Windows 7+)
        C:\\Users\\&lt;username&gt;\\AppData\\Roaming\\Microsoft\\Internet
        Explorer\\Quick Launch\\User Pinned\\TaskBar
    *   Create desktop shortcut
        C:\\Users\\&lt;username&gt;\\Desktop
    *   Create Quick Launch shortcut
        C:\\Users\\&lt;username&gt;\\AppData\\Roaming\\Microsoft\\Internet
        Explorer\\Quick Launch

#### System-level installs

*   At install:
    *   Create all-users Start Menu shortcut (in Win8 this will only
                show on the Start Screen for the user running the install (i.e.,
                not for new users); Microsoft claims that this is by design).
        C:\\ProgramData\\Microsoft\\Windows\\Start
        Menu\\Programs\\Chromium\\Chromium.lnk)
    *   Create all-users Desktop shortcut (shows on every user’s
                Desktop, but if any user deletes it, it’s gone for all...)
        C:\\Users\\Public\\Desktop\\Chromium.lnk
*   At install (if the installer is running in a given user's context;
            i.e., not for the MSI which runs as SYSTEM):
    *   Create per-user Quick Launch shortcut
        C:\\Users\\&lt;username&gt;\\AppData\\Roaming\\Microsoft\\Internet
        Explorer\\Quick Launch
    *   Pin shortcut to taskbar (WIndows 7+)
        C:\\Users\\&lt;username&gt;\\AppData\\Roaming\\Microsoft\\Internet
        Explorer\\Quick Launch\\User Pinned\\TaskBar
*   Upon the next login of each other user (when *Active Setup* triggers
            Chromium's *Active Setup* for that user and **only if** that user
            has yet to run Chrome on this machine):
    *   Create per-user shortcuts that do not have a matching all-users
                shortcut.
        *   Per-user Quick Launch shortcut.
        *   Pin shortcut to taskbar (Windows 7+).
        *   Per-user desktop shortcut (if the all-users Desktop shortcut
                    is absent).
        *   Per-user Start Menu shortcut (if the all-users Start Menu
                    shortcut is absent).

**Master Preferences**

*   do_not_create_desktop_shortcut: Prevents creation of the Desktop
            shortcut in all scenarios above (including Active Setup).
*   do_not_create_quick_launch_shortcut: Prevents creation of the Quick
            Launch shortcut (Windows XP only) in all scenarios above (including
            Active Setup).
*   do_not_create_taskbar_shortcut: Prevents creation of the taskbar pin
            (Windows 7+) in all scenarios above (including Active Setup).
*   do_not_create_any_shortcuts: Supersedes any other preference and
            prevents creation of all shortcuts in all scenarios above (including
            Active Setup).

For *Google Chrome* replace *Chromium* by *Google Chrome* above.
