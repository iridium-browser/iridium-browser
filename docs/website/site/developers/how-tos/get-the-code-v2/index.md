---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: get-the-code-v2
title: Get the Code (Deprecated)
---

[TOC]

## *This page is obsolete. Please see [Get the Code: Checkout, Build, & Run Chromium](/developers/how-tos/get-the-code) instead.*

**Post Git Migration Update! Developer workflow and tools documentation has now
largely moved to the man pages provided with depot_tools.**

Please see [the online version of those
docs](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html),
and especially the new [tutorial
page](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html)
for the most up-to-date information about getting and working with Chromium
sources.

Not all the information here has been migrated to the depot_tools docs yet, so
this page will stay around for a while as a resource, but where there are
discrepancies, the depot_tools pages should be considered authoritative.

---

# New to Git?

If you're new to Git but experienced with Subversion, we recommend following the
[Git-SVN Crash Course](http://git-scm.com/course/svn.html). This guide should
help you understand how Git differs from Subversion and how to get up to speed
the fastest.

# Prerequisites

*   Committers will need a Chromium access account. You can request one
            at [Chromium access](https://chromium-access.appspot.com/) using the
            [request form](https://chromium-access.appspot.com/request). Only
            account holders (read-only or read-write) can send tryjobs.

*   [depot_tools](/developers/how-tos/install-depot-tools) is required
            on every platform. Install it and make sure it's correctly in your
            PATH.

## Windows Prerequisites

Run gclient **TWICE, FROM A CMD WINDOW** to download and setup everything else
you need. It's important to run twice, and not to use msysgit bash or other
non-cmd shells, because otherwise gclient may fail to properly install all its
dependencies. Using the "--version" flags just reduces the amount of output
spew; it's not necessary for the operations to succeed. If you run gclient
--version a third time it should succeed.

gclient --version gclient --version

After running gclient (twice), depot_tools will now contain a full stand-alone
installation of msysgit. **If you have a previous installation of msysgit, it is
strongly recommended that you use the version installed under depot_tools**.
This version of msysgit contains custom performance improvements that facilitate
working with very large git repositories (like chromium and blink). You can run
the shell from the provided version of msysgit using:

/path/to/depot_tools/git-.../bin/sh.exe --login -i

where git-... will depend on which version of msysgit was fetched (e.g.
git-1.9.0.chromium.5_bin). However you will normally just run git.bat, which
should now be in your path.

## Mac and Linux Prerequisites

You'll need to manually install:

*   Git 1.9 or above

# Initial checkout

First, tell git about yourself.

git config --global user.name "My Name"
git config --global user.email "my-name@chromium.org"
git config --global core.autocrlf false
git config --global core.filemode false git config --global
branch.autosetuprebase always

## Git credentials setup for committers (.netrc file)

If you plan to push commits directly to Git (bypassing Commit Queue, e.g. with
'git cl land') you would need to setup netrc file with your git password:

1.  Go to <https://chromium.googlesource.com/new-password>
2.  Login with your **@chromium.org** account (e.g. your committer
            account, non-chromium.org ones work too).
3.  Follow the instructions in the "Staying Authenticated" section. It
            would ask you to copy-paste two lines into ~/.netrc file.

In the end, ~/.netrc (or %HOME%/_netrc on Windows) should have two lines that
look like:

machine chromium.googlesource.com login git-yourusername.chromium.org password
&lt;generated pwd&gt;

machine chromium-review.googlesource.com login git-yourusername.chromium.org
password &lt;generated pwd&gt;

Make sure that ~/.netrc file's permissions are 0600 as many programs refuse to
read .netrc files which are readable by anyone other than you.

On Windows, you must manually create HOME as a per-user environment variable and
set it to the same as %USERPROFILE%.

You can check that you have everything set up properly by running
tools/check_git_config.py .

## Actual Checkout

We will use the fetch tool included in depot_tools to check out Chromium,
including all dependencies. This will create a new folder in your current
working directory named src.

fetch --nohooks chromium # 'chromium' may alternatively be one of blink,
android, ios, see below. # or alternatively fetch --nohooks --no-history
chromium # get a shallow checkout (saves disk space and fetch time at the cost
of no git history) cd src git checkout main # if you are building for Linux:
build/install-build-deps.sh  # if you are building for Android:
build/install-build-deps.sh --android # if you are building for iOS: echo "{
'GYP_DEFINES': 'OS=ios', 'GYP_GENERATOR_FLAGS': 'xcode_project_version=3.2', }"
&gt; chromium.gyp_env

gclient sync

Alternatives to 'fetch chromium' are:

fetch chromium # Blink from DEPS (most recent roll) - if you're not working on
Blink itself
fetch blink # Blink at Tip of Tree (latest) - if you are working on Blink
instead of or in addition to Chromium
fetch android # Blink from DEPS with additional Android tools - if you are
building for Android
fetch ios # Using iOS dependencies instead of Mac dependencies - if you are
building for iOS

fetch chromium and fetch blink both fetch both Chromium and Blink, but the two
commands fetch different versions of Blink: fetching chromium will get a dated
Blink (most recent roll to Chromium), and is sufficient and easier if only
working on Chromium, while fetching blink will instead get the latest Blink
(ToT), and is useful if working on Blink.

In other words, fetch X if you want to work on X.

Note that, by default, fetch creates a local branch called "main". This can be
confusing if you mistake it for the upstream "origin/main". Unless you know
what you're doing, you should simply delete this branch as follows:

git checkout origin/main git branch -D main

Note that if e.g. you're developing Blink, you'll want to do this in you Blink
directory (likely third_party/WebKit) as well.

Post initial checkout, you should be able to switch between projects via
`sync-webkit-git.py` but better tooling is being worked on. If you wish to later
add Android to an existing Chromium tree, `sync-webkit-git.py` will not help. It
may be better to make a separate top level directory and have a parallel tree.
While the projects can live happily in the same tree, we don't have tools yet to
help you configure the tree for both.

## Additional environments

If you're building Chrome for:

*   Chrome OS, see [these build
            instructions](http://www.chromium.org/developers/how-tos/build-instructions-chromeos)
*   Android, see [these build
            instructions](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/android_build_instructions.md)
*   iOS, see [these build instructions](/system/errors/NodeNotFound)

## Using the last known good/compilable revision (LKGR/LKCR)

If you'd like to only sync to the last known good revision (lkgr), you can
checkout **origin/lkgr** instead of **origin/main**. Similarly, the lkcr is
available at **origin/lkcr**.

## Updating the code

Update your current branch with `git pull` followed by `gclient sync`, as
follows. Note that if you're not on a branch, `git pull` won't work, and you'll
need to use `git fetch` instead (but you make all your changes on branches,
right? See "Contributing" below).

cd "$CHROMIUM_DIR"
git pull
gclient sync

If developing Blink, you'll need to pull Blink as well, as follows:

cd "$CHROMIUM_DIR" && git pull
cd "$BLINK_DIR" && git pull
gclient sync

To speed up updating, using more jobs, for example --jobs=16:

cd "$CHROMIUM_DIR"
git pull
gclient sync --jobs=16

# Contributing

Moved to [Committing and Reverting Changes
Manually](/system/errors/NodeNotFound).

# Commit your change manually

Moved to [Committing and Reverting Changes
Manually](/system/errors/NodeNotFound).

# Tips

See [GitTips](https://code.google.com/p/chromium/wiki/GitTips) for general tips,
and [GitCookbook](https://code.google.com/p/chromium/wiki/GitCookbook) for more
Chromium-specific tips.

## Googler

See <http://wiki/Main/ChromeBuildInstructions>

## Multiple Working Directories

Moved to [Managing Multiple Working
Directories](/developers/how-tos/get-the-code/multiple-working-directories).

## Working with release branches

Moved to [Working with Release
Branches](/developers/how-tos/get-the-code/working-with-release-branches).

## Branches

Moved to [Working with
Branches](/developers/how-tos/get-the-code/working-with-branches).

## If gclient sync fails

*   Make sure you checked out main: run git branch
*   Run git status to make sure you don't have any uncommitted changes
*   Try running git rebase origin/main directly to get more specific
            errors that gclient sync might not show
*   Do the same in each subdirectory that belongs to a separate
            repository that you might have worked in - for example, if you
            hacked on
            WebKit[?](https://code.google.com/p/chromium/w/edit/WebKit) code, cd
            to
            third_party/WebKit[?](https://code.google.com/p/chromium/w/edit/WebKit)/Source
            and run git status there to make sure you're on the main branch
            and don't have uncommitted changes.

Sometimes you'll get the message "You have unstaged changes." when you
personally don't, often due to a directory that has been moved or delete. You
can fix this by moving the directory outside of the repo directory (or deleting
it), and then trying gclient sync again.

It is also possible that you're getting this due to local changes (or other
problems) in the depot_tools directory. You can fix these by going to
depot_tools and resetting:

git reset --hard HEAD

## Working with repositories in subdirectories other than the main Chromium repository

Moved to [Working with Nested
Repos](/developers/how-tos/get-the-code/working-with-nested-repos).

## Reverting a change

Moved to [Committing and Reverting Changes
Manually](/system/errors/NodeNotFound).

## Seeing strange errors using Git on Windows?

Moved to [Windows Build
Instructions](/developers/how-tos/build-instructions-windows).

## Using Emacs as EDITOR for "git commit" on Mac OS

Moved to [Mac build
instructions](https://code.google.com/p/chromium/wiki/MacBuildInstructions).

## Tweaking similarity

git-cl defaults to using 50% similarity as a threshold for detecting renames.
This is sometimes inappropriate, e.g., if splitting off a small file from a
large file (in which case you want a smaller threshold, to avoid false
negatives), or when adding a small file (in which case you want a larger
threshold, to avoid false positives from common boilerplate). This is controlled
by the add_git_similarity function in git_cl.py, and you can set threshold to a
given value for a branch (saved in config for that branch), or not look for
copies on a specific upload (but not saved in config for the branch):

git cl upload --similarity=80 # set threshold to 80% for this branch
git cl upload --no-find-copies # don't look for copies on this upload

# Managed mode

Moved to [Gclient Managed
Mode](/developers/how-tos/get-the-code/gclient-managed-mode).

# Need help?

If you find yourself needing help with the new workflow, please file a bug with
the infrastructure team at
<https://code.google.com/p/chromium/issues/entry?template=Build%20Infrastructure>
