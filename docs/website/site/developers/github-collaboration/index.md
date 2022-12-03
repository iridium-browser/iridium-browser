---
breadcrumbs:
- - /developers
  - For Developers
page_name: github-collaboration
title: GitHub Collaboration
---

This document explains how to collaborate on work in progress by using Git and
pushing changes to GitHub.

Alternatives would be to email code diffs or use codereview.chromium.org and
`git cl patch`. However they are brittle and time consuming when patches do no
apply cleanly and or are part of several pipelined or dependent patches.

Collaboration via git is made easy by simply using `git merge` to include code
from other patches into a local branch you are working with.

1.  Create a chromium fork on GitHub if you will share code with others.
    1.  [**Fork**](https://help.github.com/articles/fork-a-repo/) an
                existing chromium repository on GitHub such as:
        *   <https://github.com/chromium/chromium>
    2.  [**Delete
                branches**](https://help.github.com/articles/creating-and-deleting-branches-within-your-repository/)
                you are not interested in.
2.  Add remote references to GitHub respositories (yours or others)
    *   git remote add g https://github.com/your-user-name/chromium
    *   git remote add other-repo-name
                https://github.com/other-user-name/chromium
    *   (I typically name my own github repo just 'g')
3.  [Fetch](https://git-scm.com/docs/git-fetch) from github repos
    *   git fetch g
    *   git fetch other-repo-name
    *   Occasionally add `--prune` to remove any remote-tracking
                references that no longer exist on the remote.
4.  [Merge](https://git-scm.com/docs/git-merge) from github repos
    *   git merge other-repo-name/branch-name
5.  [Push](https://git-scm.com/docs/git-push) to github
    *   Push current branch
        *   git push g HEAD
    *   Push all branches where the local names match remote names.
        *   git push g :
    *   Delete a remote branch
        *   git push g -f :remote-branch
