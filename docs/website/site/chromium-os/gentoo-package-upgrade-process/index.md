---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: gentoo-package-upgrade-process
title: Portage New & Upgrade Package Process
---

[TOC]

#### Deprecation warning

**This page has been migrated!**

[Please refer to the updated, in-repo copy
instead](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/portage/package_upgrade_process.md).
Feel free to contribute or file bugs if it is in any way lacking.

### Purpose

This page documents the process of upgrading a Portage package in our source to
the latest upstream version, or for pulling in a new package that doesn't yet
exist (since that's the same thing as upgrading from version &lt;none&gt;).
These instructions are intended for Chromium OS developers.

The upgrade process documented here uses a script, called cros_portage_upgrade,
developed to help automate/simplify this process. It is not required that you
use this script, and if you are confident that you can update packages in
another way you are free to do so. Some of the FAQ on this page may still be of
use to you, in any case.

### Quickstart

A quick reminder of the steps to perform in your chroot set up per [Chromium OS
Developer Guide](http://www.chromium.org/chromium-os/developer-guide):

1.  Enter chroot: **$REPO/chromite/bin/cros_sdk**
2.  Set up boards to test on: **~/trunk/src/scripts/setup_board
            --board=daisy**
3.  Start a new git branch in portage-stable repo:
    cd ~/trunk/src/third_party/portage-stable**
    git checkout -b NEWBRANCH --track remotes/cros/master**
4.  Upgrade package locally:
    **cros_portage_upgrade --upgrade --board=&lt;board&gt;:&lt;board&gt;...
    some_package1 some_package2 ...**
5.  Repeat previous step as needed, fixing any upgrade errors that
            appear.
6.  Test. Yes. Really. You.
7.  Add BUG/TEST fields to commit message: **git commit --amend**
8.  upload change for review: **repo upload .**
9.  And later, abandon branch: **git branch -D NEWBRANCH**

### Assumptions

The following assumptions are made with these instructions:

*   You know which package or packages you want to upgrade.
*   You know how you intend to specifically test the package(s) after
            the upgrade (beyond the basic smoke tests).
*   You are familiar with the build environment from a developer
            perspective. You know how to start branches, amend commits, upload
            commits, end branches, etc. See the [Chromium OS Developer
            Guide](http://www.chromium.org/chromium-os/developer-guide).

### Brief Background

Any Portage packages, upstream included, can be marked as stable or unstable for
any architecture. You don't have to know anything more about this detail unless
you need to upgrade a package to an unstable version. However, this happens
somewhat frequently. Perhaps a feature you need is only available in the
absolute latest version of a package, but that version has not been marked
stable for x86, yet. If you do need to upgrade to an unstable version check the
appendix entry below.

The chromiumos source has projects that are portage "overlays", all under
src/third_party. Three of them are of interest here.

1.  The src/third_party/portage overlay represents a snapshot of the
            upstream Portage source. Currently the snapshot is really old but
            there is a project underway to update it more regularly. This
            overlay used to serve as the default source for all Portage packages
            that we use. Now it serves as an upstream reference snapshot and
            nothing more.
2.  The src/third_party/portage-stable overlay has copies of all
            upstream Portage packages that we use unaltered. When we "upgrade" a
            package, what we are really doing is putting a copy of the current
            upstream package into this overlay.
3.  The src/third_party/chromiumos-overlay is probably familiar to you.
            For package upgrade purposes, it will only be used in the following
            scenario:
    1.  If a package is locally patched before the upgrade, then you
                will probably have to port the local patch to the newer version
                as well. That should be done in this overlay.

### The Steps

#### Plan your upgrade

Figure out which package or packages you want to upgrade, using their full
"category/package" name (e.g. sys-apps/dbus). Decide whether you want to upgrade
them to the latest stable or unstable versions. Generally, your first choice
should be to upgrade packages to the latest **stable** upstream version.

Note whether any of the packages you need to upgrade are locally patched. This
means that somebody, possibly you, has made a patch to the package that we are
using right now. It is important to determine what should happen to that patch
after the upgrade. Inspect the git history for the patch, talk to the committer,
inspect the patch, etc.

If the package lives in the portage-stable overlay, you do not need to worry
about this as we do not permit patches to be made in there. For all other
overlays, it's pretty much guaranteed there are custom changes in play.

*   If the patch was done to backport a change from upstream, then
            simply updating the package to the upstream version should be
            sufficient and the patch can be ignored. Be sure to confirm which
            version the patch was backported from.
*   If the patch is still required, then you will have to re-apply the
            patch after the upgrade. Go through the upgrade process, except for
            the final testing and uploading. Then follow the [guidelines for
            re-applying a patch](#AppendixReapplyPatch) below.

Decide which boards you want to try the upgrade for. You should include **one
board from each supported arch type** (e.g. amd64, arm, arm64), unless the
package is only used on one arch for some reason. Typically, there is no reason
to specify more than one board per architecture type, unless you know of
board-specific testing that must be done. If your package or packages are used
on the host, then you want to upgrade for the host (which can be done at the
same time as the boards). Many packages are used on a chromeos board as well as
the host (the "host" is the chroot environment). Choose boards that you can test
the upgrades on, or boards that you know the packages need to be tested on.

#### Prepare Environment

You probably want to 'repo sync' your entire source tree.

```none
cd ~/chromiumos && repo sync -j 4
```

Prepare a new branch in the src/third_party/portage-stable project. This is
where the upgrades will be staged/committed.

```none
cd ~/chromiumos/src/third_party/portage-stable && repo start pkg_upgrade .
```

Now you need to setup all boards that you will need. Note: this is not required
if you're upgrading a host-only package.

```none
cros_sdk --enter
./setup_board --board=amd64-generic
./setup_board --board=link
./setup_board --board=daisy
```

#### Upgrade Locally

For these instructions, let's say you want to upgrade "media-libs/alsa-lib",
"media-sound/alsa-headers", and "media-sound/alsa-utils" on boards
"amd64-generic" and "daisy". You plan to try the stable upstream versions first.
These commands must be run from **within your chroot**. If you also need to
upgrade on the host (chroot/amd64) use the --host option, which can be used in
combination with the --board option or without it.

```none
cros_portage_upgrade --upgrade --board=amd64-generic:daisy media-libs/alsa-lib media-sound/alsa-headers media-sound/alsa-utils
```

If the script runs all the way through without any problems the first time then
you have no missing dependencies or other build-related obstacles to the
upgrade. But you still need to test the upgrade results! The upgrade should be
prepared as a commit in src/third_party/portage-stable. You should inspect/edit
the commit message before uploading it, though.

If the script does not run all the way through the first time, it should give
you the means to determine why. Typically, this means that one or more of the
upgraded packages could not be built (using emerge) after the upgrade. This may
be due to a package dependency that must also be upgraded, or a package mask
that must be edited. In any case, you are given the output of the failing emerge
command so that you can determine for yourself what is needed. Make the
necessary change (perhaps adding another package to upgrade at the command
line), then run again.

To upgrade to unstable versions, simply add the --unstable-ok option and follow
the instructions. Or see the [guidelines for upgrading to unstable
versions](/chromium-os/gentoo-package-upgrade-process#AppendixUnstableVersion)
below.

Run cros_portage_upgrade with --help to see addition options and usage. If you
are having difficultly with this step please do not hesitate to contact
chromium-os-dev@chromium.org for assistance.

#### Clean up Commit

You may also need to clean up the files in your commit. Our convention is to
only check in files that are used, and some upgrades come with extra files (such
as unused patch files, for example). You can usually tell by inspecting the list
of files and the ebuild whether any files are unused. You can remove these files
from your commit with a command like the following within portage-stable:

```none
git reset HEAD~ <filepath> ; rm <filepath> ; git commit --amend
```

If you already built your upgraded package, make sure to do it again after
cleaning up extraneous files to verify that you haven't removed something
important!

#### Testing

Use common sense, and your specific expertise, to test the upgraded packages.
The changes are active in your source right now, so you can build your target
boards to test. If your package is added to the host (chroot) you'll need to
emerge it onto your chroot and test it there as well.

You probably want to run at least the suite:smoke tests for each board, which
you can do by following the tips at
<https://www.chromium.org/chromium-os/testing> (Googlers may
also use the tips at [goto/cros-test](http://goto/cros_test)). In particular,
you can use [trybot](/chromium-os/build/local-trybot-documentation) to determine
what effect your upgrade will have on the greenness of the waterfall.

One common cause of failure is that upstream has introduced new default
[USE](/chromium-os/how-tos-and-troubleshooting/portage-build-faq) flags. For
example, if the upstream ebuild was set to "-foo" before (which defaults "foo"
to off) and now is set to "+foo", a new dependency or a new runtime behavior
might have been introduced by the change. You would need to evaluate the change
and determine whether the new USE flag default is one that we should adapt to.
If, on the other hand, the flag adds a dependency on something that we don't use
in Chrome OS, you might overlay the USE flag default in the `chromiumos-overlay`
repo's `profiles/targets/chromeos/package.use` in a separate commit, first,
before submitting the `portage-stable` upgrade. See below for an explanation of
how chromiumos-overlay functions but note that you do **not** need to copy the
upstream package into chromiumos-overlay to override the default USE flags.

Beyond the suite:smoke tests, test whatever you think makes sense given the
packages you just upgraded.

#### Uploading

When you are satisfied that the upgrade is safe to push, **you still need to
edit the commit message that was created for you** by cros_portage_upgrade. Add
a bug number, test details, and any other details you want.

```none
cd ~/chromiumos/src/third_party/portage-stable
git commit --amend
```

Upload the usual way.

```none
cd ~/chromiumos/src/third_party/portage-stable
repo upload .
```

Then go through the usual code review process on gerrit.

#### Clean up Environment

After your changelist is through review and submitted to the tree, you will want
to get off the branch you created in src/third_party/portage-stable. You know
the drill.

```none
cd ~/chromiumos/src/third_party/portage-stable
repo abandon pkg_upgrade .
```

### Appendices

#### **Re-applying a patch after upgrade**

It is not uncommon for a package to be locally patched in the source now
(typically any upstream package in the src/third_party/chromiumos-overlay
overlay). To upgrade that package the patch will (probably) need to be applied
again to the upgraded version. Let's say you upgraded the "foo/bar" package to
upstream version "1.2.3", but you need to re-apply a patch to it that was
previously applied to version "1.1.1" (perhaps "1.1.1-r1" is the active version
now with the patch).

Allow the cros_portage_upgrade script to complete the pristine upgrade in
src/third_party/portage-stable for the package first. Then copy the entire
directory over to the analogous location under
src/third_party/chromiumos-overlay, creating the directory if necessary You will
commit the unaltered package as it is in the first changelist, but first you
must mark it as unstable so that it will not be used. To do so, add a new file
like the following to the
src/third_party/chromiumos-overlay/profiles/default/linux/package.mask/
directory:

```none
# Copyright <year> The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file
# TODO(username): Masked for testing!
=foo/bar-1.2.3
```

The above masks the specific package version "foo/bar-1.2.3" for all targets.
(If your intent is to patch the ebuild only for certain platforms please speak
to the build team as this is generally unnecessary and strongly discouraged.)
Now you are ready to submit the pristine copy of the upstream package to
chromiumos-overlay.

Next make a copy of the ebuild file with a revision suffix. For example:

```none
cp bar-1.2.3.ebuild bar-1.2.3-r1.ebuild
```

Then re-apply the patch to the new revision ebuild, by copying the epatch line
or lines in the previously patched ebuild (bar-1.1.1-r1.ebuild). Be sure to keep
whatever patch files under the "files" directory are required for the patch,
too. You might have to rename them (e.g. cp files/bar-1.1.1-something.patch
files/bar-1.2.3-something.patch). **Be careful to apply the patch responsibly**,
because the source file(s) the patch alters may have changed since the patch was
created. If so, you can create a new patch or consult the original patch author
(if not you).

To inspect the source files involved, both before and after patching, use the
"ebuild" utility. For example, to see the source files for "foo/bar-1.2.3"
before any patching run this **inside chroot**, use a command like the following
examples:

```none
sudo ebuild `equery which foo/bar-1.2.3` unpack
sudo ebuild-amd64-generic `equery-amd64-generic which foo/bar-1.2.3` unpack
sudo ebuild-daisy `equery-daisy which foo/bar-1.2.3` unpack
```

The above will unpack source files and tell you where they are. You can inspect
them to verify whether the source file(s) the patch touches have changed between
versions. To see what the source files look like after patching, use the same
commands as above but replace "unpack" with "prepare". Also, make sure your
newly patched version is the version being picked up by the build system by
running "equery which" **inside chroot**:

```none
equery which foo/bar
equery-amd64-generic which foo/bar
equery-daisy which foo/bar
```

Make sure your new ebuild is the one being picked up. Your upgrade with patch is
ready for testing now. Return to the testing step of the process above.

Remember to delete the new entry you made to the package.mask file.

Don't forget to discard the upgrade in portage-stable that cros_portage_upgrade
created for you.

#### **Handling eclasses**

Some packages make use of
[eclasses](http://www.gentoo.org/proj/en/devrel/handbook/handbook.xml?part=2&chap=2),
which are essentially ebuild code factored out into library files shared across
multiple packages. If an ebuild file includes an inherit statement, then it uses
an eclass. You don't have to check for this before you upgrade. The upgrade
script will attempt to detect when an eclass must be upgraded along with a
package upgrade, but if it is unable to you must upgrade the eclass yourself. If
you are not sure how to do that please consult with a member of the build team.

#### **Upgrading to unstable version**

If you need to upgrade a package to an unstable version for a compelling reason,
then use the --unstable-ok option to the cros_portage_upgrade script and it will
try to do the right thing. What it does is edit the KEYWORDS line (or lines) in
the ebuild to mark all architectures as stable. *This is the only exception to
the rule that packages in portage-stable should be unedited.* You should inspect
the resulting ebuild file or files to confirm that the edit worked correctly,
and file a bug on cros_portage_upgrade if it did not.

### FAQ

**Q**: *I ran the upgrade script multiple times, and the runtime dropped
drastically after the first time. What gives?*
**A**: The cros_portage_upgrade script needs to have a local checkout of the
upstream origin/gentoo source to use as a reference when it runs. By default, it
creates a clone in a temporary directory and then re-uses it in later runs
(updating it each time). This explains the runtime symptoms. Messages from the
script should say something to this effect, as well.

Alternatively, you can clone your own copy of upstream origin/gentoo to point
cros_portage_upgrade to, using the --upstream option. This will have the same
runtime benefits if you run more than once. Do this inside your chroot:

```none
cd
git clone https://chromium.googlesource.com/chromiumos/overlays/portage gentoo-portage
cd gentoo-portage/
git checkout origin/gentoo
```

Then add the following option when you run cros_portage_upgrade:
--upstream=$HOME/gentoo-portage .

**Q:** *Why doesn't the upgrade script just run on all boards/architectures that
matter at once? Why do I have to specify them?*
**A:** The boards that "matter" vary. The same is true, to a lesser extent, with
architectures. You know how you intend to test the package, and you may have
plans to test it on specific boards for specific reasons. Plus,
cros_portage_upgrade requires that setup_board be completed for all specified
boards because it evaluates packages within the context of that board, making
use of emerge-&lt;board&gt; and equery-&lt;board&gt; utilities heavily. If you
already have a reasonable subset of boards setup, or available to test on, then
running on those boards makes sense.

**Q:** *Why doesn't the upgrade script run setup_board for me as needed?*
**A:** This script is intended to live within the build environment we have
today. It lives between the setup_board and build_packages stages, which all
developers are familiar with. The script is not intended to be a master script
with knowledge of build process order.

**Q:** *Why doesn't the upgrade script create the branch in
src/third_party/portage-stable for me?*
**A:** This was considered, but discarded in favor of transparency. You need to
know that a branch was created because you must amend the commit message, upload
it, and especially abandon the branch in the end. If you created the branch in
the first place you are more likely to be aware that you need to abandon the
branch to return to your previous state.

**Q:** *Why can't the upgrade script create a complete commit message so no
amend is needed, then do the upload and abandon steps as well?*
**A:** Firstly, only you know what tests you will run to verify the upgrade and
those should be mentioned in the commit message. Secondly, there are many
scenarios where the upgrade result is not final and requires
intervention/repetition by you.

**Q:** *How do you install a **new** package that is not currently installed? I
get the "The following packages were not found in current overlays (but they do
exist upstream): ..." error.*
**A:** Make sure you pass --upgrade to the script since installing a new package
is the same thing as upgrading it from &lt;none&gt;.
