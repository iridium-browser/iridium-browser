---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: add-sdk-package
title: Adding a Package to the SDK
---

[TOC]

## Add the Package

When adding a package to the SDK, for third-party packages not yet in the source
tree, start by following the
[New & Upgrade Package Process](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/portage/package_upgrade_process.md)
guide (short version: use `cros_portage_upgrade` to pull the package from
upstream). For new cros-workon packages, see
[Adding a New Package](/chromium-os/how-tos-and-troubleshooting/add-a-new-package).
Once the package is in place, add it as a dependency to the
[virtual/target-chromium-os-sdk](https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay/+/HEAD/virtual/target-chromium-os-sdk/target-chromium-os-sdk-9999.ebuild)
ebuild to be automatically installed as part of the SDK.

Do note that because virtual/target-chromium-os-sdk is a cros-workon package,
before adding the dependency, run:

```
cros-workon --host start virtual/target-chromium-os-sdk
```

## Test the Package

To install the new package to test against (e.g. to run commands/tools, check
behavior, etc.):

```
sudo emerge <package name>
```

To test the new package is installed correctly as part of the SDK, update the
chroot:

```
update_chroot
```

### Unit testing

To run the unit tests for the new package, run:

```
cros_run_unit_tests --host --packages <package name>
```

For cros-workon packages, as long as the package is in the
virtual/target-chromium-os-sdk dependency tree, the unit tests will
automatically be run on the SDK as part of the host-packages-cq builder in CQ
and are required to pass. For third-party packages, unit tests can be run with
the `cros_run_unit_tests` command above to get feedback locally, but they are
not run as part of CQ and are not required to pass as it is expected that
upstream releases have already been tested.

### Test with the SDK Builder

For more information on the SDK Builder, see
[Chromium OS SDK Creation](/chromium-os/build/sdk-creation).

To test building the SDK itself with the new package, by running the entire SDK
builder process, launch an SDK builder tryjob:

```
cros tryjob -g <cl 1> [-g ...] chromiumos-sdk-tryjob
```

Or, to just build the SDK board locally:

```
~/chromiumos/src/scripts/build_sdk_board
```
