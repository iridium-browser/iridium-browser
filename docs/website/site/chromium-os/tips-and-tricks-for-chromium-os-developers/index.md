---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: tips-and-tricks-for-chromium-os-developers
title: Tips And Tricks for Chromium OS Developers
---

## **This document can now be found here:**
##
**<https://chromium.googlesource.com/chromiumos/docs/+/main/tips-and-tricks.md>**

## **The content below is a (possibly out-of-date) copy for easier searching.**

## Introduction

This page contains tips and tricks for developing on Chromium OS.
The free-form content on this page is contributed by developers. Before adding
content to this page, consider whether the content might fit better in a
different document, such as the [Chromium OS Developer
Guide](http://www.chromium.org/chromium-os/developer-guide) or the [Chromium OS
Developer FAQ](/chromium-os/how-tos-and-troubleshooting/developer-faq):

*   **Content that belongs in this document:**
    *   tips that developers can use to optimize their workflow, but
                that aren't strictly required
    *   instructions to help developers explore/understand the build
                environment in greater depth

*   **Content that does not belong in this document:**
    *   things that every developer needs to know right away when they
                start developing (put such information in the [Chromium OS
                Developer
                Guide](http://www.chromium.org/chromium-os/developer-guide))
    *   things that only a very small subset of developers need to know
                (put such information on a page dedicated to that small subset
                of developers)

**Note:** The tips on this page generally assume that you've followed the
instructions in the [Chromium OS Developer
Guide](http://www.chromium.org/chromium-os/developer-guide) to build your image.

[TOC]

---

## repo

### How to set up repo bash completion on Ubuntu

Get a copy of
[repo_bash_completion](https://chromium.googlesource.com/chromiumos/platform/dev-util/+/refs/heads/master/host/repo_bash_completion)
and copy it to your home directory (e.g., under ~/etc). Then, add the following
line to your `~/.bashrc`:

```none
[ -f "$HOME/etc/repo_bash_completion" ] && . "$HOME/etc/repo_bash_completion"
```

### How to configure repo to sync private repositories

Create a file called .repo/local_manifest.xml and add your private project into
it.

```none
<?xml version="1.0" encoding="UTF-8"?>
<manifest>
  <remote  name   = "private"
           fetch  = "ssh://gitrw.chromium.org"
           review = "http://review.chromium.org" />
  <project path   = "src/thirdparty/location"
           name   = "nameofgitrepo"
           remote = "private" />
</manifest>
```

If you want to pull in from a different git server you will need to add a
different remote for the project. Please type `repo manifest -o tmp.xml` to dump
your current manifest to see an example. More documentation on the manifest file
format is available on the [repo Manifest format
docs](http://android.git.kernel.org/?p=tools/repo.git;a=blob;f=docs/manifest_xml.txt;hb=HEAD).

### How to make `repo sync` less disruptive by running the stages separately

Just running `repo sync` does both the network part (fetching updates for each
repository) and the local part (attempting to merge those updates in to your
tree) in one go. This has a couple of drawbacks. Firstly, because both stages
are mixed up, you need to avoid making changes to your tree for the whole time,
including during the slow network part. Secondly, errors telling you which
repositories weren't updated because of your local changes get lost among the
many messages about the update fetching.

To alleviate these problems, you can run the two stages separately using the
`-n` (network) and `-l` (local) switches. First, run `repo sync -n`. This
performs the network stage, fetching updates for each of the repositories in
your tree, but not applying them. While it's running you can continue working as
usual. Once it's finished and you've come to a convenient point in your work,
run `repo sync -l`. This will perform the local stage, actually checking out the
updated code in each repository where it doesn't conflict with your changes. It
generally only prints output when such a conflict occurs, which makes it much
easier to see which repositories you need to manually update. You can run it
multiple times until all the conflicts are fixed.

### How to find a list of branches that I've created with repo

See the `repo branches` command.

---

## How to get more commands available on the release image

If you're using the shell in a Chromium OS device (requires developer mode, or a
system where you've set a custom chronos password), you may find that you're
missing commands you wish you had. Try putting `busybox` on a USB key or an SD
Card. I formatted by USB key with `ext3` and named it `utils` (and that is
assumed in these instructions) using the Disk Utility that comes with Ubuntu (go
to the System menu at the top of the screen, then Administration). Since busybox
is built as part of a normal build (it's used in the factory image), you can
copy it to your USB key like this (run from outside the chroot). NOTE: this
assumes that you've followed all the instructions to build a custom Chromium OS
image from the [Chromium OS Developer Guide](/chromium-os/developer-guide):

```none
sudo chown root.root /media/utils
sudo chmod o+rx /media/utils
sudo cp ~/chromiumos/chroot/build/${BOARD}/bin/busybox /media/utils/
sudo chmod o+rx /media/utils/busybox
```

Then, you can go crazy and add symlinks for whatever your favorite busybox
commands are:

```none
for cmd in less vi zip zcat; do
  sudo ln -s busybox /media/utils/$cmd
done
```

Unmount your USB key:

```none
sudo umount /media/utils
```

Plug your USB key into your Chromium OS device. Make sure that the browser has
started (i.e., you're not still on the login screen) so that Chromium OS will
mount your USB key to `/media/utils`. You can get access to your commands with:

```none
sudo mount -o remount,exec /media/utils
export PATH="$PATH:/media/utils"
```

If you want a list of all of the commands that busybox supports, you can just
run busybox to get a list. If you want to run a busybox command without making a
symlink, just type busybox first. Like: busybox netstat.

**SIDE NOTE**: If you didn't build Chromium OS, it's possible that you can run a
busybox binary obtained from some other source. You'll have the best luck if the
busybox that you get was statically linked.

---

## How to modify my prompt to tell me which git branch I'm in

Add the following to your `~/.bash_profile`:

```none
export PS1='\h:\W$(__git_ps1 "(%s)") \u\$ '
```

---

## How to search the code quickly

When you [installed depot_tools](/developers/how-tos/install-depot-tools), you
got a tool called `git-gs` that's useful for searching your code. According to
the [depo_tools info page](/developers/how-tos/depottools), `git-gs` is a
"Wrapper for git grep with relevant source types". That means it should be the
fastest way to search through your code.

You can use it to search the `git` project associated with the current directory
by doing:

```none
git gs ${SEARCH_STRING}
```

If you want to search all git projects in the `repo` project associated with the
current directory:

```none
repo forall -c git gs ${SEARCH_STRING}
```

On Goobuntu, you can install
[Silversearcher](https://github.com/ggreer/the_silver_searcher), which is a
faster and smarter version of ack/grep. Ag is very fast and works independent of
git repositories, so you can do queries from your top-level folder.

```none
sudo apt-get install silversearcher-ag
# For keyword based search 
ag "keyword" .
# To ignore certain paths
ag --ignore chroot/ "keyword" .
# ag has the same syntax as grep. For example, excluding files that match a pattern - 
ag -L "keyword" .
```

---

## How to refer to bugs concisely

If you have a bug number and want to send it to someone else in an email, you
can use the "crbug" shortcut. So, to refer to bug 1234, you'd refer to:

<https://crbug.com/1234>

---

## How to wipe the stateful partition

You can use 'crossystem clear_tpm_owner_request=1; reboot'. You can also reflash
the DUT with with the --clobber-stateful flag enabled. e.g. 'cros flash
myDUTHostName --clobber-stateful myboardname/latest'

---

## How to enable a local user account

This is not to be confused with how to [enable the chronos
account](/chromium-os/developer-guide#TOC-Set-the-chronos-user-password). This
set of instructions allows you to login to the browser as something other than
guest without having any network connectivity. **Most people don't need to do
this**.

The local user account allows login with no password even if you can not connect
to the Internet. If you are customizing Chromium OS and having trouble logging
in due to your customizations, it may be handy to be able to bypass
authentication and log yourself in as a test user. This is disabled by default
for security reasons, but if you want to enable it for a backdoor user USERNAME,
enter the following from inside the ~/trunk/src/scripts directory:

```none
./enable_localaccount.sh USERNAME
```

---

## How to avoid typing 'test0000' or any password on SSH-ing to your device

Modify your /etc/hosts and ~/.ssh/config to make pinging and ssh-ing to devices
easier.

```none
# Copy testing_rsa and testing_rsa.pub from chromite into your home folder
# for both inside and outside your chroot. 
cp $(CHROMIUMOS-ROOT)/src/scripts/mod_for_test_scripts/ssh_keys/testing_rsa* ~/.ssh  # Required for both inside and outside chroot
chmod 0600 ~/.ssh/testing_rsa # ssh will ignore the private key if the permissions are too wide. 
# Add the below to your ~/.ssh/config
Host dut
  HostName dut
  User root
  CheckHostIP no
  StrictHostKeyChecking no
  IdentityFile ~/.ssh/testing_rsa
  ControlMaster auto
  ControlPersist 3600
# Do this for any lab/test machine
Host 100.*
  User root
  IdentityFile ~/.ssh/testing_rsa
# This assumes you have the following entry
# in your /etc/hosts 
100.107.2.189 dut # Replace with your DUT IP and chosen name for your DUT
# All of the above enables you to do the below 
# from inside and outside of your chroot 
$ ping dut
$ ssh  dut # No need to specify root@ or provide password
```

---

## Making sudo a little more permissive

**If you are at Google, you will instead need to follow the instructions on the
[internal workstation
notes](https://g3doc.corp.google.com/company/teams/chromeos/sites/resources/ubuntu-workstation-notes.md#configuring-etcsudoers)
page for finishing the sudoers setup**.

To set up the Chrome OS build environment, you should turn off the tty_tickets
option for sudo, because it is not compatible with `cros_sdk`. The following
instructions show how:

```none
cd /tmp
cat > ./sudo_editor <<EOF
#!/bin/sh
echo Defaults \!tty_tickets > \$1          # Entering your password in one shell affects all shells 
echo Defaults timestamp_timeout=180 >> \$1 # Time between re-requesting your password, in minutes
EOF
chmod +x ./sudo_editor 
sudo EDITOR=./sudo_editor visudo -f /etc/sudoers.d/relax_requirements
```

Note: See the [sudoers man page](http://www.gratisoft.us/sudo/sudoers.man.html)
for full detail on the options available in sudoers.

Note: depending on your Linux distribution's configuration, sudoers.d may not be
the correct directory, you may check /etc/sudoers for an #includedir directive
to see what the actual directory is.

---

## How to share files for inside and outside chroot

The `cros_sdk` command supports mounting additional directories into your chroot
environment. This can be used to share editor configurations, a directory full
of recovery images, etc.

You can create a `src/scripts/.local_mounts` file listing all the directories
(outside of the chroot) that you'd like to access inside the chroot. For
example:

```none
# source(path outside chroot) destination(path inside chroot)
/usr/share/vim/google
/home/YOURID/Downloads /Downloads
```

Each line of `.local_mounts` refers to a directory you'd like to mount, and
where you'd like it mounted. If there is only one path name, it will be used as
both the source and the destination directory. If there are two paths listed on
one line, the first is considered to be the path `OUTSIDE` the chroot and the
second will be the path `INSIDE` the chroot. The source directory must exist;
otherwise, `cros_sdk` will give off an ugly python error and fail to enter the
chroot.
Note: For security and safety reasons, all directories mounted via
`.local_mounts` will be read-only.

---

## How to create a cros_workon repository

(TODO: Under construction by dianders, 2011-01-10)

(TODO: some of this probably belongs in a "background" section).

#### First: A brief review of Chromium OS software package management...

Chromium OS uses [Gentoo](http://www.gentoo.org/)
[Portage](http://www.gentoo.org/proj/en/portage) for software package
management. Gentoo maintains an official list of available software packages,
called the [Portage tree](http://packages.gentoo.org/). All packages in this
tree are available to the Chromium OS developer... but they are not all
guaranteed to work with - or even compile for - Chromium OS.

The Portage tree is a two-level hierarchy. The top level is a collection of
categories (e.g., `sys-apps` or `dev-python`), with each category having a list
of associated packages (e.g., `sys-apps/upstart` or `dev-python/django`).

Each package has a corresponding *ebuild* file that contains all information to
maintain the package. There are some ebuilds that install binary packages, or
just specify dependencies, such as the Chromium OS top-level 'meta-ebuild':
`chromeos/chromeos`. However, most ebuilds specify (1) how to obtain, configure,
and build the package's source code, (2) how and where to install the
executables, and (3) other packages upon which the package depends, both at
compile-time and at run-time. Portage also uses ebuild files to handle package
options, package versioning and for which computer architectures (`arm`, `x86`,
`amd64, mips`, etc.) a package is build-able and stable:

*   Package options are handled by so-called *USE flags*, that can be
            passed to an ebuild to turn on or off various package options (i.e.
            'build this package *using* this option'). These options can cause a
            package to depend on additional packages, or change how the source
            code is compiled.
*   The version number of the package built by a particular ebuild is
            always explicit in the ebuild file's name (e.g.,
            `net-wireless/wpa_supplicant/wpa_supplicant-0.7.2-r39.ebuild`). The
            ebuild version number almost always directly corresponds to the
            version number of the corresponding upstream package.
*   Most ebuilds have a `KEYWORDS` field, which lists the architectures
            for which the package can be *built*, and for which the package is
            considered *stable*. In Portage, an unstable architecture is marked
            with a tilde ('~'). For example, `KEYWORDS="~arm x86"` means *stable
            for x86, unstable for arm*.

The tool `emerge` is the used to build a portage package. The first task for
`emerge` is to search the Portage tree to determine which ebuild to use, for a
given package. For a single package, there are often several ebuilds in the
tree, each with a different version number. Using target-specific portage
settings found in `make.conf` and `package.keywords`, `emerge` examines the
ebuilds' version numbers and their `KEYWORDS` fields to select an appropriate
ebuild. In some cases, there is no appropriate ebuild, and `emerge` will fail
with an error. In general, `emerge` will select the ebuild with the highest
version number that is stable for the chosen architecture.

After selecting an ebuild for the package, `emerge` computes a dependency graph
containing the package's explicit dependencies and their dependencies, etc.
Assuming it can find matching ebuilds for all of these prerequisite packages,
emerge will then start building them all in parallel.

In most cases a source code archive for a package is retrieved from its official
'upstream' maintainers (for example, `x11-base/xorg-server` retrieves its source
directly from `anongit.freedesktop.org`). The source code archive retrieved for
a particular versioned ebuild is always of a fixed version. However, the Gentoo
package maintainers often find a need to modify the original upstream source
code, for example, to apply critical security fixes, or to fix
architecture-specific or cross-compilation issues. In these cases, the ebuild
will apply a series of patches to the retrieved upstream source code before
building it. These patches are usually stored in a package specific 'files/'
subdirectory. Eventually (hopefully), upstream will accept these patches (or
otherwise resolve the original issues), and the Gentoo package maintainers will
create a new ebuild that retrieves a newer source code archive, applying new
patches to fix any regressions, and so on...

For Chromium OS we always try to use unmodified packages from the same version
of the Portage tree. In the Chromium OS source tree, the local copy of this
version Portage tree is at `src/third_party/portage`. However, it is not always
possible to use only these packages for the following reasons:

1.  Like the main Gentoo maintainers, the Chromium OS team will often
            need to maintain a collection of patches to further modify upstream
            source code to fix issues or add features before they are available
            in the main Portage tree (or even, often, before they are available
            in the original upstream source code).
2.  Similarly, sometimes the package's source code is correct, but the
            ebuild itself doesn't quite work as required.
3.  Our version of the Portage tree is relatively constant and does not
            track the daily changes to the upstream Portage tree. Often, we need
            a newer version of a package that only exists in a newer version of
            the Portage tree.
4.  There are some software components of Chromium OS that are Chromium
            OS specific, and don't have a corresponding package in the Portage
            tree.
5.  Chromium OS runs on many different hardware platforms (also referred
            to as *boards*), some that require board specific software packages.

To address these issues, we use *overlays* to modify and extend the base Portage
tree. The most important overlay is the Chromium OS overlay, located at
`src/third_party/chromiumos-overlay`. In addition, each of the different
supported hardware platforms has its own board-specific overlay (e.g.
`src/overlays/overlay-x86-generic`). Lastly, some vendors may want to add
additional packages to a private overlay (in `src/private-overlays`) to further
customize a Chromium OS image for a particular vendor-specific board.

Thus, packages (and their ebuilds) can be located in any of the following
places:

1.  `src/third_party/portage/` - the Portage tree
2.  src/third_party/chromiumos-overlay/ - the Chromium OS overlay
3.  src/overlays/overlay-${BOARD}/ - a board specific overlays
4.  src/private-overlays/${PRIVATE-OVERLAY}/ - a vendor-board specific
            private overlay

Taken together, the overlays contain:

1.  Packages from the Chromium OS version of the Portage tree
2.  New, unmodified, upstream portage packages that don't exist in our
            version of the Portage tree.
3.  Upstream portage packages with modified ebuilds, some of which also
            include Chromium OS-specific source code patches.
4.  Chromium OS-specific packages. In particular, all of the packages in
            the `chromeos/` and `chromeos-base/` categories. These packages
            mostly retrieve their source code archives from the Chromium OS git
            server (git.chromium.org). (TODO: verify)
5.  Packages that were originally based on upstream portage packages,
            but which have diverged significantly from the original source over
            time. Instead of maintaining an enormous ever growing set of
            unwieldy patches, these packages retrieve their source code archives
            from the Chromium OS git server (git.chromium.org). (TODO: verify)

#### So, this was supposed to be a FAQ about cros_workon...

Right, so remember those ebuilds that fetch source code archives from
git.chromium.org?

Well, those ebuilds invariably inherit from the "cros-workon" eclass. This adds
some additional functionality to the ebuild.

(TODO: what is that additional functionality)

In particular, the cros_workon ebuild will fetch a specific version.

(TODO: from where exactly does it fetch the source repo/branch/SHA-1)

Well, it is also possible to configure the build system to fetch the code from
the local filesystem instead of the release branch of git.chromium.org.

(TODO: how does this work)

(TODO: what are -9999 ebuilds)

(TODO: The actually instructions for creating a new **cros_workon-able**
package)

---

## How to avoid generating a new password or gitcookies for each push/upload

If you get the following message very often while trying to do a 'git push' or
'repo upload', it is likely that the ~/.netrc authentication is not sufficient
for you.

```none
Username for 'https://chromium.googlesource.com': fooPassword for 'https://foo@chromium.googlesource.com': fatal: remote error: Invalid authentication credentials.Please generate a new identifier:  https://chromium.googlesource.com/new-password
```

More details on this
[here](https://sites.google.com/a/google.com/android/development/migrating-from-netrc-to-corpsso).

```none
$ sudo apt-get install git-remote-sso$ git config --global --add credential.helper corpsso
```

---

## How to symbolize a crash dump that I got running on my device?

There is a nifty little script in your chroot, under trunk/src/scripts called
cros_show_stacks. The script takes in the parameters: board name, IP of the
remote device you want to fetch and symbolize crashes from,

and a path to the debug symbols. If the path is not specified, it picks it up
from your local build correctly (/build/$BOARD/usr/lib/debug/breakpad)

```none
# Typical usage. Note that /Downloads below refers to the Downloads folder outside the chroot, the shared folder mapping is specified in trunk/src/scripts/.local_mount. #./cros_show_stacks --board=eve --remote $DUT_IP --breakpad_root /Downloads/eve_debug_syms/debug/breakpad/
```

---

## How to free up space after lots of builds?

Use \`cros clean\` outside the chroot. This should take care of purging your
.cache, builds downloaded from devserver etc.

---

## How to fetch patches/changes from a Gerrit CL?

Very often, you may want to fetch changes from someone else's or your own CL
uploaded for code-review on[
chromium-review.googlesource.com](http://chromium-review.googlesource.com). The
following function in your bashrc may be useful. Replace cros with whatever
remote name is listed in your .git/config. The function takes one argument, the
commit SHA which you can find right below 'Author' and 'Committer' information
on the review page. For example, for
<https://chromium-review.googlesource.com/#/c/344778/>, you would use 'apply-cl
2a22b9dc94c9c13dbff0a1c397f49cb6456f4f2c'. Make sure to be cd-ed into the
correct directory for that git repository.

```none
# Google Gerrit specific # Takes a commit ID of a patch in Gerrit, fetches it# and applies it# Sample Usage: apply-cl 2a22b9dc94c9c13dbff0a1c397f49cb6456f4f2capply-cl() {   ref=`git ls-remote cros | grep $1 | cut -f2`;   git fetch cros $ref;   git cherry-pick FETCH_HEAD;}
```

This works because the CLs are stored on the same remote git server as where all
the branches and refs reside. You can switch to a branch that has its HEAD
pointing to a CL, based on just the CL id as follows.

```none
# Continuing the previous example, to fetch https://chromium-review.googlesource.com/#/c/344778/ 
# in the 3.18 kernel repo. 
```

```none
$ git ls-remote cros | grep 344778
720a16e3533d17ea73447788fa11db7908accd6f	refs/changes/61/319961/8
5ef1aeb982d60f0071070730e3f21cf542e2c11c	refs/changes/78/344778/1
2a22b9dc94c9c13dbff0a1c397f49cb6456f4f2c	refs/changes/78/344778/2
85130b96bc71b48dbba5ab8eec8e35717af234fa	refs/changes/78/344778/meta
```

```none
$ git co -b test-branch
$ git fetch cros refs/changes/78/344778/2
$ git reset --hard FETCH_HEAD
# Remember to set the tracking branch to something meaningful, else git gets confused. 
# In this case, the default tracking branch is the main kernel branch. 
$ git branch -u cros/chromeos-3.18
```

---

## How to split my change into two (or any number of) separate changelists for upload

Usually when submitting your changes into a revision control system, it is
considered best practice to break your submissions into the smallest possible
logical chunks and then submit one changelist per chunk. Each chunk should work
fine without future chunks, but may depend on previous chunks. For instance, you
may be able to submit your change to add a new API in a separate changelist from
your change that uses the API.

This has several advantages:

*   It documents which parts of your change are related to what goal.
*   It makes it easy for someone to revert part of your change without
            reverting the whole thing (if there are problems).
*   It makes it easy for someone to cherry-pick your changes later.

Note that most developers will still develop their entire change at once, then
only break things apart during submission.

It turns out that `repo` and `git-cl` are not really optimized for this
workflow. Why?

*   The git-cl command always munges all of the changes in your branch
            into one big changelist. It does not easily allow you to create
            multiple changelists from a branch.
*   The repo command always creates branches from the mainline (AKA the
            remote master). This makes it impossible to use repo to create
            branches that depend on one another.

...but, luckily, you can work around that by using git directly.

TODO: This is probably not the perfectly ideal way to do things, but will work.
Can someone optimize?

TODO: I haven't actually tested all of these steps. They are based on an email
plus my (poor) understanding of git. Can someone test, then remove this TODO?

### Do all your work in one big branch

I'll assume that you initially did all of your work in one big branch. AKA, you
started the branch like this:

```none
repo start ${BRANCH_NAME} ${CROS_WORKON_PROJECT}
```

...and then made a bunch of changes.

### Create a new branch for your first submission

To keep things simple, we're going to leave our first branch alone and create
new branches for our submission. We'd like to eventually get to a structure that
looks like this (if we wanted to do N separate uploads, each of which was
dependent on the previous ones):

master

\\_____${BRANCH_NAME}_A

\\_______________${BRANCH_NAME}_B

...so the first step is to create "${BRANCH_NAME}_A". We can use `repo start` to
do this one. Remember, that I'm assuming `${CROS_WORKON_PROJECT}` is the name of
your project.

<pre><code>repo start ${BRANCH_NAME}<b>_A</b> ${CROS_WORKON_PROJECT}
</code></pre>

The rest of the instructions are going to assume that you've changed into the
directory that your project lives. If you want to use repo to help you find it,
you can do:

```none
cd `repo forall ${CROS_WORKON_PROJECT} -c 'pwd'`
```

### Get the relevant changes into branch A

There are probably better ways to this this, but one way to get the changes from
your big branch into branch A is to use `git cherry-pick`. First, use `git log`
to figure out what changes you want:

```none
git log ${BRANCH_NAME}
```

For instance, in my example, I see this (using `--format=short`):

```none
commit d7b250822f98bda19f62072555beb66602bed29d
Author: Me <me@...>
    Change 4
commit 0c577cee881471d6509c91eba8eeeb0c8bec1551
Author: Me <me@...>
    Change 3
commit 0a7285d2d09159e540fb89f086f58a457b5f583f
Author: Me <me@...>
    Change 2
commit dd1ee8d9a9d3148cf4403c339ab18f094304d2fa
Author: Me <me@...>
    Change 1
```

Now, use `git cherry-pick` to pick the changes you want. ...so if I wanted
change #1 and #3, I could do:

```none
git cherry-pick dd1ee8d9a9d3148cf4403c339ab18f094304d2fa
git cherry-pick 0c577cee881471d6509c91eba8eeeb0c8bec1551 
```

### Create branch B

Now, you'll want to create branch B as a subbranch of branch A. You can do that
like:

```none
git branch ${BRANCH_NAME}_B ${BRANCH_NAME}_A
git checkout ${BRANCH_NAME}_B
```

### Get the relevant changes into branch B

We'll use the same cherry-picking technique to get things into branch B. Let's
imagine that I want change #2 and #4:

```none
git cherry-pick 0a7285d2d09159e540fb89f086f58a457b5f583f
git cherry-pick d7b250822f98bda19f62072555beb66602bed29d 
```

### Double-check that you've got everything

At this point, your branch B should have everything that was in the original
branch. There's a `git diff` command you can do to verify that:

```none
git diff ${BRANCH_NAME}..${BRANCH_NAME}_B
```

### Upload for code review

To upload the first part for code review, just do:

```none
git checkout ${BRANCH_NAME}_A
git cl upload
```

To upload the second part for code review, just do:

```none
git checkout ${BRANCH_NAME}_B
git cl upload ${BRANCH_NAME}_A  # sends diff from ${BRANCH_NAME}_A to ${BRANCH_NAME}_B
```

It is ***very important*** in this case that you mention in your second upload
that this change depends on the first one. You should actually include the URL
pointing to the first code review in the changelist comments of the second
change. If you don't do this, your reviewers will be very confused. **Even
better** is if you can actually wait to start the code review for branch B until
after the code for branch A has been committed.

### Make changes, re-upload

To upload additional changes to branch A (the first changelist), you can do:

```none
git checkout ${BRANCH_NAME}_A
# ... make changes ...
git cl upload
git checkout ${BRANCH_NAME}_B
git rebase ${BRANCH_NAME}_A     # Incorporate branch A's changes into branch B
```

To upload additional changes to branch B, you can do:

```none
git checkout ${BRANCH_NAME}_B
# ... make changes ...
git cl upload ${BRANCH_NAME}_A  # sends diff from ${BRANCH_NAME}_A to ${BRANCH_NAME}_B
```

### Push

When you're ready to push branch A, it's pretty easy:

```none
git checkout ${BRANCH_NAME}_A
git cl push
```

To push branch B (which you can't do until after you've pushed branch A), you
need to rebase it to master. TODO: Can someone confirm that this is the right
set of steps for Chromium OS?

```none
git checkout cros/master
git pull
git checkout ${BRANCH_NAME}_B
git rebase --onto cros/master ${BRANCH_NAME}_A
git cl push
```

---

## How to get a fully functional vim on a test device

By default there is a very minimalist build of vim included on test images. To
replace it with a fully functional copy of vim do the following:

```none
export BOARD=eve # Replace with the board name of your test device
export TESTHOST=127.0.0.1 # Replace with the hostname or ip address of the test device
export JOBS=$(nproc)
emerge-${BOARD} --noreplace --usepkg -j${JOBS} app-editors/vim-core app-vim/gentoo-syntax
USE=-minimal emerge-${BOARD} -j${JOBS} app-editors/vim
cros deploy "${TESTHOST}" app-vim/gentoo-syntax
cros deploy "${TESTHOST}" app-editors/vim-core
cros deploy "${TESTHOST}" app-editors/vim
```
