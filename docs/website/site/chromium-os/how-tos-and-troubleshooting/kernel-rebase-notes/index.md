---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/how-tos-and-troubleshooting
  - How Tos and Troubleshooting
page_name: kernel-rebase-notes
title: Kernel rebase notes
---

## Overview

### Flow

A kernel rebase consists of moving sets (ie, topics) of patches from one version
to a later version.

The flow is:

*   Separate current patches into topic branches
    *   git cherry-pick individual patches to common branches
*   Rebase patches from old kernel version to new kernel version
    *   git rebase $new_version
*   Merge new topics together on the new kernel version
    *   git merge $topics

This flow is iterative. It can take a long time to cherry-pick all the changes
into topics, then rebase, then merge. In that time, more changes land which
require a second or even third iteration of this sequence.

### Estimate how long the rebase effort will take

It took me 3 months to sort through about 5644 3.14 changes and rebase
appropriately to 3.18. And that was with help. So I would estimate roughly 434
per week.

Really, ramping up on my git knowledge and settling on a set of useful tools
took a significant amount of time, too.

### Tools

Various git commands will be particularly important during the rebase. gitk is a
graphic tool that some folks use, but I used just the git command itself.

```none
git log --oneline --first-parent --reverse v3.14..m/master
```

This shows all the commits in our tree on top of v3.14.

--oneline shows just one line per commit: commit hash plus the one line summary.

--first-parent shows merge changes, but it excludes merge lineage (ie, all the
changes that were actually being merged in). Merges are rare after the rebase
and are typically handled on a case by case basis.

--reverse shows the changes in the order they occur (oldest to newest) rather
than the default order (newest to oldest).

These parameters are the most important ones in my rebase (to 3.18).

Pipe to 'wc -l' shows how many commits you are dealing with.

Redirect to a file, say "gitlog.v3.14..", lets me examine the list in 'vi'.

```none
git show $COMMIT --oneline --name-only
```

--name-only shows the touched files. This is important in determining which
topic a change should be cherry-picked into.

```none
set keywordprg=git\ show
```

in my ~/.vimrc file allows me to 'vi gitlog.v3.14..', position the cursor on the
change_id, then "K" (the keyword program command in 'vi') runs 'git show' on
that change_id.

This is useful for examining change details within the list of changes in which
it appears.

```none
git rebase --onto master next topic
```

Rebase all changes between next and topic onto master, and move the current
branch label (usually topic) to the resulting HEAD.

There were a number of times I decided to replay some subset of changes to a new
head, and this is the way to do it. I highly recommending closely examining 'git
rebase --help', first.

It will basically say it changes this:

```none
               o---o---o---o---o  master
                    \
                     o---o---o---o---o  next
                                      \
                                       o---o---o  topic
```

into this:

```none
               o---o---o---o---o  master
                   |            \
                   |             o'--o'--o'  topic
                    \
                     o---o---o---o---o  next
```

Note that the master and next labels remain unchanged, but topic moves to the
rebased HEAD.

Did you screw up an important branch label with a bad rebase?

```none
git reflog
git reset --hard $change_ID
```

The reflog shows recent HEAD changes and the commands that changed them. The
first column is the resulting change_ID.

The reset changes the label to the specified change_ID. --hard restores the
files to that version.

[TOC]

Source organization

### **Topic Branches**

The key to sanity when doing this is to do good topic branch split-ups. This
reduces the scope of each branch to only cover a specific part of the kernel, or
a specific set of functionality. ***Multiple,smaller topic branches allow faster
iterations.*** The following branches are in use for v3.0 and v3.2 rebase:

> <table>
> <tr>
> <td> chromeos-base-&lt;version&gt;</td>
> <td>This branch contains more or less all base changes we need to run ChromeOS on an x86 system: EFI, ACPI changes, build infrastructure, configs, and some of the chromeos-specific drivers we have picked up (should be very few left)</td>
> </tr>
> <tr>
> <td>chromeos-misc-&lt;version&gt;</td>
> <td>Kitchen sink for backported patches and misc other changes. It can sometimes be hard to decide what goes here vs in -base.</td>
> </tr>
> <tr>
> <td> chromeos-gobi-&lt;version&gt;</td>
> <td>Our driver changes for qcserial and gobi. Given that gobi drivers have been rejected upstream we might have to carry this for the long run.</td>
> </tr>
> <tr>
> <td> chromeos-verity-&lt;version&gt;</td>
> <td> Verity device mapper module. This branch should hopefully go away once the patches have been accepted upstream.</td>
> </tr>
> <tr>
> <td> chromeos-tegra-&lt;version&gt;</td>
> <td>All the Nvidia ARM Tegra changes. Quite a stack of patches that are slowly going upstream.</td>
> </tr>
> <tr>
> <td> chromeos-input-&lt;version&gt;</td>
> <td> Mouse/Touch/trackpad and related subsystem drivers (e.g. multi-touch support).</td>
> </tr>
> </table>

