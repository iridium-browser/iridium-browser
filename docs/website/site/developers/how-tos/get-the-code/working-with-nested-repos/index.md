---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/get-the-code
  - 'Get the Code: Checkout, Build, & Run Chromium'
page_name: working-with-nested-repos
title: Working with Nested Third Party Repositories
---

The Chromium source tree includes other repositories in third party directories
(src/third_party) and elsewhere.

## Make changes

## Do your work, on a branch here, and commit locally.

git checkout -b work
vim third-party-file.cc
git commit -a -m "work done"

## Upload for review

git cl upload

## Do the usual add-reviewers-seek-lgtm dance. Once reviewed, submit your change:

git cl land

## Done? Not quite: you need to roll your change into the Chromium tree.

## Roll DEPS

Since Chromium mirrors these projects with git, the DEPS file entry for the
project has a git hash. So how do you update the DEPS file hash for source code
changes that don't really live in git?

Method #1: Use the depot_tools roll-dep script

# cd to the main src directory for your checkout cd src # Create and switch to a
new branch git new-branch depsroll # Run roll-dep giving the path and the
desired revision # if you omit the revision, it will roll to the current repo
HEAD roll-dep src/third_party/foo
\[--roll-to=SVN_REVISION_NUMBER/GIT_COMMIT_HASH\]

Method #2: Manual inspection (patch the hash into the DEPS file by hand).

# cd to the main src directory for your checkout pushd src # Create and switch
to a new branch git new-branch depsroll # cd to the directory listed in the DEPS
file (e.g., src/third_party/cld_2/src for 'cld2') pushd third_party/foo/bar #
Fetch (but do not pull) the latest revisions from Chromium's git mirror of the
remote repository git fetch origin # Use 'git log' to list the commits in the
mirror and find the hash of the one you need git log origin # Go back to src/
popd # Patch the revision hash into the DEPS file by hand.

Regardless of method used, the result is a modified DEPS file. Commit it locally
and upload for review:

git commit -a -m "roll third_party/whatever deps because ..."
git cl upload

## Adding new repositories to DEPS

Ensure the repository in question **is mirrored at chromium.googlesource.com**.
We do this to avoid swamping upstream hosts with loads of traffic from our
developers and bot fleet. To mirror a repository, please file an [infrastructure
ticket](https://code.google.com/p/chromium/issues/entry?template=Build%20Infrastructure).
