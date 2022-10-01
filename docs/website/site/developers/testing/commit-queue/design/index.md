---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
- - /developers/testing/commit-queue
  - Chromium Commit Queue
page_name: design
title: 'Design doc: Chromium Commit queue'
---

## Objective

Have a formal way for developers to ensure their patch won't break the
Continuous Integration checks with a relatively high confidence level.

## Background

Before the Chromium Commit Queue, it's on each developer's shoulder to manually
run multiple try jobs on the [Try Server](/system/errors/NodeNotFound) and check
their results before committing.

This is "brain wasted time" as this can be completely automated. The Commit
Queue aims at automating this manual verification so the developer can start
right away working on the next patch. This is necessary to scale at a sustained
~100 commits/day.

## Overview

The CQ polls rietveld for CLs that are ready to be committed. The Chromium's
fork of Rietveld has a 'Commit' checkbox. When the author or the reviewer checks
it, it will be included in the next poll. Once the CQ learns about the CL, it
verifies the author or one of the reviewers approving this CL is a full
committer, then runs the presubmit checks, then runs try jobs and then commit
the patch on the behalf of the author, faking its credential. The whole project
is written in python.

## Infrastructure

It runs as a single-thread process. The infrastructure is really minimal since
it's just a logic layer above the current infrastructures to automate something
that was done manually by the developers. In particular, the CQ reuses:

*   the [try server](/system/errors/NodeNotFound)
*   chromium's branch of [rietveld](http://code.google.com/p/rietveld/)
*   [presubmit
            scripts](/developers/how-tos/depottools/presubmit-scripts) included
            in [depot_tools](/developers/how-tos/depottools)

## Detailed Design

Runs the following loop:

1.  Polls <http://codereview.chromium.org/search?closed=3&commit=2> to
            find new issues to attempt to commit.
2.  For each issue found,
    1.  Runs preliminary checks,
        1.  Make sure someone is a full committer
        2.  Make sure there's a LGTM or the CL is TBR'ed.
    2.  Runs presubmit checks, including OWNERS check.
    3.  Sends new try jobs with -r HEAD.
        1.  They are publicly visible at
                    <http://build.chromium.org/p/tryserver.chromium/waterfall?committer=commit-bot@chromium.org>.
        2.  Astute users will see that it runs a subset of the tests.
                    Please contribute to Flaky tests fight so more tests can be
                    used.
        3.  If ToT is broken, the commit queue will retry your patch on
                    an older revision automatically.
    4.  Makes sure no new comments were added to the issue.
    5.  If everything passes, once the tree is open;
        1.  Commits the change on the behalf of the issue owner, even if
                    the owner is not a Chromium commiter.
    6.  If one of the steps fails, the 'commit' checkbox is cleared and
                the issue is removed from the queue.
        1.  However, they author or a reviewer can re-check the box and
                    the CQ will try again.

Issues are processed asynchronously so whatever faster try job completes first
wins. This is important as a flaky CL won't bottleneck the remaining one. This
is mainly because the number of commit per hour is disproportionate to the try
server latency, e.g. the full build and test cycle time for all the platforms.

### Faking author in subversion

To commit on the behalf of the author, an unconventional technique is used in
subversion with a server-side pre-commit hook. The code is at:
<http://src.chromium.org/viewvc/chrome/trunk/tools/depot_tools/tests/sample_pre_commit_hook?view=markup>

The control flow is:

1.  On the client side, with a checkout with a special committer
            credential;
    1.  svn commit --with-revprop realauthor=&lt;author to attribute the
                commit to&gt;
2.  On the server side;
    1.  A pre_commit_hook intercepts the commits
    2.  It opens &lt;repo&gt;/db/transactions/&lt;tx&gt;.txn/props and
                parses its data
    3.  If the realauthor svn property if found,
        1.  It verifies svn:author is a special committer
        2.  It does simple sanity checks
        3.  It replaces svn:author with realauthor's value
        4.  It sets the commit-bot svn property to know this revision
                    was committed by the CQ.
    4.  The updated data is saved in the transaction file.

This works much better than using svn propset --revprop since there is no race
condition and the revision property modifications are done *during* the commit.
This technique can only be used when it is possible to set a server side hook so
for example, it can't be used with projects hosted on code.google.com.

## Project Information

*   maruel@ wrote it and dpranke@ did the code reviews.
*   Code:
            <http://src.chromium.org/viewvc/chrome/trunk/tools/commit-queue/>

## Caveats

The main problem is test flakiness. The CQ works around this problem partially
by retrying failed tests a second time. It retries compile failure with a full
rebuild, versus an incremental build normal, to work around cases of broken
incremental compiles, which does happen relatively frequently. Fixing these two
problems is outside the scope of the CQ project.

Rietveld doesn't enforce a coherent svn url mapping on its CLs, causing CLs to
be ignored by the CQ.

## Latency

The CQ is very slow at the moment. This is why the [test
isolation](/system/errors/NodeNotFound) effort was started. The CQ is bound by:

*   Rietveld polling, which is at best 10 seconds.
*   Synchronous presubmit check execution, which is synchronous and
            single threaded, but with a timeout.
*   Try job execution, including automatic retries.
*   Waiting for the tree to open.
*   The actual commit, which is fairly fast.

## Scalability

The main scalability issue is running the presubmit checks and sending the try
jobs. Many of the presubmit checks assume they are not running concurrently on
the system and running them in parallel on one VM could cause random problems.
If the CQ used a more decentralized approach, it would scale much better for
that.

## Redundancy and Reliability

There are multiple single points of failure;

*   The CQ itself, running on a single process.
*   The try server, which is itself not redundant.

## Security Considerations

The commit queue require a full committer involvement, either to be the author
of the CL or to be a reviewer giving approval. The security depends on Rietveld
assumptions about its meta data:

*   A rietveld issue cannot change of owner.
*   A rietveld comment cannot be faked by a third party.

All communications happens over https.

## Testing Plan

Comprehensive set of unit tests was written. The code itself sends stack traces
in case of exception for monitoring purposes.