Update from my 3.14-3.18 experience:

> <table>
> <tr>
> <td> chromeos-base-&lt;version&gt;</td>
> <td> chromeos/config and chromeos/scripts changes</td>
> </tr>
> <tr>
> <td> chromeos-misc-&lt;version&gt;</td>
> <td> Miscellaneous software related patches: futex, ftrace, time, genalloc, FS, etc.</td>
> </tr>
> <tr>
> <td> chromeos-platform-&lt;version&gt;</td>
> <td> Similar to misc, but more hardware specific. power, suspend, irq, NAND, MMC, MTD, ACPI, IIO, etc.</td>
> </tr>
> <tr>
> <td> chromeos-drm-&lt;version&gt;</td>
> <td> drivers/gpu/drm and related changes. For 3.18, we dropped all 3.14 patches, and</td>
> <td>Stephane provided a new set for 3.18.</td>
> </tr>
> </table>

Add new topic branches where there is active development. e.g. we might add
chromeos-wifi-&lt;version&gt; for wifi drivers since they have plenty of
backports.

### kernel and kernel-next Repos

Today we have two kernel repos: kernel and kernel-next. The historical reason is
repo tools don't allow to track two branches of one git repo in two locations in
the directory structure. We needed two different base versions for quite a while
-- one for x86 and one for ARM. We have since merged the bases, but
"kernel-next" tree is still around.

The overview of the steps I work with the repos is:

1.  Do the original topic-branch split-up in the kernel.git repo since
            the patches reside on the main branch.
2.  Move the topic branches to the kernel-next repo and do all the
            rebase work in kernel-next repo.
3.  Merge the topic branches together to a combined branch and ask
            others to test the combined branch in kernel-next repo.
4.  Warn "Testers" the topic-branches will get rebased and to NOT keep
            significant work on top of kernel-next combined branch.

## Rebase process

### **Step 1: Add upstream mainline/stable trees**

```none
git remote add mainline git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
git remote add linux-stable git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
git remote update mainline
git remote update linux-stable
```

This makes "interesting" tags available from upstream sources. This step can be
done at any time before we need to use tags in upstream repos.

### **Step 2: Set up Topic branches**

Splitting the tree up into topic branches is something that you rarely have to
do from scratch. This document will assume those exist.

