---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: chroot_version_hooks
title: chroot_version_hooks
---

### Summary

On occasion changes are made to some component of the Chromium OS source that
are not backwards-compatible with existing chroots. When this happens a "chroot
version hook" should be used to make whatever changes are necessary to an
existing chroot to make it compatible. Often this involves deleting some
previously compiled/generated component in order to force it to be
compiled/generated again.  Another possible scenario is when there is a Chromium
OS source change that is dependent on a change coming in a future version of the
SDK, a version hook is needed to bridge that gap between SDK releases and make
that change directly in existing chroots (e.g. a config file change).

### Details

The version hooks all exist under
[src/scripts/chroot_version_hooks.d](https://chromium.googlesource.com/chromiumos/platform/crosutils/+/HEAD/chroot_version_hooks.d/).
Each hook script has a name that begins with "&lt;num&gt;_", where &lt;num&gt;
is an integer that represents the version number of a chroot. Whenever a new
chroot is entered (via `cros_sdk`) or `cros build-packages` is run all upgrade
hooks with versions newer than the chroot version are run (in sequential order)
to upgrade the chroot to the latest version.

Upgrade scripts can be arbitrary bash scripts. The best way to write a new
script is to look for existing examples in chroot_version_hooks.d. With that
said, the following example hook script will delete the build roots for all
boards and all Chrome artifacts. This is needed, for example, when a toolchain
uprev needs to be reverted. Everything built with the newer (bad) toolchain must
be deleted.

```none
# This is to clear the build roots for all boards to cope with a
# toolchain revert.
for board_root in /build/* ; do
  board=${board_root##*/}
  emerge_board=$(type -P emerge-${board} 2>/dev/null || true)
  if [[ -x "${emerge_board}" ]]; then	
    # It is a valid baord.
    build="/build/${board}"
    if [[ -d ${build} ]] ; then
      # The board has a build root to clear.
      info "Deleting ${board} build root at ${build}"
      sudo rm -rf ${build}
      info "Running setup_board --board=${board}"
      ~/trunk/src/scripts/setup_board --board=${board} --skip_chroot_upgrade
    fi
  fi
done
# Delete Chrome artifacts
sudo rm -rf /var/cache/chromeos-chrome/* || true
```
