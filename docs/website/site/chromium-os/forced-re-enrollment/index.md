---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: forced-re-enrollment
title: Forced re-enrollment
---

The enrolled Chrome OS devices can be reset. The reset process cleans all the
state-full partitions and the TPM. On the first boot after reset, the device is
clean. Normally it would lose the enrollment and all the policy that comes with
it. But in most of the cases, the admins want the device to remain enrolled
after reset. For this purpose a process that forces the device to be re-enrolled
after reset was designed and implemented. The process is called forced
re-enrollment (FrE).

When is it applied?

The device is forced to re-enroll only after device reset if the admin chose to
enable FrE in the admin console.

How does it look from user perspective?

On first boot after reset the previously enrolled device has to connect to
internet to proceed. After user chooses the connection and the device is
connected, it will check with DM server if the device has FrE enabled. If that's
the case, the user is presented with enrollment screen with the enrollment
domain given (as received from DM server). User has to enter credentials for the
domain and the device will do the normal enrollment process in the background
that includes downloading and saving the device and user policy.

What happens from the code perspective?

There are two separate processes that can happen on the device:

1. The check to know if the device is forced into re-enrollment.

2. The enrollment process itself.

The check for FrE initially was happening at every device reset for every Chrome
OS device. It meant that every device after reset would communicate with DM
server to check if it's forced to re-enroll. That generated a privacy concern
that it would allow Google servers to get information about the devices every
time they reset. To alleviate this concern a separate process called hash dance
has been designed and implemented.

Hash dance

The purpose of hash dance is to communicate with DM server in a way that would
provide as little information as possible to the server about the device, and
yet identify if the device is forced to re-enroll.
