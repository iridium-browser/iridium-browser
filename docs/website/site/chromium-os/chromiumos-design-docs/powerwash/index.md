---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: powerwash
title: Powerwash
---

**Status**: v1 implemented in M23

**Goal**

Provide users with a means to clear mutable system data as part of the base OS.

**Background**

There are a number of cases where it can be useful for users to have a safety
hatch that doesn't require knowledge of the developer switch or have a valid
recovery device (USB Stick, or SD card) handy:

*   Disk corruption: in the wrong place, it could impair the ability to
            sign in to any account
*   Bad update interaction: may impair sign-in or other behavior
*   Too many accounts to delete one at a time: happens in lab
            environments or other high traffic locations
*   ...

**Requirements**

> v1:

    *   Must be accessible from the normal UI
    *   Must be accessible before sign-in
    *   Must not be usable by a remote attacker with a single account
                compromise
    *   Must not destroy unrecoverable data: Limited TPM state on disk,
                Install-time Attributes file

> v2:

    *   Must clear the TPM, or
    *   Must preserve enterprise enrollment status
    *   May preserve enterprise enrollment token

**Overview**

Powerwash will provide a key combination that is accessible at the sign-in
screen (Ctrl+Alt+Shift+R) and a menu item in Settings, after sign-in, to trigger
a system wipe. The wipe itself will using an existing implementation and will be
performed at boot-time.

**Detailed Design**

Powerwash is simple in practice. It performs the following steps at boot-time:

*   Preserve any files desired by moving them to /tmp (a ramdisk)
*   Perform a fast wipe of the stateful partition (using the same script
            as the developer mode transition back to verified)
*   Restore the files preserved in /tmp

In order to trigger those steps, a request from Chrome must be made to the
session manager requesting a wipe. If no user has signed in to the system yet,
then the request will be granted, and a wipe will occur on the next boot. If a
user has signed in, then a request to wipe the system will result in a reboot
followed by a confirmation request at the sign-in screen. This is meant to limit
the risk of a single compromised account from being used to wipe a machine.

**Interface to Chrome**

Chrome will communicate with session_manager over DBus. A call to
StartDeviceWipe() will trigger the request. If no user has signed in, then
session_manager writes "fast safe" to a reset file:
/mnt/stateful_partition/factory_install_reset. This is already used for factory
wipes, so the changes are minimal.

**Wipe mechanism**

During boot, chromeos_startup checks the ownership (root) and existence of the
reset file. This will follow the normal factory reset wipe path except that it
will not use the factory wipe screen. clobber-state is then called with the
arguments in the reset file, as mentioned above.

**Preserved Files**

In order to preserve files, the "safe" argument is added to clobber-state. A
"safe" wipe will use the tar utility to preserve the path, ownership, and
permissions of any files that should survive the wipe. At present, the install
attributes file (lockbox.pb) and the TPM key blobs are preserved. The TPM is not
cleared by a power wash which means that the lockbox cannot be reinitialized nor
can some pre-TPM-ownership actions be taken.

**Internationalization**

The UI in Chrome will use the normal localized asset infrastructure.

The wipe message will use chromeos-boot-alert's localized asset implementation.

**Security considerations**

The primary security considerations here are around whether a remote attacker
can wipe a system. While this is not necessarily the most malicious of attacks,
it would not be a pleasant user experience in the least. To limit this attack,
we've required that powerwash be triggered or confirmed prior to any active
accounts signing in. This means that an unprivileged attacker could request a
powerwash, but not persist across the reboot to confirm it (minus a small
chronos-owner attack surface at sign-in time). If an attacker had gained higher
privileges, then they could wipe the device without help.

**Privacy considerations**

This feature allows users to wipe all user data and no user data is generated in
the process.

**Testing Plans**

(detailed elsewhere)

**Caveats**

**Differences from a developer mode wipe:**

*   The TPM is not cleared which means:
    *   enterprise enrollment after a powerwash will fail
    *   shared-data disk encryption key will not have changed
    *   lockbox & tpm data must be manually preserved using "safe"

**Constraints on v1/Additional work needed:**

*   Enterprise enrolled devices cannot be reset using this mechanism.
    *   The UI does not support it.
    *   The implementation may not allow re-enrollment after a wipe (if
                the UI supported it).
*   crossystem clear_tpm_owner_request is not in use.
