---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/day-to-day
  - '6: Day-to-Day Engineering'
page_name: codereview
title: Native Client Code Review and Check-in Process
---

# Subteams

1) Designate owners for that subteam.
2) Set policies for who can give LGTM for the modules owned by that subteam.
3) Once OWNERS is implemented, there may be an approval step required in
addition to (2).

## Authors

0) Read the [coding style document](/nativeclient/styleguide).
1) Run cpplint.py, pylint, tidy, etc., as appropriate.
2) The change list comment should describe the change and whether it is a
cosmetic change, a bug fix, or a new feature.
3) Make sure the CL is passing trybots before sending out for review.
4) Pick reviewers that are familiar with the code in question.

a) Pick more than one reviewer if the CL spans more than one module.

b) One LGTM per module touched is required.

c) (Multiple reviewer details to be worked out.)

5) All review requests should be sent to native-client-reviews@googlegroups.com.
6) All “significant” CLs have a BUG= line and an entry in the issue tracker.

e.g., CLs that fix misspellings in comments or improve coding convention
conformance are not bug-worthy, while fixing a crasher or adding a new PPAPI
interface is.

7) All new code CLs have a TEST= line and one or more tests that exercise the
new code.
e.g., Changing comments does not require a test, while adding a new system call
does.
The test should be specific enough that the reviewer can reproduce it.
8) Don’t re-upload the CL until addressing all of the reviewers’ comments from
an iteration.
Explicitly reply to comments in Rietveld.

(Rietveld makes it difficult to ensure all the comments were addressed.)

## Code reviewers

Ensure
1) The CL conforms to the [NaCl coding
conventions](http://www.chromium.org/nativeclient/styleguide).
2) The CL passed the trybots.
3) The CL follows the designated author tasks above.
4) You understand what the CL is doing and believe it is doing it correctly.
5) Review iterations are completed within one business day or referred to
another reviewer.

a) Repeated issues may be elided and another iteration required.

b) Large CLs may be broken into sub-reviews.

c) We encourage sending comments or feedback early so as to pipeline the fixes.

The reviewer may summarily reject the CL if it doesn’t follow (2-3).
If you LGTM a CL that breaks the build, you share responsibility for the break.

## Sheriffs

1) The first responsibility at all times is to keep the tree open. Even if that
means going back to the last known good revision.
2) Can summarily revoke CLs, and ask author or a reviewer(s) to do revocation.
3) For more details, see <http://www.chromium.org/developers/tree-sheriffs>
