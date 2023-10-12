---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/get-the-code
  - 'Get the Code: Checkout, Build, & Run Chromium'
page_name: working-with-branches
title: Working with Branches
---

*This applies to commits to Chromium repository branches. For changes to
Chromium OS repositories, see the information
[here](/chromium-os/how-tos-and-troubleshooting/working-on-a-branch).*

### Basics

Enumerate your local branches:

```shell
cd src
git branch
```

Switching from one branch to another: Example: Switching from branch 'branch1'
to branch 'branch2'.

```shell
cd src
git checkout branch2
```

Note that `-` can be used to refer to the previous branch, which is useful when
switching between two branches.

```shell
git checkout some_branch
git checkout -  # back to previous branch!
```

### Suggested branching workflow

We normally do our feature work in branches to keep changes isolated from each
other. The recommended workflow is to make local branches off the server main,
referred to as the origin/main branch. The `git-new-branch` command (in
depot_tools) will do this:

```shell
git new-branch a_new_branch_name
```

Note that this is equivalent to the following:

```shell
git checkout -b a_new_branch_name origin/main
```

### Branch off a branch

If you have dependent changes, a very productive workflow is to have a branch
off a branch. Notably, this means that your patch sets (in Gerrit) will show
the separate changes, and be easy to review, rather than showing the merged
changes (including irrelevant changes), and means you can commit downstream CLs
without having to rebase them after the upstream has landed. Do this as follows:

```shell
git checkout main
git checkout branch1
# some edits, commit
git checkout -b branch2
git branch --set-upstream-to=branch1
# some edits, commit
```

Now when you update the first branch, you can simply git pull on the second
branch as normal to pull in your changes:

```shell
git checkout branch1
# some edits, commit
git checkout branch2
git pull
```

When the first branch is merged, "git rebase-update" will automatically reparent
"branch2" onto "origin/main" for you.

**Splitting up a CL**

A common variant of "branch off a branch" is splitting up a large CL into
pieces. Given a local branch `big`, you'd like to split it up into `branch1` and
`branch2`.

One way to do this is to split off `branch1`, have that reviewed and committed,
update local repo (`gclient sync`), and then rebase `big` to `main`. This is
ok, but adds latency, and you can get the same result locally, so the second
part of the CL can be uploaded and reviewed even before the first part is
committed.

This can be done manually, particularly if the files don't overlap, by making a
new branch `branch1`, manually copying the changed files in the first part, then
branching `branch2` off of that, and manually copying the files in the second
part.

However, this can be done more cleanly via git. The main points are `git-add -i`
([Interactive
Staging](http://git-scm.com/book/en/Git-Tools-Interactive-Staging): to
interactively choose files or hunks of a patch set) and `git-rebase` (to set the
second part to be dependent on the first part, removing the common changes);
[`git-cherry-pick`](http://git-scm.com/docs/git-cherry-pick) is a technicality:

```shell
# first split off branch1
git checkout origin/main
git new-branch branch1
git cherry-pick -n ..big  # apply and stage all ancestors of big that are not ancestors of HEAD, do not commit
git add -i  # interactively choose which files or hunks to stage
git commit  # commit staged changes
git checkout -- . # discard unstaged changes
# next set big to be a branch off of branch1
git checkout big
git branch --set-upstream-to=branch1
git rebase branch1  # may need to resolve conflicts
git branch -m branch2  # done!
```

Concretely, `git-cherry-pick -n` applies and stages all changes, but does not
commit them. In `git-add -i`, you can unstage files via `3: **r**evert`, and
then stage files (or hunks of files, or edit the diff manually) via `5:
**p**atch`, with manually editing via by the `e - manually edit the current
hunk` option when staging a hunk. While tedious, this is often easier than
resolving conflicts during rebasing.

### Deleting an obsolete branch

You generally want to delete local branches after the changes have been
committed to main. It is safest to check that your work has, in fact, been
committed before deleting it.

Remember that you can always apply a CL that has been posted via:

```shell
git checkout -b myworkfrompatch
git cl patch 12345 # Using CL issue number, not bug number, applies
                   # latest patch set
git cl patch https://codereview.chromium.org/download/issue12345_1.diff
                   # Use URL of raw patch for older patch sets
```

...so as long as your CL has been posted, you can easily recover your work.
Otherwise, you'll need to dig through the git repository, so be careful.

Simply, if main (remote or local) is up-to-date and your branch has been
rebased, git branch -d will delete the branch. If not, it will refuse; to force
deletion, use git branch -D. So make sure main is up-to-date, rebase branch,
and then try deleting (optionally check manually before).

Beware that if your local branch has many revisions (instead of always amending
a single revision), rebasing to main may fail, since it will try to apply the
patches incrementally. You can avoid this by squashing your local revisions into
a single revision (see [6.4 Git Tools - Rewriting
History](http://git-scm.com/book/en/Git-Tools-Rewriting-History), or more simply
[squashing commits with
rebase](http://gitready.com/advanced/2009/02/10/squashing-commits-with-rebase.html)).
This may be more trouble than it's worth, but it's the safest way.

To check that the branch has been committed upstream:

```shell
git checkout mywork
git rebase origin/main
git diff --stat origin/main # optional check
```

If there are no differences, delete the branch:

```shell
git checkout origin/main
git branch -d mywork # will only work if has been merged
```

**NOTE:** If you haven't waited long enough after your commit, it is possible
that 'git fetch' did not get your commit and git will continue to believe that
you have local changes, which will prevent "git branch -d" from succeeding. It's
best to redo the steps later, (The repo is updated every 3 minutes) but you can
also instruct git to force-delete your branch.

Note that when you delete your branch, it will give you the SHA-1 hash for its
tip:

```
Deleted branch mywork (was 123abc0).
```

You can then recover the branch via:

```shell
git checkout -b mywork 123abc0
```

If you forget the hash, you can find it via git reflog. (Reference: [Can I
recover branch after the deletion in
git?](http://stackoverflow.com/questions/3640764/can-i-recover-branch-after-the-deletion-in-git))

### Prevent commits to main

If you commit to main, updating will be messy. (This is a good reason to
simply delete main entirely, as noted near the top of this document. You only
need to read the remainder of this section if you don't do this.)

You can prevent this by adding a pre-commit hook that checks if you're in main
and stops you from doing this. Create a file named
chromium/src/.git/hooks/pre-commit and add the below to it, then mark
executable. (Blink developers: add in blink directory as well.)

```shell
#!/bin/bash
# Prevent commits against the 'main' branch
if \[\[ \`git rev-parse --abbrev-ref HEAD\` == 'main' \]\]
then
echo 'You cannot commit to the main branch!'
echo 'Stash your changes and apply them to another branch, using:'
echo 'git stash'
echo 'git checkout &lt;branch&gt;'
echo 'git stash apply'
exit 1
fi
```

If you've accidentally uploaded a change list from main, you can clear the
association of an issue number with main via:

```shell
git cl issue 0
```

If you've accidentally committed to main, then, after copying your work to a
new branch (e.g., make patch and then apply to new branch), you can clean up
main by deleting your accidental commit as per [this
answer](http://stackoverflow.com/a/6866485) to [How to undo the last Git
commit?](http://stackoverflow.com/questions/927358/how-to-undo-the-last-git-commit)
-- see more details there.

```shell
git reset --hard HEAD~1
```
