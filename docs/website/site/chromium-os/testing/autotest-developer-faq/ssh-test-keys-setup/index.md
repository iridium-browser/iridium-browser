---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/autotest-developer-faq
  - Autotest Developer FAQ
page_name: ssh-test-keys-setup
title: Setting up SSH Access to your test device.
---

## In order to run the automated tests against your device you need to ensure it is running a Test Image and you have password-less SSH Access.

1. First make sure you have a Chrome OS Device with a Test Image installed.

*   If you are a Google partner, please contact your Google
            representative for Google Storage access to our builds for for
            automated re-imaging of your test devices via the devserver
            (described below).
*   Otherwise:
    *   Download a test image from the public waterfall
                <http://build.chromium.org/p/chromiumos/waterfall>
    *   Or build a test image.
        *   <http://www.chromium.org/chromium-os/developer-guide#TOC-Building-Chromium-OS>
    *   Once you have a test image, install it on your test device.
        *   <http://www.chromium.org/chromium-os/developer-guide#TOC-Installing-Chromium-OS-on-your-Device>

2. Next, install the RSA keys to allow you SSH into the device.

*   Download the testing RSA keys into ~/.ssh/ (on your test server).
    *   You can get the key from the chromiumos checkout under
                ***chromiumos/src/scripts/mod_for_test_scripts/ssh_keys/testing_rsa***
    *   Or download from
                <https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/ssh_keys>
*   Restrict the permissions on ~/.ssh/testing_rsa; SSH will ignore the
            file if the permissions are too open:

```none
$ chmod 0600 ~/.ssh/testing_rsa
```

*   Add an entry for your device in your ~/.ssh/config:

```none
Host device
  HostName 172.22.168.233 # The IP address of the Chrome OS Device.
  CheckHostIP no
  StrictHostKeyChecking no
  IdentityFile %d/.ssh/testing_rsa
  Protocol 2
  User root
```

*   Once you have done both steps verify you have password less login as
            root:

```none
$ ssh device
Warning: Permanently added '172.22.168.233' (RSA) to the list of known hosts.
Last login: Mon Mar 17 14:21:30 PDT 2014 from 172.18.72.8 on ssh
localhost ~ # 
```

*   If connecting to multiple test devices, you can share common config
            options in your ~/.ssh/config:

```none
Host 172.22.168.* # The subnet containing your Chrome OS test devices.
  CheckHostIP no
  StrictHostKeyChecking no
  IdentityFile %d/.ssh/testing_rsa
  Protocol 2
  User root
Host device1
  HostName 172.22.168.233 # The IP address of the first Chrome OS Device.
Host device2
  HostName 172.22.168.234 # The IP address of the second Chrome OS Device.
```

**Note:** you can also reach your device via ssh if using a Dev image, but you
will need to enable the debugging features on the setup screen after a flash or
powerwash.