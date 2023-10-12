---
breadcrumbs:
- - /getting-involved
  - Getting Involved
page_name: become-a-committer
title: Become a Committer
---

[TOC]

## What is a committer?

Technically, a committer is someone who has write access to the Chromium src Git
repository. A committer can submit their own patches or patches from others.
A committer can also review patches from others, though all patches need
to either be authored by or reviewed by an
[OWNER](https://chromium.googlesource.com/chromium/src/+/main/docs/code_reviews.md#owners-files)
as well.

This privilege is granted with some expectation of responsibility: committers
are people who care about the Chromium projects and want to help them meet their
goals. A committer is not just someone who can make changes, but someone who has
demonstrated their ability to collaborate with the team, get the most
knowledgeable people to review code, contribute high-quality code, and follow
through to fix issues (in code or tests).

A committer is a contributor to the Chromium projects' success and a citizen
helping the projects succeed. See [Committer's
responsibility](/developers/committers-responsibility).

What is written below applies to the main Chromium source repos; see the note at
the bottom for ChromiumOS, which has different policies. For some other Chromium
repos (e.g., the infra repos), we follow the same policies as the main Chromium
repos, but have different lists of actual committers. Certain other repos may
have different policies altogether. When in doubt, ask one of the
[OWNERS](https://chromium.googlesource.com/chromium/src/+/main/docs/code_reviews.md#owners-files)
of the repo in question.

## Becoming a committer

In a nutshell, contribute 10-20 non-trivial patches in the [Chromium
src](https://chromium.googlesource.com/chromium/src/) Git repository, and get at
least three different people to review them (you'll need three people to support
you). Then ask someone to nominate you. You're basically demonstrating your

*   commitment to the project (10+ good patches requires a lot of your
            valuable time),
*   ability to collaborate with the team,
*   understanding of how the team works (policies, processes for testing
            and code review,
            [OWNERS](https://chromium.googlesource.com/chromium/src/+/main/docs/code_reviews.md#owners-files)
            files, etc),
*   understanding of the projects' code base and coding style, and
*   ability to write good code (last but certainly not least)

A current committer nominates you by sending email to committers@chromium.org
containing the following information. Please do not CC the nominee on the
nomination email.

*   your first and last name
*   your email address. You can also ask to get an @chromium.org email
            address at this time, if you don't already have one (see below).
*   an explanation of why you should be a committer,
*   embedded list of links to revisions (about top 10) containing your
            patches

Two other committers need to second your nomination. If no one objects in 5
working days (U.S.), you're a committer. If anyone objects or wants more
information, the committers discuss and usually come to a consensus (within the
5 working days). If issues can't be resolved, there's a vote among current
committers.

That's it! There is no further action you need to take on your part. The
committers will get back to you once they make a decision.

In the worst case, this can drag out for two weeks. Keep writing patches! Even
in the rare cases where a nomination fails, the objection is usually something
easy to address like "more patches" or "not enough people are familiar with this
person's work."

Once you get approval from the existing committers, we'll send you instructions
for write access to Git. You'll also be added to committers@chromium.org. If you
work for Google, you are expected to [become a
sheriff](/developers/tree-sheriffs) at this point as well (see the internal
instructions for how to add yourself to the rotations).

Historically, most committers have worked at least partially on the Chromium
core product and thus demonstrated C++ coding ability in their CLs, but this is
not required. It is possible to be a committer if you only work on other parts
of the code base (e.g., build and test scripts in Python), but you still have to
demonstrate that you understand the processes of the project with a list of CLs
that you've landed. Committership is primarily a mark of trust, and we expect
committers to only submit or approve changes that they are qualified to review.
Failure to do so may result in your committership being revoked (also see below
for other reasons that you might get your committership revoked).

Being a committer is something that a person is, not something an email
address or account is. This means that you can be a committer under multiple
email addresses, and you can change your address, without needing each
address to be re-nominated. To do so, send an email to accounts@chromium.org.

## Other statuses

If you just want to edit bugs, see: [Get Bug-Editing
Privileges](/getting-involved/get-bug-editing-privileges).

### Getting a @chromium.org email address

Many contributors to chromium have @chromium.org email addresses. These days, we
tend to discourage people from getting them, as creating them creates some
additional administrative overhead for the project and have some minor security
implications, but they are available for folks who want or need them.

At this time there are only a few fairly rare cases where you really need one:

*   If you are an employee of a company that will not let you create
            public Google Docs, and you wish to do so (e.g., to share public
            design specs). Often you can work around this by having someone with
            an account create the document for you and give you editing
            privileges on it.
*   If you need to do Chromium/ChromiumOS-related work on mailing lists
            that for some reason tend to mark email addresses from your normal
            email address as spam and you can't fix that (some mail servers are
            known to have problems with DMARC, for example).

It is also fine to get a @chromium.org address if you simply don't want your
primary email addresses to be public. Be aware, however, that if you're being
paid to contribute to Chromium your employer may wish you to use a specific
email address to reflect that.

You can get a @chromium.org email address by getting an existing contributor to
ask for one for you; normally it's a good idea to do this as part of being
nominated to be a committer. Include in your request what account name you'd
like and what secondary email we can use to associate it with (and what company
you are affiliated with, if you wish to make that clear; we track this
affiliating internally but it isn't publicly visible). People tend to match
usernames (for example, someone who usually uses email@example.com would ask for
email@chromium.org) to minimize confusion, but you are not required to do so and
some people do not.

### Try job access

If you are contributing patches but not (yet) a committer, you may wish to be
able to run jobs on the [try servers](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/infra/trybot_usage.md)
directly rather than asking a committer or reviewer to do so for you. There are
two potential scenarios:

You have an @chromium.org email address and wish to use it for your account:

*   If you have an @chromium.org email address, you most likely already
            have try job access. If for some reason you do not, please send an
            email to accounts@chromium.org with a brief explanation of why you'd
            like access.

You do not have an @chromium.org email address, or wish to use a different email
address. If this is your situation, the process to obtain try job access is the
following:

*   Ask someone you're working with (a frequent reviewer, for example)
            to send email to accounts@chromium.org nominating you for try job
            access.
*   You must provide an email address and at least a brief explanation
            of why you'd like access.
*   It is helpful to provide a name and company affiliation (if any) as
            well.
*   It is very helpful to have already had some patches landed, but is
            not absolutely necessary.

If no one objects within two (U.S.) working days, you will be approved for
access. It may take an additional few days for the grant to propagate to all of
the systems (e.g., Rietveld) and for you to be notified that you're all set.

Googlers can look up the committers list
[here](https://goto.google.com/chromium-committers).

## Maintaining committer status

A community of committers working together to move the Chromium projects forward
is essential to creating successful projects that are rewarding to work on. If
there are problems or disagreements within the community, they can usually be
solved through open discussion and debate.

In the unhappy event that a committer continues to disregard good citizenship
(or actively disrupts the project), we may need to revoke that person's status.
The process is the same as for nominating a new committer: someone suggests the
revocation with a good reason, two people second the motion, and a vote may be
called if consensus cannot be reached. I hope that's simple enough, and that we
never have to test it in practice.

In addition, as a security measure, if you are inactive on Gerrit for more than
a year, we may revoke your committer privileges and remove your email
address(es) from any OWNERS files. This is not meant as a punishment, so if you
wish to resume contributing after that, contact accounts@ to ask that it be
restored, and we will normally do so. This does not mean that we will shut off
your @chromium.org address, if you have one; that should continue to work.

If you have questions about your committer status, overall, please contact
accounts@chromium.org.

\[Props: Much of this was inspired by/copied from the committer policies of
[WebKit](http://www.google.com/url?q=http%3A%2F%2Fwebkit.org%2Fcoding%2Fcommit-review-policy.html&sa=D&sntz=1&usg=AFrqEze4W4Lvbhue4Bywqgbv-N5J66kQgA)
and
[Mozilla](http://www.google.com/url?q=http%3A%2F%2Fwww.mozilla.org%2Fhacking%2Fcommitter%2F&sa=D&sntz=1&usg=AFrqEzecK7iiXqV30jKibNmmMtzHwtYRTg).\]

## Chromium OS Commit Access (Code-Review +2)

Note that any registered user can do Code-Review +1. Only access to Code-Review
+2 in Chromium OS repos is restricted to Googlers and managed by team lead
[nominations](http://go/new-cros-committer-nomination).