Maintain the topic branches over time, syncing up from the main tracking branch
every now and then and pushing them to the server. *Others should NOT base their
work off of the topic branches.*
'git-new-workdir' checks out a specific branch into a new directory but uses the
same backing .git object storage. This allows us to check out each topic branch
into one directory per topic and we won't have to sync up the .git contents when
done.
**WARNING: NEVER use git-new-workdir for a branch that you have checked out
somewhere else, it is a recipe for disaster.** In other words, delete the
"split" directory (and it's children) created below before checking out any of
the topic branches in some other location.

Under src/third_party/kernel/ , build the "topic branches":

```none
cd src/third_party/kernel
mkdir split
cd split
for d in base gobi misc verity tegra ; do
/usr/share/git/contrib/workdir/git-new-workdir  ../../../../.repo/projects/src/third_party/kernel/files.git ${d} cros/chromeos-${d}-3.0
cd ${d}
git checkout -b chromeos-$d-3.0 cros/chromeos-$d-3.0
cd ..
done
```

In case not all the topic branches are based on the same tree, run:

`git rebase --onto v3.0 v3.0.3 HEAD`

in order to rebase all changes in "HEAD" from v3.0.3 back to v3.0.
Then open one terminal window for each topic branch and two extra windows:

*   add `alias gp="git cherry-pick"` to ~/.bash_profile in the chroot
            (used below to shorten the command lines)
*   In the first 5 windows, cd into one of the 5 topic branch
            directories.
*   In the 6th window, to run/view git log command (listed below)
*   Use the 7th for misc git commands to determine where conflicts are
            coming from, etc.

In the 6th window find the "sync point" (last commit that was cherry picked)
with:

```none
git log --oneline cros/chromeos-3.0
```

and search for "Merge". The "sync point" commit should look something like:

e8f848b Merge branches 'chromeos-base-3.0', 'chromeos-misc-3.0',
'chromeos-verity-3.0', 'chromeos-gobi-3.0' and 'chromeos-tegra-3.0' into
chromeos-3.0

One can also look at "git log" for each of the topic branches to find out the
last cherry-picks into each of those and confirm the SHA1 matches.

### Step 3: resync Topic Branches

Next, produce a list of commits that need to be cherry picked from
cros/chromeos-3.0 into the topic branches:

<pre><code>git log --date=short --pretty=format:"gp %h # %cd %s" <i>lastsyncpoint</i>..cros/chromeos-3.0 | tac
</code></pre>

Redirect output to a file or pipe into less if it's long. In the order listed,
copy and paste each line of the output into one "corresponding" topic branch
window. The "gp" alias will cherry-pick that commit, and everything after the
'#' will be ignored. Merge conflicts may occur if "gp" was accidentally pasted
in the wrong window OR the change doesn't belong in that topic branch. Either
way, use git reset --hard to "undo" the failed cherry-pick in order to retry on
a different topic branch.

\[*Some merge conflicts are likely to occur and require fix up if the original
commit is based on a different release (e.g. v3.0.13) than the topic branch is
based on (e.g. v.3.0). If topic branches were based on the same stable release
(e.g. v3.0.13) at this point, such conflicts wouldn't occur.*\]

### Step 4: sanity-check Topic Branches

Here's how to make sure the local topic branches contain all commits (assumes
chromeos-3.0 is based on v3.0.13):

```none
git checkout -b mergetest v3.0.13
git merge chromeos-base-3.0
git merge chromeos-misc-3.0
git merge chromeos-verity-3.0
git merge chromeos-gobi-3.0
git merge chromeos-tegra-3.0
```

Fix up (using $EDITOR), then `git add` and `git commit` (to mergetest branch)
conflicts between each merge.
\[Digression: Unfortunately, combining the topic branches into one merge command
failed:

git merge chromeos-{base,misc,verity,gobi,tegra}-3.0

We think this is either a git merge bug or the merge conflicts in our case
prevented the octopus merge from running.\]

Finally, we get to verify with:

```none
git diff cros/chromeos-3.0
```

The diff should be empty. If it isn't, check first if one of the conflicts was
resolved "unfavorably" and run git commit --amend to preserve the update. If
not, something is missing from a topic branch. Hunt it down and add to the topic
branch.

### Step 5: Mark sync in master branch

To indicate in the master branch the points in time where a topic branch sync
occurred, re-merge all the topic branches into the main branch:
**TBD \[ git commands to "re-merge into main branch" \]**
Since the diff in Step 4 was empty, this results in an empty merge changeset
that will be used as a marker for the next time one has to "resync". This is the
same "mark" (searching for "Merge") was used in Step 2.

### **Step 6: Create new Topic-Branches**

This is straight forward:

```none
for b in base misc gobi verity tegra ; do
git checkout -b chromeos-${b}-3.2 chromeos-${b}-3.0
done
```

No need to use the git-new-workdir.

### **Step 7: Rebase -base Topic branch first**

Rebase sounds simple. But it can get really really messy in some cases. Take it
easy, be ready to throw away your work a couple of times and take plenty of
notes. Do one branch at a time.

We start with the -base branch since it contains the build infrastructure that's
needed to build the kernel as an ebuild. This allows x86 builds/testing
immediately. After chromeos-base-3.2 is done, boot that on an x86 system to
confirm. Then rebase each branch, one-by-one and rebuild the kernel after each
branch is done.

So, then in your main work dir:

```none
git checkout chromeos-base-3.2
git rebase v3.2 
```

git will stop on conflicts, you resolve them, git add (don't commit), and git
rebase --continue. If conflicts get ugly, see "Handling Rebase Conflicts" advice
below. When the rebase is done, do NOT commit until the result compiles. It's
likely some kernel internal API changed and a merge was still referring to the
old API. (e.g. v3.0 -&gt; v3.2 had INIT_GROUP_RWSEM() change to
INIT_THREADGROUP_FORK_LOCK()). One can compile with:

```none
FEATURES="noclean" emerge-$B chromeos-kernel
```

And once that compiles, try it. If the system boots, then run git commit for
each file that was modified. Later, when we run "rebase -i" below, we will
re-order and "squash" or "fixup" each of those changes. Prefix each commit with
something like "REBASE_FIXUP" so it's obvious which commits are fixups that need
to be squashed.

Make sure to document changes made to resolve conflicts AND to fix compile
failures in the commit message. This can be simple as appending a one-liner like
this to the commit message:

`[$WHOAMI: resolved trivial conflicts for 3.2] `
`Signed-off-by: Chromium KernDev <kerndev@chromium.org>`

Once kernel is booting with the given topic branch, run:

```none
git rebase -i v3.2
```

The list of commits on the topic branch will be available to edit. See the
comment at the bottom of that file to understand the options besides the default
"pick".

Additional Cleanups to do while running git rebase command above:

1.  **squash chromeos/config commits**: first just reorder commits
            together in git rebase -i commit list, then mark those commits as
            fixup instead of pick.
2.  **Drop reverted commits**: If a change is later on reverted, delete
            both the pick line for the original and revert commits.
3.  **Clean up Commit log entries**: Someone missed the CHROMIUM:
            prefix? add it. Misformatted changelogs? Fix them up a bit. Etc.

Re-run the git rebase command as often as necessary to reduce the number of
patches to a minimum. Add more topic branches if necessary.

#### Handling Rebase Conflicts

As shown above, on the first try, run git rebase &lt;newversion&gt; and see how
ugly the conflicts get. Really lucky folks will get a few conflicts that can be
resolved easily. The rest of us will find conflicting upstream changes.

It's possible conflicts are due to a change appearing in multiple branches.
Check and compare the contents of each topic branch with

```none
git log --oneline v3.2.. 
```

Don't understand a conflict? Backported commits get weird. First find the
related commits using one or more of the following:

`git log -S Sym v3.2 -- drivers/net/ # list commits touching "Sym" in v3.2
branch under drivers/net`

git diff HEAD -- drivers/net/r8169.c # compare diff with current branch contents
- NOT a 2-way diff
`git log --stat chromeos-base-3.0 -- drivers/gpu/drm/ # find "big" diffs in
chromeos-base-3.0`

and then look at them with "git show":

For the very bad cases, do the same as during the split-up into topic branches,
run `git log --oneline | tac | ...` on the original branch and copy and paste
each and every change over to have full control over what happens.
In some cases it's easier to apply the contents with:

`git show <changeset> | patch -p1`
`# fix up failed hunks`
`git commit -a -c <original changeset>`

There are probably more clever ways of fixing up conflicts with git. But this
method mostly works.

#### **Debugging Boot**

Since dm-verity isn't entirely upstream, first is to build and install an image
with --noenable_rootfs_verification. This will allow us to update the kernel for
testing chromeos-base-3.2:

``` bash
cros build-image --board=$B test --no-enable-rootfs-verification
```

Once we merge in chromes-verity-v3.2 topic branch, then we can rebuild normally.

Then get the kernel to spew it's verbage to the console. Here's the list the
parameters to modify:

*   quiet : delete this one
*   loglevel=1 : replace with loglevel=7 or debug
*   console=tty2 : replace with console=tty1.
*   Also add console=ttyS0 if serial console is available
            ([servo](/chromium-os/servo) or mini-PCIe serial card)

One can modify the parameters in two places depending on which tools are used to
push kernels:

1.  update_kernel.sh : vi ~/trunk/src/build/images/$B/latest/config.txt
2.  `cros build-image` : vi ~/trunk/src/scripts/build_kernel_image.sh and
            modify where this script writes out parameters to config.txt. Then
            (re)run build_image as shown above.

### Step 8: Push -base Topic Branch

***Before pushing your branch, contact*** ***[Chromium OS
dev](https://groups.google.com/a/chromium.org/group/chromium-os-dev)***
***mailing list to request a push of a kernel.org/linus tag (e.g. v3.2) to
kernel-next and kernel repos AND permissions to push a new branch to kernel-next
and kernel repos. Please include a reference to this page so people understand
what you are asking for.***

Setup and push to chromium.org repo commands:

```none
git remote add kernel-ssh https://chromium.googlesource.com/chromiumos/third_party/kernel.git
git remote add kernel-next-ssh https://chromium.googlesource.com/chromiumos/third_party/kernel-next.git
git push kernel-next-ssh chromeos-base-3.2
```

If the chromeos-base-X.Y branch needs another rebase or drop some
commits/whatever, push the branch again. If you haven't yet, also push the
previous release branches (can be one command if preferred) that have been
sync'd:

```none
git push kernel-next-ssh chromeos-gobi-3.0 
git push kernel-next-ssh chromeos-misc-3.0 
git push kernel-next-ssh chromeos-verity-3.0 
git push kernel-next-ssh chromeos-tegra-3.0
```

### Step 9: Rebase other Topic branches

Topic branches other than -base, depend on -base. So we need to do the rebase
slightly differently. -misc topic branch is used in the examples below but the
process should be the same for any topic branch. Instructions here are
essentially the same as "***Rebase -base Topic branch first***" step.

```none
git checkout chromeos-misc-3.2
git rebase v3.2
```

Fix up merge conflicts. Commit each file touched separately. Don't build (yet).
Then squash fixups/cleanup into original commits with:

```none
git rebase -i v3.2
```

Now build/test/ using a "junk branch" to park commits on:

```none
git checkout -b mergetest chromeos-base-3.2
git merge chromeos-misc-3.2
```

Do the previously described build/boot/fixup/commit cycle until things look good
(enough). Then we need to bring those changes back into the original topic
branch we are working.

```none
git checkout chromeos-misc-3.2
git log mergetest    # in one window
git cherry-pick <mergetest commits> # in another window
```

At this point. it's possible conflicts are due to a change appearing in multiple
branchs. When working on a topic branch (checked out), check/record the contents
of that topic branch with:

```none
git log --oneline v3.2.. 
```

And as usual, squash, drop, or fix up and commit cleanups with "-i" parameter:

```none
git rebase -i v3.2
```

When done, tree builds and boots on a machine, then push the tree to kernel-next
(or kernel) with:

```none
git push kernel-next-ssh chromeos-misc-3.2
```

### **Step 10: Merge Topic Branches**

Merge all the branches together (like you did at the end of the topic branch
splitup), then try compiling and see how much is broken. e.g.:

```none
git checkout -b chromeos-3.2 chromeos-base-3.2
git merge chromeos-misc-3.2
git merge chromeos-gobi-3.2
git merge chromeos-verity-3.2
git merge chromeos-input-3.2
```

Expect to fix ups conflicts/mistakes in earlier steps:

1.  commit each fixed file into the merged-together branch with "FIXUP"
            prefix (so they are easy to find later).
2.  find the topic branch which has the conflicting change:
    `git log -S <symbol> -- <filename> # find candidate commits which touch
    <symbol>`
    `git show <SHA1> # confirm this is the conflict`
3.  Checkout the topic branch with the offending &lt;SHA1&gt;.
4.  cherry pick the FIXUP &lt;SHA1&gt; into that topic branch.
5.  git rebase -i v3.2 and squash the FIXUP into the commit that
            introduced the problem.

The result is less noise in the change log, a cleaner history, and the branch
will be more bisectable.

Then delete (or reset to pre-merge state) the "merge branch" (chromeos-3.2 in
above example) and redo the merge from scratch. If you move the old branch
aside, you can diff the two end results and make sure they are the same.

### Step 11: Test Merged Branch

The very first testing is to see if the system even comes up. It is easier to
work with just one system on your desk. Once that comes up, suspend-resumes and
reboots cleanly, I kick off a handful of labtest runs of bvt and regression.

### Step 12: Change the manifests for kernel-next.git and ask for help

Send out a PSA saying "switching kernel-next over to track chromeos-X.Y" on
chromium-os-dev, and change the internal and external manifests to track the new
branch by default. I normally do this before I ask others to start pitching in
on testing since it makes it easier to make sure you're tracking the right
sources.

Also, this is the time when I start asking various subteams and other team
members to start pitching in and testing the new kernel on whatever hardware
they have. For 3.0, I created a large number of bugs in the bugtracker for
sign-off, which worked quite well. I made those bugs blockers of the root
"rebase to 3.0" bug.

```none
cd ~/trunk/.repo
git branch   # see which branches are available
git checkout default
git pull
git branch update-kernelnext
$EDITOR oldlayout.xml  # update kernel-next version
git diff # review change
git commit  # "kernel-next: move to chromeos-3.2 branch"
git push origin HEAD:refs/for/master
```

### Step 13: Move over to kernel.git and change those manifests

Once testing in kernel-next.git look reasonably stable, the branches are clean
(don't contain too much cruft), the time has come to move the branches back to
the kernel.git repo. Use "git push" to the kernel.git repo (appropriate
permissions needed in gerrit):

```none
git push kernel-ssh chromeos-3.2
```

When the pushed branch looks good, update the manifest to point to the new
branch just like for kernel-next. Send PSAs to chromium-os-dev when before,
during, and after changing the manifest:

```none
cd ~/trunk/.repo/manifests/
# updating public manifest requires SSH access
git remote add manifest-ssh https://chromium.googlesource.com/chromiumos/manifest.git
git remote update
git branch -a  # see which branches are available
git checkout remotes/manifest-ssh/master
git branch update-kernel
git checkout update-kernel
$EDITOR oldlayout.xml  # update kernel version
git diff # review change
git commit  # "kernel: move to chromeos-3.2 branch"
git push manifest-ssh HEAD:refs/for/master
# internal manifest update: git push origin HEAD:refs/for/master
```

The above will create a gerrit change that needs to be reviewed/approved.

(*Reminder: ChromeOS folks will also need to update manifest-internal*)

### Step 14: Wait and debug

Once the new kernel tree is in production, you will surely start hearing about
problems. Have fun! :) Usually it's a smaller number of problems. If things look
really bad, the manifest can always be switched back to the old branch.

## **Important notes**

*   **DOCUMENT YOUR WORK!** This is extremely important. If you have to
            touch up a commit, document it! The way to do it is in the commit
            message, adding your own Signed-off-by with a quick one-line comment
            above. For example:
    commit 462fd39525eb3221d320ff3f0eaaddec1ed28a80

    `Author: Yen Lin <yelin@nvidia.com>`

    `Date: Thu Aug 18 10:14:03 2011 -0700`

    ` CHROMIUM: arm: tegra: reserve lp0_vec, fb and carveout`

    ` The fb, fb2 and carveout memory reserved are from high address memory.`

    ` That means with the current u-boot "mem=384M@0M nvmem=128M@384M
    mem=512M@512M"`

    ` cmdline parameter, there will be a 128MB hole wasted.`

    ` BUG=chrome-os-partner:5501`

    ` TEST=hot-plug HDMI then perform suspend/resume LP0`

    ` Change-Id: I1e8877623e62afa7f1a3ad0dd6938e83db5bd188`

    ` Signed-off-by: Yen Lin <yelin@nvidia.com>`

    ` Reviewed-on: http://gerrit.chromium.org/gerrit/6290`

    ` Reviewed-by: Bryan Freed <bfreed@chromium.org>`

    ` Tested-by: Bryan Freed <bfreed@chromium.org>`

    ` [olofj: split up in common.c change and board change, and cleanup]`

    ` Signed-off-by: Olof Johansson <olofj@chromium.org>`

*   Likewise, just keep a short log of the patches that you drop due to
            conflicts at rebase, so you can come back and revisit them when the
            branch is done.
*   Keep track of which patches didn't apply, so you can tell people
            what work needs to be done. For example, I have traditionally not
            made much of an effort to bring wifi patches forward, since I know
            most of them are backports and Sam and Paul have been doing a good
            job of re-syncing it (they are the ones who know how to run the
            tests, and more importantly, parse the test results, in the first
            place). But keep a log of it, so you know who to ask for help later
            on.
*   During split-up, you might realize that someone has done a change
            that touches several topic branches. In most cases, this is a time
            when it makes sense to split up the commit into several ones.
            Typical case is when someone did a config change as part of a code
            change, or someone changed both ARM board code and a driver as one
            change. Split the commit and fixup the commit message as appropriate
            here. Use your judgement and taste for what is reasonable to do.
*   Compile often during rebase to catch build errors as soon as they
            are introduced as possible. If you miss them, then fix them up,
            commit into a separate change, and then go back and squash that
            change in with the offending commit (documenting the fixup in the
            Signed-off-by trail just like with other edits).
*   Once you move the code over to kernel.git, make sure noone is
            commiting to kernel-next.git. This causes pain if you want to keep
            the two trees in sync since things will merge in different order
            instead of just fast-forward when you merge over, and overall it can
            cause confusion.
*   I normally tell people that I will try to keep the new kernel.git
            checkins in sync with kernel-next.git (i.e. bring them over) once
            the rebase starts. In some cases people have still been submitting
            their code to both trees, which is OK but be careful that you do
            bring everything over since it can be easy to miss things when some
            is in already and some is not.
*   Do the rebase of the topic branches on top of the master mainline
            release (i.e. 3.0), but merge them together on top of the latest
            -stable at the time (3.0.8). This way next rebase will be easier.
            Make sure to split up into topic branches before you merge in a new
            stable release into kernel.git ToT as well (i.e. if you want to
            update that tree to 3.0.10).
