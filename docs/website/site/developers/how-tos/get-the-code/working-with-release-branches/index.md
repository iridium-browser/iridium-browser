---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/get-the-code
  - 'Get the Code: Checkout, Build, & Run Chromium'
page_name: working-with-release-branches
title: Working with Release Branches
---

*This applies to commits to Chromium repository branches. For changes to
Chromium OS repositories, see the information
[here](/chromium-os/how-tos-and-troubleshooting/working-on-a-branch).*

## Checking out a release branch

**Note: Prior to branch 3420 it is usually NOT possible to sync and build a
release branch** (i.e. with consistent third_party DEPS), The instructions below
will only work with branch 3420 or later. For old branches, please refer to the
internal documentation (*go/ChromeReleaseBranches*).

```sh
# Make sure you are in 'src'.
# This part should only need to be done once, but it won't hurt to repeat it.
# The first time checking out branches and tags might take a while because it
# fetches an extra 1/2 GB or so of branch commits.
gclient sync --with_branch_heads --with_tags

# You may have to explicitly 'git fetch origin' to pull branch-heads/
git fetch

# Checkout the branch 'src' tree. $BRANCH can be found under Chromium
# [here](https://chromiumdash.appspot.com/branches).
git checkout -b branch_$BRANCH branch-heads/$BRANCH

# Checkout all the submodules at their branch DEPS revisions.
gclient sync --with_branch_heads --with_tags
```

## Building a branch

Once checked out, building a branch should be [the same as building
trunk](/developers/how-tos/get-the-code). To avoid clobbering other build
artifacts, you may want to specify a different build directory (e.g.
`//out/Branch1234` instead of `//out/Default`).

## Merging a patch from another branch (e.g. trunk)

Please see the [cherry-pick/drover instructions.](/developers/how-tos/drover)

## Developing a patch directly on the branch

**Note:** Bugs should generally be fixed and tested on trunk (canary) and then
merged to branches. However, if you cannot do that:

```sh
# Make sure you are in 'src'.
# Create a branch for the patch.
git new-branch --upstream branch-heads/$BRANCH my_hack_on_the_branch

# Develop normally.
git commit
git cl upload

# After your CL is carefully reviewed, land via the normal CQ process.
git cl land
```

## Syncing and building a release tag

Releases are tagged in git with the release version number.
Note: You cannot commit to a release tag. This is purely for acquiring the
sources that went into a release build.

```sh
# Make sure you have all the release tag information in your checkout.
git fetch --tags
# Checkout whatever version you need (known versions can be seen with
# 'git show-ref --tags')
git checkout -b your_release_branch 74.0.3729.131 # or tags/74.0.3729.131
gclient sync --with_branch_heads --with_tags
```

Then build as normal.

## Get back to the "trunk"

```sh
# Make sure you are in 'src'.
git checkout -f origin/main
gclient sync
```
