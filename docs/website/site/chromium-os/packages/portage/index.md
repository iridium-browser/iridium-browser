---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/packages
  - packages
page_name: portage
title: 'portage: the Gentoo package manager (aka emerge)'
---

[TOC]

## Introduction

We use Gentoo's portage (aka emerge) as the package manager in Chromium OS. This
page is more geared towards developers of the portage tool itself rather than
developers just using it (i.e. for ebuilds, or configuration, etc...).

You can find Chromium OS's mini-fork here:
<https://chromium.googlesource.com/chromiumos/third_party/portage_tool>

Changes constantly flow back into upstream Gentoo's tree, but we backport fixes
sometimes.

## Upstream Repo

The upstream repo can be found here:
<https://gitweb.gentoo.org/proj/portage.git>

You can add it to your local tree:

```none
git remote add --tags upstream git://anongit.gentoo.org/proj/portage.git
```

Then you can view/cherry pick easily from there.

## Hacking on Portage

Portage is a standard cros-workon package. That means you can use the normal
`cros_workon` tool to start hacking on portage, and using the source repo under
`src/third_party/portage_tool/` as your base.

Keep in mind that normally when you run `emerge` or `emerge-$BOARD` inside the
sdk, you're running the host copy of portage. If you want to test changes there,
you'll want to do:

```none
$ cros_workon --host start portage
$ sudo emerge portage
```

Then run your tests on `emerge-$BOARD` or whatever.

## Upgrading Chromium OS

We create a `chromeos-<ver>` branch for each version of portage that we track.
So in the case of our 2.2.12 version, we have a [chromeos-2.2.12
branch](https://chromium.googlesource.com/chromiumos/third_party/portage_tool/+/chromeos-2.2.12).

When doing an upgrade to a new version, you'll want to follow these steps:

1.  Create a new branch for the new version you want to upgrade to. If
            you want to upgrade to 2.3.4, then create a chromeos-2.3.4 branch.
            The initial branch point should match the respective portage tag.
    1.  You can do this via the [gerrit admin
                page](https://chromium-review.googlesource.com/admin/projects/chromiumos/third_party/portage_tool),
    2.  Or via git (if you have permission),

        ```none
        git push https://chromium.googlesource.com/chromiumos/third_party/portage_tool portage-2.3.34:refs/heads/chromeos-2.3.34
        ```

    3.  Or talk to a [build team
                member](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/contact.md#Public-discussion-groups)
                for help.
2.  Once that branch has been created, you should rebase any existing
            CrOS changes we have onto it. This means the normal review & merge
            process with gerrit.
    1.  Note: These changes won't be vetted by the CQ as the new version
                won't be used yet!
    2.  Now would be a good time to squash/update/discard any CLs that
                are no longer needed.
3.  You'll want to do validation work yourself locally.
    It's a cros-workon package now, so you can run:

    ```none
    $ cros_workon --host start portage
    ```

    Then make sure to start with a new chroot:

    ```none
    $ ./chromite/bin/cros_sdk -r
    ```

    Make sure `cros build-packages` can finish without using binary packages:

    ```bash
    $ cros build-packages --board=amd64-generic --no-usepkg
    ```

    This implicitly tests `update_chroot` and `setup_board`. Also, it makes sure
    the common packages can be built by the new version of portage. Binary
    packages can hide incompatibilities that show up whenever the packages are
    rebuilt later. After that looks good test a representative set of boards
    covering the various hardware architectures and board types to catch
    incompatibilities in less common packages.
4.  Once all the changes have been merged into the new branch, we need
            to test in the CQ/trybots. Manifest changes are not well tested
            currently, but we can approximate this with a merge commit.
    1.  Manually merge the new branch into the new one.

        ```none
        # Assuming the current branch is the old one.
        $ git merge portage-2.3.34
        ```

    2.  Resolve any conflicts so it looks like the new chromeos-2.3.34
                branch.
    3.  Upload that merge commit to Gerrit.
    4.  Let it run through CQ dry-run to get initial coverage. You can
                also launch manual tryjobs with this CL.
    5.  Once you're confident it's working, abandon this merge commit.
5.  Finally, switch the manifest to point to the new branch. This should
            go through the CQ and should validate that the new version works.

    ```none
    <project path="src/third_party/portage_tool"
             name="chromiumos/third_party/portage_tool"
             revision="refs/heads/chromeos-2.3.4" />
    ```

Note: In the past, we used the `master` & `next` branches to do development.
Since we switched over to the cros-workon flow, those are no longer used.
