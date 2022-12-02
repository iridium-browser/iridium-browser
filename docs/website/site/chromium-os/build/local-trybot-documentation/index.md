---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: local-trybot-documentation
title: Chromium OS Local Trybots
---

[TOC]

## Introduction

The local trybot allows you to emulate a buildbot run on your local machine with
a set of your changes. The changes are patched to tip of tree (TOT). They should
be very similar to remote tryjobs in most regards.

For Google developers, please also take a look at the [Remote Trybot
documentation](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/remote_trybots.md).

NOTE: The first time you run the trybot it will sync down a fresh checkout of
the source, build a new chroot and board, which will make the initial run take
longer than subsequent runs. An incremental run with an existing board takes
30min-40min.

**Pre-instructions (These are important!)**

1.  Make sure you’ve read through the [ChromiumOS Developer
            Guide](https://chromium.googlesource.com/chromiumos/docs/+/main/developer_guide.md)
            and can kick off a build properly.
2.  [Modify your sudo
            config](https://chromium.googlesource.com/chromiumos/docs/+/main/tips-and-tricks.md#How-to-make-sudo-a-little-more-permissive)
            If you are at Google, you need to follow [these
            instructions](http://go/cros-glinux-sudo#configuring-etcsudoers)
            first (Under 'Tweaking with /etc/sudoers' section).
3.  Run `sudo apt install qemu-kvm pbzip2 zip`.
4.  Run `sudo apt upgrade`.
5.  Run `gclient` from outside the chroot to update depot_tools.
6.  If you haven’t rebooted your computer in a while, reboot it now
            after updating.

## **Verifying the Trybot**

It’s good to first verify things are working properly with your system setup.

To do so, run the following from within your repo source checkout:

1.  Run `repo sync` to get latest version of trybot.
2.  Run `cros tryjob --local amd64-generic-full-tryjob` to do a test run with
            the current TOT. Everything should pass.

## **Instructions For Using the Trybot**

Run the following from within your repo source checkout:

1.  Run `repo sync` to get latest version of cros trybot.
2.  Run `cros tryjob --list` to see a list of configs to run with. Add
            additional arguments to filter, such as `cros tryjob --list release`

### To patch in a gerrit CL

**3a.** Run

```none
cros tryjob --local -g '[*]<cl_1> [*]<cl_2> .. [*]<cl_N>' config
```

Substitute &lt;config&gt; with your desired config. Prepend a '\*' to the CL ID
to indicate it's an internal CL. The CL ID can be a Gerrit Change-ID or a change
number.

An example:

```none
cros tryjob --local -g I5bed88effd9c4c26885f8c75da1ec2499c4b74c8 -g 12345 -g *4168 amd64-generic-full-tryjob
```

This example patches in three CLs:

1. an external CL using Gerrit change-ID (unabbreviated commit hash)
2. an external CL using Gerrit CL number
3. internal CL using Gerrit CL number. In case a CL has several patchsets
associated with it, the latest patchset is used.

To patch in multiple CLs, you can pass all CL numbers in a quoted,
space-delimited string, or specify the `-g` argument multiple times.

### To patch in a local change

**3c.** Run

```none
cros tryjob -p '<project1>[:<branch1>]...<projectN>[:<branchN>]' config
```

Specify the name of the project (not the path) and optionally the project
branch. If no branch is specified the current branch of the project will be
used.

NOTE: To get the project name of the project you're working on run repo list.

NOTE: Do not do development within the trybot buildroot! Your changes will get
wiped out on the next trybot run. Develop within your source root, and patch in
changes.

NOTE: Use the --nosync option to prevent the trybot from updating its source
checkout. See the Tips section below for more info.

NOTE: Per the output of `cros tryjob --help`, the `-p` option is known to be
buggy and `-g` is preferred for specifying patches. Note that `-g` requires the
specified change to be uploaded to Gerrit.

## **Sample Trybot Run**

```shell
$ cros tryjob --local -g 'I5bed88effd9c4c26885f8c75da1ec2499c4b74c8 4168' -p 'chromiumos/chromite chromiumos/platform/crosutils' amd64-generic-asan-tryjob

WARNING: Using default directory /usr/local/google/home/ldap/trybot as buildroot

Do you want to continue (yes/NO)? yes

INFO: Saving output to cbuildbot.log file

[sudo] password:
```

NOTE: The output of the trybot is automatically saved to a cbuildbot.log file in
&lt;trybot_root&gt;/cbuildbot_logs. The log directory is printed out at the end
of a run.

## **Overview of Trybot Operation**

The first time you run the trybot, it will sync down its own checkout of the
source, and build its own chroot. The CL's you specify to patch are patched to
the trybot's own source tree.

When you specify local changes to patch (by specifying a project and branch the
changes are on) the trybot will generate git patch files based on those changes
and apply the patch files to its private source checkout.

When you specify a Gerrit changelist, the trybot will look up the changelist
info from Gerrit, fetch the ref of the change, and rebase it to TOT in its
private source checkout.

## **Feedback**

Thanks for using the trybot! Please contact
[chromium-os-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-os-dev)
for any issues you run into. Please report any bugs you find, and file feature
requests. There will be active development on the trybot, so please sync to the
latest version before you run it.
