---
breadcrumbs:
- - /developers
  - For Developers
page_name: experimental-branches
title: Experimental Branches
---

Sometimes it is useful to share work directly, rather than just via code
reviews. Everyone who is a Chromium Committer has permission to push their work
to a set of experimental refs in the chromium/src.git repository. Simply

1.  Check out the branch you wish to push
2.  git push origin HEAD:refs/experimental/**&lt;your
            username&gt;**/**&lt;branch name&gt;**

where **&lt;your username&gt;** is "foo@chromium.org" and **&lt;branch
name&gt;** is whatever you want the ref to be called (e.g. "foo-refactor-wip").
You'll then be able to view your push online at
https://chromium.googlesource.com/chromium/src/+/refs/experimental/&lt;your
username&gt;/&lt;branch name&gt;.

Note that anyone, including anonymous users, can checkout or view online what
you push to your experimental ref.
