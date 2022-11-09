---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/get-the-code
  - 'Get the Code: Checkout, Build, & Run Chromium'
page_name: multiple-working-directories
title: Managing Multiple Working Directories
---

**git worktree**

If you only want to do some quick git operations, like merging/rebasing other
branches, but don't want to make your working directory dirty and cause
unnecessarily rebuild, git worktree is a good solution. It only handles one
single git repo though, and doesn't come with Chromium environment setup like
gclient and install-build-deps.

If you need to have multiple working directories that you can actually build,
use gclient-new-workdir instead. Using git worktree and set up environment there
would be slower and use more disk space.

**gclient-new-workdir**

If you are a multitasker or want to build chromium for different targets without
clobbering each other, then perhaps you'll like the
[gclient-new-workdir.py](https://chromium.googlesource.com/chromium/tools/depot_tools.git/+/HEAD/gclient-new-workdir.py)
script located in
[depot_tools.](http://www.chromium.org/developers/how-tos/depottools) The script
works by creating a new working directory with symlinks pointing to the git
database(s) found in your original chromium checkout. You can have as many
working directories as you want without the overhead and hassle of cloning
chromium multiple times.

```none
gclient-new-workdir.py /path/to/original/chromium chromium2
```

If your file system supports copy-on-write, like btrfs, gclient-new-workdir is
smart enough to use reflink to save copying time and disk space. Better yet, if
the source repo is a btrfs subvolume, btrfs snapshot would be created, with all
the artifacts retained. You can skip environment setup, and rebuilding should be
incremental. This is the most space-efficient way to create a separate working
directory. See <https://crbug.com/721585> for details.

### Windows devs

gclient-new-workdir.py doesn't support Windows, but you can try[
https://github.com/joero74/git-new-workdir](https://github.com/joero74/git-new-workdir)
to do the same thing **(needs to be run as admin)**. For the curious, the script
essentially uses mklink /D and other minor tricks to setup the mirrored .git
repo.

### Chromium OS devs

gclient-new-workdir.py uses symlinks that will not work inside the cros_sdk
chroot. If using a [local Chromium source for Chromium
OS](/chromium-os/developer-guide#TOC-Making-changes-to-non-cros_workon-able-packages),
be sure to use the original working directory.
