---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: add-a-new-package
title: Adding a New Package
---

Here's how you add a new package to Chromium OS:

1.  Create an ebuild file for your package
            `src/third_party/chromiumos-overlay/`. See the ebuild(5) man page
            (from within your development chroot) and the other ebuilds in
            `src/third_party/portage` and `src/third_party/chromiumos-overlay`
    *   for example, the ebuild file of systemtap is placed in
                `src/third_party/chromiumos-overlay/dev-util/systemtap`.
2.  Upload the package to Localmirror of Google (require Google
            account).
3.  Create a **README.chromiumos file** that describes the purpose of
            the package. If your package is a third-party package, document any
            custom patches that have been applied
4.  Add a dependency to your new package in one of:
    *   `src/third_party/chromiumos-overlay/virtual/target-chromium-os`
        *   Includes the package in all images (base/release, developer,
                    test)
    *   `src/third_party/chromiumos-overlay/virtual/target-chromium-os-dev`
        *   Includes the package in developer and test images
    *   `src/third_party/chromiumos-overlay/virtual/target-chromium-os-test`
        *   Includes the package in test images
    *   `src/third_party/chromiumos-overlay/virtual/target-chromium-os-sdk`
        *   Includes the package in the SDK itself
5.  Increment the revision number of the ebuild that you modified so
            that incremental builds recognize the changes.

The source code of the package shouldn't be placed in the `src/`. emerge will
download the package and store it in `/var/lib/portage/distfiles-target/`, and
the code is compiled in `/build/tegra2_seaboard/tmp/portage/`. When porting a
new package, we can put the package in `/var/lib/portage/distfiles-target/`.

Once you're done, make sure your changes to do not break the build system. Then
create a changelist and get it reviewed (see [Contributing
Code](/developers/contributing-code) for details).

### Adding a package to a running Chromium OS system

The easiest way to add a new package, or update a package is to start
`devserver` on the build host, and use `gmerge` to build the package and install
the results on the target device.

See [Using the dev
server](http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/using-the-dev-server#TOC-How-to-build-a-single-package-and-i).

### Adding a package to the manifest

See the [Git server-side
information](http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/git-server-side-information#TOC-How-do-I-add-my-project-to-the-mani)
site for information about how to add your package to a manifest.
