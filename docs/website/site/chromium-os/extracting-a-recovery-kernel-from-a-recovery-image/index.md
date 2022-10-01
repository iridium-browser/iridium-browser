---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: extracting-a-recovery-kernel-from-a-recovery-image
title: Extracting a Recovery Kernel from a Recovery Image
---

## If you need to generate a recovery image, use "mod_image_for_recovery.sh --image &lt;image_file&gt;".

## &lt;Outdated instructions needed for a few special case CR-48 Users&gt;

## Intro

If you find yourself in the need of an officially signed Recovery Kernel, but
you only have an officially signed Recovery Image: you're in luck! You've come
to the right place.

What are Recovery Images and Recovery Kernels?

*   A **Recovery Image** is an image that can be placed onto a USB disk
            that can boot a particular Chrome OS Notebook and reinstall the
            default software onto the computer, putting it into a known good
            state. Instructions for getting a Recovery Image are usually
            provided by the manufacturer of a Chrome OS Notebook.
*   A **Recovery Kernel** is a small part of the the Recovery Image that
            can be extracted, then used for installing a custom build of the OS.
            Sometimes a manufacturer will provide a separate download of the
            Recovery Kernel as a convenience.

What does it mean for a Recovery Kernel to be officially signed? It means that
it was built by someone (like your device manufacturer) with access to your
device's secret recovery key. That means that your device will allow booting
from this Recovery Kernel. **NOTE**: every hardware product has a different
secret recovery key, so you need to obtain an official Recovery Kernel that is
specific to your product.

...you might have read about the Recovery Kernel in such places as the [Poking
around your Chrome OS
Notebook](/chromium-os/poking-around-your-chrome-os-device) page or the
[Chromium OS Developer Guide](/chromium-os/developer-guide).

## Get an officially-signed Recovery Image

These steps are going to be different for every piece of hardware. However, just
to be illustrative, here are the steps for the Cr-48 Chrome Notebook:

```none
mkdir -p ~/trunk/recovery
curl -L 'https://dl.google.com/dl/chromeos/recovery/latest_mario_beta_channel' > ~/trunk/recovery/recovery_image.zip
TMPDIR=`mktemp -d`
unzip ~/trunk/recovery/recovery_image.zip -d ${TMPDIR}
RECOVERY_IMAGE=`ls ${TMPDIR}/*.bin`
```

Future steps will assume that you've put the recovery image into the
`RECOVERY_IMAGE` variable (as the above instructions do).

## Extract the Recovery Kernel

The easiest way to do this is if you've already followed the instructions in the
[Chromium OS Developer Guide](/chromium-os/developer-guide) and are running from
within the chroot. In that case, you'll have already the `cgpt` tool. That means
you can follow the generic instructions. In that case, the instructions are:

```none
KERNA_START=`cgpt show -b ${RECOVERY_IMAGE} | grep KERN-A | awk '{print $1}'`
KERNA_SIZE=`cgpt show -b ${RECOVERY_IMAGE} | grep KERN-A | awk '{print $2}'`
mkdir -p ~/trunk/recovery/
RECOVERY_KERNEL=~/trunk/recovery/recovery_kernel.bin
dd of=${RECOVERY_KERNEL} if=${RECOVERY_IMAGE} \
   bs=512 count=${KERNA_SIZE} skip=${KERNA_START}
```

If you aren't running from the chroot, most of the commands above are still
valid. You'll just have to figure out your own `KERNA_START` and `KERNA_SIZE`.
At the moment, this document doesn't have the instructions for that (sorry!).
However with current images, you could actually just do:

```none
KERNA_START=4096
KERNA_SIZE=32768
```

...and be done with it.
