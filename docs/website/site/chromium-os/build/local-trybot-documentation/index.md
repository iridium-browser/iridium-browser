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
documentation](/chromium-os/build/using-remote-trybots).

NOTE: The first time you run the trybot it will sync down a fresh checkout of
the source, build a new chroot and board, which will make the initial run take
longer than subsequent runs. An incremental run with an existing board takes
30min-40min.

**Pre-instructions (These are important!)**

1.  Make sure you’ve read through the [ChromiumOS Developer
            Guide](http://www.chromium.org/chromium-os/developer-guide) and can
            kick off a build properly.
2.  [Modify your sudo
            config](/chromium-os/tips-and-tricks-for-chromium-os-developers#TOC-Making-sudo-a-little-more-permissive).
            If you are at Google, you need to follow [these
            instructions](https://sites.google.com/a/google.com/chromeos/resources/ubuntu-workstation-notes)
            first (Under 'Tweaking with /etc/sudoers' section).
3.  Run sudo aptitude install kvm pbzip2 zip.
4.  Run sudo aptitude safe-upgrade.
5.  Run gclient from outside the chroot to update depot_tools.
6.  If you haven’t rebooted your computer in a while, reboot it now
            after updating.

## **Verifying the Trybot**

It’s good to first verify things are working properly with your system setup.

To do so, run the following from within your repo source checkout:

1.  Run repo sync to get latest version of trybot.
2.  Run cros tryjob --local amd64-generic-pre-cq to do a test run with
            the current TOT. Everything should pass.

## **Instructions For Using the Trybot**

Run the following from within your repo source checkout:

1.  Run repo sync to get latest version of cros trybot.
2.  Run cros tryjob --list to see a list of configs to run with. Add
            additional arguments to filter, such as cros tryjob --list release

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
cros tryjob --local -g 12345 -g *4168 samus-paladin-tryjob
```

This patches in three CL's: 1) an external CL using Gerrit change-ID 2) an
external CL using Gerrit change number 3) internal CL using change-iD. In case a
CL has several patches associated with it, the latest patch is used.

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

## **Sample Trybot Run**

```none
rcui@ryanc$ cros tryjob --local -g 'I5bed88ef 4168' -p 'chromiumos/chromite chromiumos/platform/crosutils' x86-alex-pre-cq
```

```none
```

```none
WARNING: Using default directory /usr/local/google/home/rcui/trybot as buildroot
```

```none
```

```none
Do you want to continue (yes/NO)? yes
```

```none
```

```none
INFO: Saving output to cbuildbot.log file
```

```none
[sudo] password for rcui: 
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