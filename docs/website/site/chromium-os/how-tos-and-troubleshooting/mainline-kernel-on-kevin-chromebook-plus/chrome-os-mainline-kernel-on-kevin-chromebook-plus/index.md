---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
- - /chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus
  - Arch Linux + Mainline kernel on kevin (Chromebook Plus)
page_name: chrome-os-mainline-kernel-on-kevin-chromebook-plus
title: Chrome OS + Mainline kernel on kevin (Chromebook Plus)
---

Adapted from Tomasz's instructions
[here](https://bugs.chromium.org/p/chromium/issues/detail?id=907715#c7)

## Applying the downstream patches

There are a few patches you must apply to Chrome OS and the kernel to get things
working. The following commands will bootstrap your repo.

**1-** Cherry pick the CLs from [this
ta](https://chromium-review.googlesource.com/q/hashtag:%2522scarlet-upstream%2522+status:open)g.
For convenience, just run the following commands from your src dir in your
chromiumos checkout:

```none
pushd third_party/chromiumos-overlay/ && \
    repo start rockchip_upstream . && \
    git fetch https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay refs/changes/89/1348489/4 && git cherry-pick FETCH_HEAD && \
    git fetch https://chromium.googlesource.com/chromiumos/overlays/chromiumos-overlay refs/changes/36/1458536/1 && git cherry-pick FETCH_HEAD && \
    popd
pushd overlays/ && \
    repo start rockchip_upstream . && \
    git fetch https://chromium.googlesource.com/chromiumos/overlays/board-overlays refs/changes/11/1351911/1 && git cherry-pick FETCH_HEAD && \
    popd
pushd platform/minigbm && \
    repo start rockchip_upstream . && \
    git fetch https://chromium.googlesource.com/chromiumos/platform/minigbm refs/changes/52/1347652/4 && git cherry-pick FETCH_HEAD && \
    popd
```

**1a-** If you have an internal checkout, also cherry pick the patch from [this
tag](https://chrome-internal-review.googlesource.com/q/hashtag:%2522scarlet-upstream%2522+status:open).
Again, for convenience, here is the command:

```none
pushd private-overlays/baseboard-gru-private/ && \
    repo start rockchip_upstream . && \
    git fetch https://chrome-internal.googlesource.com/chromeos/overlays/baseboard-gru-private refs/changes/14/722014/1 && git cherry-pick FETCH_HEAD && \
    popd
```

**2-** Check out an upstream kernel into the v4.4 kernel directory, and apply
the attached patches:

```none
pushd third_party/kernel/v4.4/ && \
    git remote add torvalds git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git && \
    git fetch torvalds && \
    git branch -t rockchip_upstream torvalds/master && \
    git checkout rockchip_upstream && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0001-FROMLIST-drm-panel-add-Kingdisplay-kd097d04-panel-dr.patch' | git am) && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0002-CHROMIUM-drm-Add-drm_master_relax-debugfs-file-non-r.patch' | git am) && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0003-CHROMIUM-drm-Allow-DRM_IOCTL_MODE_MAP_DUMB-for-rende.patch' | git am) && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0004-CHROMIUM-drm-Allow-render-node-access-to-KMS-getters.patch' | git am) && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0005-CHROMIUM-drm-Allow-DRM_IOCTL_MODE_DESTROY_DUMB-for-r.patch' | git am) && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0006-CHROMIUM-drm-rockchip-Enable-rendernode.patch' | git am) && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0007-CHROMIUM-drm-rockchip-Add-GEM-ioctls.patch' | git am) && \
    (curl -L 'https://www.chromium.org/chromium-os/how-tos-and-troubleshooting/mainline-kernel-on-kevin-chromebook-plus/chrome-os-mainline-kernel-on-kevin-chromebook-plus/0008-CHROMIUM-Revert-drm-vgem-create-a-render-node-for-vg.patch' | git am) && \
    popd
```

Alternatively, you can use [the following
branch](https://gitlab.freedesktop.org/seanpaul/kernel/tree/rockchip_upstream)
with the patches already applied:

```none
pushd third_party/kernel/v4.4/ && \
        git remote add seanpaul_gitlab https://gitlab.freedesktop.org/seanpaul/kernel.git && \
    git fetch seanpaul_gitlab && \
        git branch -t rockchip_upstream seanpaul_gitlab/rockchip_upstream && \
    popd
```

If you use the provided branch, you might want to rebase it on a fresher kernel
if it becomes stale \[exercise left to the reader\].

## Building the image

All of the pieces are in place to build your image with the upstream kernel! Run
the following commands in your chroot to build your image:

```none
export BOARD="kevin|scarlet"# Only run once./setup_board --board=$BOARDcros_workon-$BOARD start minigbm chromeos-kernel-4_4
```

For scarlet:

```bash
VIDEO_CARDS="llvmpipe rockchip" USE="llvm llvm_targets_ARM gpu_sandbox_failures_not_fatal -gpu_sandbox_start_early -tpm2 mocktpm $USE" cros build-packages --board=$BOARD
```

For kevin:

```bash
VIDEO_CARDS="llvmpipe rockchip" USE="llvm llvm_targets_ARM gpu_sandbox_failures_not_fatal -gpu_sandbox_start_early $USE" cros build-packages --board=$BOARD
```

Build a test image:

```bash
cros build-image --boot-args="noinitrd kgdboc=ttyS2 slub_debug=FZPUA" --enable-serial=ttyS2 --board=${BOARD} --no-enable-rootfs-verification test
```

Now follow the instructions from the [Chromium OS Developer
Guide](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_guide.md#Installing-Chromium-OS-on-your-Device)
to flash your image to a USB stick and boot it on your device.

Deploying changes to the kernel is as easy as:

```none
emerge-${BOARD} chromeos-kernel-4_4
./update_kernel.sh --remote <device_ip>
```
