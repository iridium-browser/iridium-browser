---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
- - /chromium-os/how-tos-and-troubleshooting/building-chromium-browser
  - Building Chromium for Chromium OS (simple chrome)
page_name: chrome-api-keys-in-the-chroot
title: Chrome API keys in the Chromium OS SDK chroot
---

When building Chromium on Chromium OS, there are (at least) 3 environments for
building that are relevant.

1.  The host home directory.
2.  The home directory in the chroot.
3.  The home directory while building the chromeos-base/chromeos-chrome
            package.

The mechanism for compiling API keys into the Chromium binary itself under
Chromium OS uses copy-propagation between the layers.

### Between layers 1 & 2 (host & chroot)

The enter chroot functionality takes care of propagating the keys. If the file
`.googleapikeys` is not present in the chroot, or is empty, the keys are
searched for in your home directory, extracted and written there. It is a
line-oriented file that should look like the sample in the chrome page.

<pre><code>'google_api_key': '<b>ABC123</b>',
'google_default_client_id':     '<b>123/abc</b>',
'google_default_client_secret': '<b>floor-sweeper</b>',
</code></pre>

To reset these the easiest way to update the chroot is simply:

```none
cros_sdk -- rm ../../../.googleapikeys
```

and then the next time something enters the chroot, your native credentials will
be copied in. If the keys are not working, try taking a look at the file to see
if the format of your keys outside the chroot confused the propagation.

If you want your chroot to build something non-default or non-shared, you can
enter the chroot and change the `.googleapikeys` directly.

### Between layers 2 & 3 (chroot and package)

The chromeos-base/chromeos-chrome ebuild takes care of respecting the keys in
the chroots `.googleapikeys` in the final source build environment. It does this
by expanding and converting the file into an include.gypi in the inside
environment that does chromium's make.
