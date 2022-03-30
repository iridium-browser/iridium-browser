---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: blink-post-merge-faq
title: Blink post-merge FAQ
---

## Merge versions

**Pre-merge:**

> Blink git SHA:
> [37d233bde3baaea720a9a81296fa77b63c9d8981](https://chromium.googlesource.com/chromium/blink/+/HEAD)

> Blink revision: **[202666](http://blinkrev.hasb.ug/202666)**

> Chromium git SHA:
> [70aa692d68ee86d365928edd160c3575fda2b453](https://chromium.googlesource.com/chromium/src/+/70aa692d68ee86d365928edd160c3575fda2b453)
> [350323](http://crrev.com/350323) Chromium revision:
> Chrome version:
> [47.0.2518.0](https://chromium.googlesource.com/chromium/src/+/70aa692d68ee86d365928edd160c3575fda2b453/chrome/VERSION)

**Post-merge:**

> Blink git SHA: *n/a*

> Blink revision: *n/a*

> Chromium git SHA:
> [b59b6df51a249895fbba24f92b661f744e031546](https://chromium.googlesource.com/chromium/src/+/b59b6df51a249895fbba24f92b661f744e031546)

> Chromium revision: **[350324](http://crrev.com/350324)**

> Chrome version:
> [47.0.2519.0](https://chromium.googlesource.com/chromium/src/+/341be68a2f3f424685a709aacc683d227f0de7eb)
> -- version bump came 1 day after
> [branch](https://chromium.googlesource.com/chromium/src/+/b59b6df51a249895fbba24f92b661f744e031546/chrome/VERSION)

## How to migrate local /src (chromium main project) branches

*   ## Only once to update the remote

    ## ```none
    git remote update (or git fetch)  
    ```

    ## `For each branch`

    ## ```none
    git rebase $(git merge-base branch_name origin/master) branch_name --onto origin/master
    ```

## How to migrate local /src/third_party/WebKit branches

When running gclient sync after the merge point, the previous .git directory
(containing all the local branches) for blink will be saved in
//src/../old_src_third_party_WebKit.git

For each local branch, run the following steps:

*   Create a patch file

    ```none
    git diff origin/master...branch_name > /tmp/old-blink.patch
    ```

And then in the new checkout, after the branch point

*   Recreate the branch

    ```none
    git new-branch old-blink
    ```

*   Apply the patch

    ```none
    git apply -3 --directory third_party/WebKit/ /tmp/old-blink.patch
    ```

## How to migrate blink patches from Rietveld

If you have the patch locally as a git branch, migrating that would be easier.
Otherwise, follow these steps:

1.  Download the raw patch from codereview.chomium.org/123456
2.  Create a new local git branch in a post-merge checkout

    ```none
    git new-branch issue123456
    ```

3.  Apply the patch

    ```none
    git apply -3 --directory third_party/WebKit/ issue123456_1001.diff
    ```

4.  Add new files and commit changes
5.  Upload to a new CL

## How can I avoid getting the entire Blink history when going over the merge commit

If you use `git log` or similar over the merge commit, use `--first-parent` to
walk up only in the chromium history.

## Cherry-picking a Blink CL to a release branch

The active releases branches will be merged (i.e. won't have a
third_party/WebKit subproject) as well.

For CL that get landed after the merge day, it won't be any different than
cherry-picking a chromium CL. Just follow the
[git-drover](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/git-drover.html)
man page.

To cherry-pick a blink CL that was landed before the merge day:

*   Find the corresponding commit that you want to cherry-pick in the
            chromium master and note its SHA1 (note: it will have a different
            SHA1 than what reported in the codereview.chromium.org/NNNN)
    e.g., if you want to cherry-pick
    <https://codereview.chromium.org/1340403003>

    ```none
    git log origin/master --grep codereview.chromium.org/1340403003 -- third_party/WebKit
    commit c614d6deae3c6e4cc0bfbb60bb15a17305b418b7
    Author: ....
    ```

*   Follow the
            [git-drover](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/git-drover.html)
            documentation using SHA1 above when doing git cherry-pick -x

## Creating a checkout using fetch blink does no longer work

Use `fetch chromium` instead.

## Converting revision range links from SVN to git

See [this document
](https://docs.google.com/document/d/1v2fTiNF2FNzbSZXuF1JQxT3UHghM9GUI5csWIpSdu68/edit)for
instructions.

## Bisecting across the merge point

*   Bisect scripts (src/tools/bisect-builds.py) and bots should Just
            Work TM. File a bug if that is not the case.
*   Doing a git bisect on a revision range that includes the merge point
            requires a little trick. (See [\[chromium-dev\] Merge of Chromium
            and Blink repositories - git bisect is
            broken?](https://groups.google.com/a/chromium.org/forum/#!topic/chromium-dev/ydCRw4V6u5o)
            for context and details)
*   The trick consists in convincing git that the merge commit
            (b59b6df51) did NOT pull in the blink history (70aa692d6) using
            grafts

    ```none
    git replace --graft b59b6df51 70aa692d6
    # Do the git bisect as usual
    git bisect start GOOD_SHA1 BAD_SHA1 
    # Remove the grafts at the end
    $ git replace -d b59b6df51
    ```
