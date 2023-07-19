---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: encrypted-partition-recovery
title: Encrypted Partition Recovery
---

**Background**

Starting with R22, /var and /home/chronos are bind mounted from the encrypted
stateful partition (/mnt/stateful_partition/encrypted). Since this partition is
tied to the TPM state, it will not be available when switching to dev mode, and
cannot be decrypted if the drive is removed from the device. Additionally, some
devices don't have a removable drive, making content extraction very difficult
even without handling the encryption.

The current process to extract the contents depends on a signed recovery image
written to a USB stick that has been modified to start the extraction process.
This will cause the system to reboot twice: first into verified mode to perform
decryption and then back into recovery mode to copy the results to the USB
stick. Specifically, this will copy the user’s eCryptfs home directory contents
(and, when the user is the System Owner, also the "encrypted stateful" system
partition) to the recovery USB stick.

**Prerequisites**

1.  Obtain device in verified mode that is misbehaving (this process
            assumes it does not misbehave until after cryptohomed has started
            running).
2.  Obtain device user's username (full email address) and password.
3.  Obtain USB stick at least 4GB in size.
4.  Obtain [signed recovery
            image](https://support.google.com/chromebook/answer/1080595) for the
            device.
5.  Generate modified recovery image for stateful partition extraction:
    1.  Put image inside [cros chroot](/chromium-os/developer-guide)
                (e.g. ~/trunk/recovery_image.bin).
    2.  Run modification script (this grows the recovery image’s
                stateful partition to 3GB to hold the device’s stateful
                contents, and creates a flag file):
                ~/trunk/src/platform/dev/contrib/mod_recovery_for_decryption.sh
                --image ~/trunk/recovery_image.bin --to
                ~/trunk/recovery_decrypt.bin
6.  Write modified recovery image to USB stick.

**Process**

1.  DO NOT switch device to Developer mode, as the TPM will get cleared,
            making it impossible to decrypt the stateful partition.
2.  Boot device into Recovery mode.
3.  Insert modified recovery USB stick (built during “Prerequisites”
            above).
4.  Observe "Stateful recovery requested." statement on-screen, then
            wait. (If normal recovery mode starts doing image verification,
            power off the device before it overwrites itself, and see
            troubleshooting below).
5.  Observe “Stateful recovery requires authentication.” Enter username
            (full email address) and password, then wait. (To include the system
            partition in the recovery, the System Owner's credentials should be
            used.)
6.  Observe automatic reboot back into Verified mode. Wait at login
            screen while files are copied. Copying may take some time, so be
            patient. If the device has a disk activity light, it should show
            activity during this step.
7.  Observe automatic reboot back into Recovery mode.
8.  Observe "Stateful recovery requested." statement on-screen, then
            wait. (If normal recovery mode starts doing image verification,
            power off the device before it overwrites itself, and see
            troubleshooting below).
9.  Observe “Prior recovery request exists. Attempting to extract
            files.” on-screen, then wait while files are copied.
10. Observe “Operation complete, you can now remove your recovery
            media.”
11. Pull USB stick, and mount first partition on a test machine.
            Device’s recovered contents will be in /recovery/decrypted/.

**Troubleshooting**

If the on-screen statement in step 4 (or 8) is not seen, something went wrong
with the recovery image modification, and the device should be powered off to
avoid wiping the device’s stateful partition. Recheck the prerequisites, and
make sure the image on the USB stick was successfully modified.

If the system never reboots to step 7, the system is likely in such a bad state
that even cryptohomed hasn’t started. Handling this situation is out of scope of
this document; a new bug should be opened for this. Large copies can take a
while, so check for disk activity.

If step 10 fails (e.g. “Copying failed. Please resize your stateful partition
and try again!”), something went wrong with the copying from the device to the
USB stick. On devices that have a removable drive, the decrypted files should be
visible in /mnt/stateful_partition/decrypted. Otherwise, try again with a larger
USB stick, and increase the number of sectors used when modifying the recovery
image via the --statefulfs_sectors argument.
