# Advanced Gerrit Usage

This file gives some examples of Gerrit workflows.  This includes uploading
patches with Gerrit, using Gerrit dependency chains, and managing local git
history.

## Managing patches with Gerrit

The instructions in [README.md](README.md) using `git cl upload` are preferred
for patch management.  However, you can interact manually with the Gerrit code
review system via `git` if you prefer as follows.

### Preliminaries

You should install the
[Change-Id commit-msg hook](https://gerrit-documentation.storage.googleapis.com/Documentation/2.14.7/cmd-hook-commit-msg.html).
This adds a `Change-Id` line to each commit message locally, which Gerrit uses
to track changes.  Once installed, this can be toggled with `git config
gerrit.createChangeId <true|false>`.

To download the commit-msg hook for the Open Screen repository, use the
following command:

```bash
  curl -Lo .git/hooks/commit-msg https://chromium-review.googlesource.com/tools/hooks/commit-msg
  chmod a+x .git/hooks/commit-msg
```

### Uploading a new patch for review

You should run `git cl presubmit --upload` in the root of the repository before pushing for
review (which primarily checks formatting).

After verifying that presubmission works correctly, you can then execute:
`git cl upload`, which will prompt you to verify the commit message and check
for owners.

The first time you upload an issue, the issue number is associated with the
current branch. If you upload again, it uploads on the same issue (which is tied
to the branch, not the commit). See the [git-cl](https://chromium.googlesource.com/chromium/tools/depot_tools.git/+/HEAD/README.git-cl.md) documentation for more information.

## Uploading a new dependent change

If you wish to work on multiple related changes without waiting for them to
land, you can do so in Gerrit using dependent changes.  There doesn't appear to
be any official documentation for this, but the rule seems to be: if the parent
of the commit you are pushing has a Change-Id line, that change will also be the
current change's parent.  This is useful so you can look at only the relative
diff between changes instead of looking at all of them relative to master.

To put this into an example, let's say you have a commit for feature A with
Change-Id: aaa and this is in the process of being reviewed on Gerrit.  Now
let's say you want to start more work based on it before it lands on master.

``` bash
  git checkout featureA
  git checkout -b featureB
  git branch --set-upstream-to featureA
  # ... edit some files
  # ... git add ...
  git commit
```

The git history then looks something like this:

```
  ... ----  master
          \
           A
            \
             B <- HEAD
```

and `git log` might show:

```
commit 47525d663586ba09f40e29fb5da1d23e496e0798 (HEAD -> featureB)
Author: btolsch <btolsch@chromium.org>
Date:   Fri Mar 23 10:18:01 2018 -0700

    Add some better things

commit 167a541e0a2bd3de4710965193213aa1d912f050 (featureA)
Author: btolsch <btolsch@chromium.org>
Date:   Thu Mar 22 13:18:09 2018 -0700

    Add some good things

    Change-Id: aaa
```

Now you can push B to create a new change for it in Gerrit:

``` bash
git push origin HEAD:refs/for/master
```

In Gerrit, there would then be a "relation chain" shown where the feature A
change is the parent of the feature B change.  If A introduces a new file which
B changes, the review for B will only show the diff from A.

## Examples for maintaining local git history

```
                  D-E --- feature B
                 /  ^N
            A-B-C-F-G-H --- feature A
           /    ^M  ^O
          /
  ... ----  master
        /|
       M O
       |
       N
```

Consider a local repo with a master branch and two feature branches.  Commits M,
N, and O are squash commits that were pushed to Gerrit.  The arrow/caret (`^`)
indicates whence those were created.  M, N, and O should all have Change-Id
lines in them (this can be done with the [commit-msg
hook](https://gerrit-documentation.storage.googleapis.com/Documentation/2.14.7/cmd-hook-commit-msg.html)).
M and O are separate patchsets in one review (M containing A, B, C and O
containing A, B, C, F, G) and N is the first patchset in a new review that is
dependent on the first patchset of the first review.

Starting without M, N, or O, the commands to create them are as follows:

``` bash
git checkout C
git checkout -b M
git rebase -i origin/master # squash commits
# Note: make sure a Change-Id line exists on M at this point since N will need
# it.  You can git commit --amend with the commit-msg hook active or add it via
# git commit --amend after pushing.  Don't git commit --amend after creating N
# though.
git push origin HEAD:refs/for/master
git checkout E
git checkout -b N
git rebase -i C --onto M # squash commits
git push origin HEAD:refs/for/master
git checkout G
git checkout -b O
git rebase -i origin/master # squash commits and copy the Change-Id line from M
git push origin HEAD:refs/for/master
```

```
                        D-E --- feature B
                       /  ^Q
            A-B-C-F-G-H --- feature A
           /          ^P
          /
  ... ----  master
         /|\
        M O P
        |   |
        N   Q
```

The next example shows an additional patchset being uploaded for feature A
(commit P) and feature B being rebased onto A, then uploaded to Gerrit as commit
Q.

Starting from the endpoint of the previous commands, this point can be reached
as follows:

``` bash
git checkout H
git checkout -b P
git rebase -i origin/master # squash commits, same note as M about Change-Id
git push origin HEAD:refs/for/master
git checkout featureB # E
git rebase # assume featureA is set as featureB's upstream branch
git checkout -b Q
git rebase -i H --onto P
git push origin HEAD:refs/for/master
```
