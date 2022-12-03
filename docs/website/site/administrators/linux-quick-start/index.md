---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: linux-quick-start
title: Linux Quick Start
---

This page describes the steps to get a managed instance of Google Chrome or
Chromium up and running on Linux.

## Download / Build the Browser

Depending on your network's requirements, you may either want to deploy Chromium
or Google Chrome. The differences are described
[here](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/chromium_browser_vs_google_chrome.md).

There are several different ways to get the browser:

*   If you want to deploy Google Chrome, download Google Chrome
            [here](https://www.google.com/chrome).
*   If you want to deploy Chromium, your distro may have already
            repackaged Chromium for you. See which distros have repackaged
            Chromium
            [here](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/linux/chromium_packages.md).
*   If you want to deploy Chromium but you want to build it yourself,
            follow the instructions on building Chromium
            [here](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/linux/build_instructions.md).

At the end of this process, you should have Google Chrome or Chromium installed.

## Set Up Policies

The location of the policy configuration files depends on whether you are
running Google Chrome or Chromium:

* Google Chrome looks for policies installed in **/etc/opt/chrome/policies**
* Chromium looks for policies installed in **/etc/chromium/policies** by default
  * **Note:** if you use a Chromium package from a Linux distribution, please
    make sure you check its documentation. Ubuntu's package checks for policies
    in **/etc/chromium-browser/policies** instead, for example.

There are two sets of policies kept in these directories: one set that is
required and mandated by an administrator, and one set that is recommended for
users but not required. For Google Chrome, these two sets live at (remember
that the paths differ for Chromium):

* /etc/opt/chrome/policies/managed/
* /etc/opt/chrome/policies/recommended/

Create these directories if they do not already exist:

```
mkdir /etc/opt/chrome/policies
mkdir /etc/opt/chrome/policies/managed
mkdir /etc/opt/chrome/policies/recommended
```

Make sure that the files under /managed are not writable by non-admin users;
otherwise, they could just overwrite your policies to get the configuration they
want!

```
chmod -w /etc/opt/chrome/policies/managed
```

To set policies that are required, create a file named "test_policy.json" in
/etc/opt/chrome/policies/managed/

```
touch /etc/opt/chrome/policies/managed/test_policy.json
```

In this file, put the following content:

```
{
  "ShowHomeButton": true
}
```

That's it! The next time you start Google Chrome on that machine, the home
button next to the location bar (which is not shown by default) will be shown
and locked to this value.

To see what other policies you can control, review the [exhaustive list of all
manageable policies](https://chromeenterprise.google/policies/).

You can spread your policies over multiple JSON files. Chrome will read and
apply them all. However, you should not be setting the **same** policy in more
than one file. If you do, it is **undefined** which of the values you specified
prevails.

**Make sure that policy JSON files under ../managed/ are not writable by just
anyone! Google Chrome / Chromium gives these files higher priority, so they
should only be writable by an administrator or root!**

## Push out the Policies and Browser

Using whatever mechanism you use to push files to clients, whether it be a
utility or just a script file, push the "test_policy.json" file out to the
target machines in your network. Make sure that this file exists at
/etc/opt/chrome/policies/managed/ on all the target machines. You could do this
simply by scp'ing the files to the target:

```
scp -r /etc/opt/chrome/policies adminusername@targetmachine:/etc/opt/chrome
```

Similarly, use whatever file-pushing utility or script to push out Google Chrome
/ Chromium. Whenever a user on those target machines runs Google Chrome /
Chromium, it will obey the policy files you copied onto those machines.
