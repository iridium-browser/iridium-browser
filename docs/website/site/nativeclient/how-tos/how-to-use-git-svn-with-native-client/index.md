---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: how-to-use-git-svn-with-native-client
title: Developing Native Client
---

**Setup from scratch**

Native Client code depends on some external code that is not present in its own
repository. These dependencies (aka DEPS) are managed by a tool called gclient
that is part of Chromium's depot_tools package. Get depot_tools
[here](/developers/how-tos/install-depot-tools) and put the directory (we'll
call it $DEPOT_TOOLS) in your PATH.

Then create a directory to hold Native Client and all its deps (We'll call it
$NACL_DIR) and cd into it.

```none
cd $NACL_DIR
```

Then fetch the source code.

```none
fetch nacl
```

This command sets up a .gclient config file, fetches the Native Client sources
from git, then reads the DEPS file checked into native client's git repository,
and fetches the dependencies from their git repos.
If you will be committing, you will also want to set up 'git cl', the
depot_tools code review and commit tool.

```none
cd $DEPOT_TOOLS
git cl config   # just hit return at all the prompts
```

If you want to build 32 bit nacl on a x86_64 system, you also need to:

```none
cd $NACL_DIR
sudo native_client/tools/linux.x86_64.prep.sh
```

Your workflow might be like the following:

**Update your master branch**

```none
cd $NACL_DIR
cd native_client
git checkout master
git pull
gclient sync
```

The the 'git pull' command updates your origin/master branch to match the server
and merges it into your local checked-out master branch. The 'gclient sync'
command will pull down the dependencies specified in the DEPS file. If you use a
local master branch in this way, you should not make local commits to it, or
'git pull' will complain about non-fast-forward merges.

**Create a new branch for a new CL**

```none
git checkout -b my_feature origin/master
```

(This is equivalent to git branch my_feature origin/master; git checkout
my_feature)

**Hack away, committing to git whenever you like**

```none
emacs $file
git commit -a (roughly equivalent to git add $file; git commit)
emacs $file
git commit -a
./scons $tests
```

I like to at least have a commit for every time I run a test (or try job) that
takes sufficiently long that I won't sit and wait for it. That way when I come
back and look at the results, I'm sure to have a snapshot of my code that was
tested, even if I make new edits while the test is running.

**Send a try job**

```none
git try
```

The 'git try' command will try whatever state you have committed in your local
git repo (it will complain if you have uncommitted changes).

If you use 'git cl try' it will try whatever patchset you have currently
uploaded to Rietveld (see 'git cl upload' below).

**Send out for code review**

```none
  git cl upload
```

This creates a new CL and associates it with the current branch (all commits on
this branch are now assumed to be part of this CL). It then uploads it to the
code review server. git cl upload also supports the -m option for the patch
message, -r for reviewers, --cc for extra people to CC in the email, and a few
others. If you commit a new change and run 'git cl upload' again, it will upload
a new patchset to the existing CL.

**Wait for the review to come back.**

In the meantime, you can switch to another branch and work on something else.

```none
git fetch origin
git checkout -b my_next_feature origin/master
```

The above sequence will base your next CL off of the current master.
Alternatively you can base your new CL on your first one by omitting the last
argument to 'git checkout'.

**That mean reviewer wanted changes!**

Edit based on results of review, then re-upload for more review

```none
git checkout my_feature
emacs $file
git commit -a
git cl upload
```

**Got LGTM!**

We are almost ready to commit to the server. However, while you were waiting,
there were other commits to the master, so we need to rebase my_feature against
the new HEAD and make sure everything still works. First, update the local
master:

```none
git checkout master
git pull
gclient sync
```

Then, rebase your local changes off the new master:

```none
git checkout my_feature
git rebase origin/master
```

Test, try and update as needed:

```none
./scons <tests>
git try
```

Everything works! Commit it:

```none
git cl land
```

'git cl land' squashes all the changes you made on your branch into a single
diff and commits them to the master branch in a single commit.

Now you can update your local master again (to include the change you just
committed) and start a new branch for your next CL.

```none
git checkout master
git pull
gclient sync
git checkout -b my_even_newer_feature origin/master
```

If you have a CL already in flight (especially one that was branched off of your
previous feature branch rather than master), you will probably want to rebase it
against master now. After you have updated your master branch as above,

```none
git checkout my_next_feature
git rebase origin/master
```
