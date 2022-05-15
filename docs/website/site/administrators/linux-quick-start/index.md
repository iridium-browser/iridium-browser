---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: linux-quick-start
title: Linux Quick Start
---

This page describes the steps to get a managed instance of Google Chrome or
Chromium up and running on Linux.

**Download / Build the Browser**

Depending on your network's requirements, you may either want to deploy Chromium
or Google Chrome. The differences are described
[here](http://code.google.com/p/chromium/wiki/ChromiumBrowserVsGoogleChrome).

There are several different ways to get the browser:

*   If you want to deploy Google Chrome, download Google Chrome
            [here](http://www.google.com/chrome/eula.html?platform=linux&hl=en&hl=en).
*   If you want to deploy Chromium, your distro may have already
            repackaged Chromium for you. See which distros have repackaged
            Chromium
            [here](http://code.google.com/p/chromium/wiki/LinuxChromiumPackages).
*   If you want to deploy Chromium but you want to build it yourself,
            follow the instructions on building Chromium
            [here](http://code.google.com/p/chromium/wiki/LinuxBuildInstructions).

At the end of this process, you should have Google Chrome or Chromium installed.
Verify that the version you are running is **later than 6.0.444.0**.

**Set Up Policies**

Policy configuration files live under **/etc/chromium** for Chromium, and under
**/etc/opt/chrome** for Google Chrome (note the lack of **opt** for Chromium).
There are two sets of policies kept in these directories: one set that is
required and mandated by an administrator, and one set that is recommended for
users but not required. These two sets live at:

/etc/opt/chrome/policies/managed/

/etc/opt/chrome/policies/recommended/

Create these directories if they do not already exist:

```none
>mkdir /etc/opt/chrome/policies
>mkdir /etc/opt/chrome/policies/managed
>mkdir /etc/opt/chrome/policies/recommended
```

Make sure that the files under /managed are not writable by non-admin users;
otherwise, they could just overwrite your policies to get the configuration they
want!

```none
>chmod -w /etc/opt/chrome/policies/managed
```

To set policies that are required, create a file named "test_policy.json" in
/etc/opt/chrome/policies/managed/

```none
>touch /etc/opt/chrome/policies/managed/test_policy.json
```

In this file, put the following content:

```none
{
  "HomepageLocation": "www.chromium.org"
}
```

That's it! The next time you start Google Chrome on that machine, the home page
will be locked to this value.

To see what other policies you can control, review the [exhaustive list of all
manageable policies](/administrators/policy-list-3).

You can spread your policies over multiple JSON files. Chrome will read and
apply them all. However, you should not be setting the **same** policy in more
than one file. If you do, it is **undefined** which of the values you specified
prevails.

**Make sure that policy JSON files under ../managed/ are not writable by just
anyone! Google Chrome / Chromium gives these files higher priority, so they
should only be writable by an administrator or root!**

**Push out the Policies and Browser**

Using whatever mechanism you use to push files to clients, whether it be a
utility or just a script file, push the "test_policy.json" file out to the
target machines in your network. Make sure that this file exists at
/etc/opt/chrome/policies/managed/ on all the target machines. You could do this
simply by scp'ing the files to the target:

```none
>scp /etc/opt/chrome/policies adminusername@targetmachine:/etc/opt/chrome
```

Similarly, use whatever file-pushing utility or script to push out Google Chrome
/ Chromium. Whenever a user on those target machines runs Google Chrome /
Chromium, it will obey the policy files you copied onto those machines.