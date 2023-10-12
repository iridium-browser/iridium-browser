---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: mac-quick-start
title: Mac Quick Start
---

This page describes how to get started with managing Google Chrome on a Mac
network using Apple’s Profile Manager. These instructions are only an example to
help you get started. You can use any system management software you choose, but
specific steps may vary.

Note: These steps focus only on pushing out a plist configuration file for the
Chrome Browser. Your Mac network must be set up and in a managed state. This
guide does not cover how to setup macOS Server or Profile Manager to manage your
Mac devices.

1.  Log into Profile Manager on the server machine from a web browser
2.  Navigate to **Devices** &gt; **Settings**, choose the device group
            for which to define Chrome policies.
3.  Click **Edit**.
4.  Go to **Custom Settings** and add a Preference Domain with domain
            '*com.google.Chrome'*
5.  Upload a pre-configured plist file or manually add items by key and
            value. A full list and description of policies can be found at
            <https://www.chromium.org/administrators/policy-list-3>.
6.  Click **Ok** and payload will be created and synced to the selected
            devices.

**Debugging**

If you have trouble, it usually pays off to examine whether the settings are
correctly stored and read by Chrome. First of all, navigate to about:policy in
Chrome. It lists any policy settings that Chrome has picked up. If your settings
show up, good. If not, you can dig deeper and check whether macOS actually put
them into place correctly. Mandatory policy is stored in /Library/Managed
Preferences/&lt;username&gt;/com.google.Chrome.plist while recommended policy is
stored in /Library/Preferences/com.google.Chrome.plist. The plutil command can
be used from a terminal to convert it to XML format:

```
# sudo -s
# cd /Library/Managed Preferences/<username>;
# plutil -convert xml1 com.google.Chrome.plist
# cat com.google.Chrome.plist
```

For debugging, this file is in a place where it can be edited manually. Chrome
will pick up the updated preferences automatically. Note that this is not
recommended for making persistent changes to policy, since macOS will rewrite
the file with settings configured through Workgroup Manager.

Recent versions of macOS may cache values from these files and so you may need
to run `defaults read "/Library/Managed Preferences/`... before changes will
be noticed. This can also apply if a plist file is deleted, in which case
that same command is needed for macOS to notice the deletion.

Chrome on macOS does not show unknown policies on the chrome://policy page. If
you don't find a policy you have set there check if the name is spelled
correctly and if the policy is actually supported on the macOS platform.

**Notes for Chromium**

Chromium can be managed in a similar way, with a few differences:

*   The Chromium application must be managed, instead of Google Chrome
*   Chromium is identified as "org.chromium.Chromium"
*   The plist file is in the same place but is named
            "org.chromium.Chromium.plist"

**Unknown issues**

Please file a bug report at <http://new.crbug.com> and select the "Enterprise
Issue" template.
