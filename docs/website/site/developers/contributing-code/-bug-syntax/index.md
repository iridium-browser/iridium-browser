---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/contributing-code
  - Contributing Code
page_name: -bug-syntax
title: CL Footer Syntax
---

Chromium stores a lot of information in footers at the bottom of commit
messages. With the exception of R=, these trailers are only valid in the last
paragraph of a commit message; any trailers separated from the last line of the
message by whitespace or non-trailer lines will be ignored. This includes
everything from the unique "Change-Id" which identifies a Gerrit change, to more
useful metadata like bugs the change helps fix, trybots which should be run to
test the change, and more. This page includes a listing of well-known footers,
their meanings, and their formats:

**Bug:**

A comma-separated list of bug references, where a bug reference is either just a
number (e.g. 123456) or a project and a number (e.g. skia:1234). On
chromium-review, the default project is assumed to be 'chromium', so all bugs in
non-chromium projects on bugs.chromium.org should be qualified by their project
name.

**Fixed:**

The same as Bug:, but will automatically close the bug(s) as fixed when the CL
lands.

**R=**

A comma-separated list of reviewer email addresses (e.g. foo@example.com,
bar@example.com). This field will be going away with the switch to Gerrit, use
the "-r foo@example.com" command line option to "git cl upload" instead.

**Tbr:**

Same format as the R line, but indicates to the commit queue that it can skip
checking that all files in the change have been approved by their respective
OWNERS.

**Cq-Include-Trybots:**

A comma-separated list of trybots which should be triggered and checked by the
CQ in addition to the normal set. Trybots are indicated in "builder
group:builder" format (e.g. tryserver.chromium.linux:linux_asan_experimental).

**No-Presubmit:**

If present, the value should always be the string "True". Indicates to the CQ
that it should not run presubmit checks on the CL. Used primarily on automated
reverts.

**No-Try:**

If present, the value should always be the string "True". Indicates to the CQ
that it should not start or check the results of any tryjobs. Used primarily on
automated reverts.

**No-Tree-Checks:**

If present, the value should always be the string "True". Indicates to the CQ
that it should ignore the tree status and submit the change even to a closed
tree. Used primarily on automated reverts.

**Test:**

A freeform description of manual testing that was done to ensure the change is
correct. Not necessary if all testing is covered by trybots.

**Reviewed-by:**

Automatically added by Gerrit when a change is submitted, this is a list of
names and email addresses of the people who approved (set the Code-Review label
on) the change prior to submission.

**Reviewed-on:**

Automatically added by Gerrit when a change is submitted, this is a link back to
the code review page for easy access to comment and patchset history.

**Change-Id:**

Automatically added by "git cl upload", this is a unique ID that helps Gerrit
keep track of commits that are part of the same code review.

**Cr-Commit-Position:**

Automatically added by the git-numberer Gerrit plugin when a change is
submitted, this is of the format fully/qualified/ref@{{ '{#' }}123456} and gives both
the branch name and "sequence number" along that branch. This approximates an
SVN-style monotonically increasing revision number.

**Cr-Branched-From:**

Automatically added by the git-numberer Gerrit plugin on changes which are
submitted to branches other than master, to help someone looking at that commit
know when that branch diverged from master.
