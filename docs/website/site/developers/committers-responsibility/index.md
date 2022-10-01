---
breadcrumbs:
- - /developers
  - For Developers
page_name: committers-responsibility
title: Responsibilities of Committers and Reviewers
---

<table>
<tr>
<td>This content should be migrated eventually to in-repo documentation:</td>
<td><a href="https://chromium.googlesource.com/chromium/src/+/main/docs/contributing.md">main/docs/contributing.md</a></td>
<td><a href="https://chromium.googlesource.com/chromium/src/+/main/docs/code_reviews.md">main/docs/code_reviews.md</a></td>
</tr>
</table>

When you're committing to the chromium repositories, ensure that you follow
those guidelines:

1.  Find the right reviewer for your changes **and** for patches you're
            asked to review.
2.  Use the [Commit
            Queue](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/infra/cq.md)
            to commit your change (by setting Commit-Queue +2 on the code review
            page). For exceptional cases where you might not use the CQ, see
            [Directly committing
            code](/developers/contributing-code/direct-commit).
3.  If you need to revert your change, use the *Revert* button on the
            code review page.
4.  Some types of mechanical changes can be landed without requiring
            full owners approval. See [Mechanical
            Changes](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/code_reviews.md#mechanical-changes)
            docs.

In short, do the right thing for the project, not the easiest thing to get code
committed, and above all: use your best judgement.

Don't be afraid to ask questions. There is always someone on Slack who can help
you.

**Changes with multiple reviewers**

There are occasionally changes with a lot of reviewers on them, since sometimes
several people might need to be in the loop for a change because of multiple
areas of responsibility and expertise.

The problem is that without some guidelines, there's no clear responsibility
given in these reviews.

If you're the sole reviewer on a change, you know you have to do a good job.
When there are three other people, you sometimes assume that somebody else must
have looked carefully at some part of the review. Sometimes all the reviewers
think this and the change isn't reviewed properly.

In other cases, some reviewers say "LGTM" for a patch, while others are still
expecting changes. The author can get confused as to the status of the review,
and some patches have been checked in where at least one reviewer expected
further changes before committing.

At the same time, we want to encourage many people to participate in the review
process and keep tabs on what's going on.

So, here are some guidelines to help clarify the process:

1.  When a patch author requests more than one reviewer, they should
            make clear in the review request email what they expect the
            responsibility of each reviewer to be. For example, you could write
            this in the email:
    larry: bitmap changes
    sergey: process hacks
    everybody else: FYI
    In this case, you might be on the review list because you've asked to be in
    the loop for multiprocess changes, but you wouldn't be the primary reviewer
    and the author and other reviewers wouldn't be expecting you to review all
    the diffs in detail.
2.  If you get a review that includes many other people, and the author
            didn't do (1), please ask them what part you're responsible for if
            you don't want to review the whole thing in detail.
3.  The author should wait for approval from everybody on the reviewer
            list before checking in.
4.  People who are on a review without clear review responsibility (i.e.
            drive-by reviews) should be super responsive and not hold up the
            review. The patch author should feel free to ping them mercilessly
            if they are.
5.  If you're an "FYI" person on a review and you didn't actually review
            in detail (or at all), but don't have a problem with the patch, note
            this. You could say something like "rubber stamp" or "ACK" instead
            of "LGTM." This way the real reviewers know not to trust that you
            did their work for them, but the author of the patch knows they
            don't have to wait for further feedback from you. Hopefully we can
            still keep everybody in the loop but have clear ownership and
            detailed reviews. It might even speed up some changes since you can
            quickly "ACK" changes you don't care about, and the author knows
            they don't have to wait for feedback from you.
