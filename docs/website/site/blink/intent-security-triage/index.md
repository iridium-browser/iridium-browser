---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: intent-security-triage
title: '"Intent to {Implement,Ship}" Security Triage'
---

Blink's [launch process](/blink#TOC-Web-Platform-Changes:-Process) ensures that
interesting features show up as "Intent" threads on the
[blink-dev@chromium.org](https://groups.google.com/a/chromium.org/group/blink-dev/topics)
mailing list. These threads provide a forum for discussion of new features, and
go/no-go decisions from API OWNERS, and are a pretty comprehensive view of the
feature set that we're planning on providing to developers.

Feature owners generally want the security team to sign off on features before
shipping them to the web, and benefit from a contact they can poke with security
questions. To that end, it behooves us to proactively skim through these threads
to give feedback early, when it's easily actionable. That's where you come in,
you wonderful security-minded person, you:

## Triage Workflow

    Visit <https://bit.ly/blinkintents> and find the as-yet untriaged "Intent"
    threads.

    Read through each feature proposed in an "Intent to Implement" or "Intent to
    Implement and Ship" thread, with an eye for security concerns or interesting
    side effects that the feature's author might not have considered.

    *Note: we're assuming here that anything at the "Intent to Ship" stage has
    gone through wide review, and that deprecation is generally
    security-positive. If those turn out not to be reasonable assumptions, we
    can reevaluate what threads we care about.*

    If you end up with questions, post them to the thread. In particular, it's a
    good idea to encourage developers to include an explicit "Security
    Considerations" section in their specification and to read through things
    like the TAG's [self-review
    questionnaire](https://w3ctag.github.io/security-questionnaire/) (bonus
    points for filing spec bugs if there's a clear way to do so).

    If substantial questions are raised, flagging the feature for wider review
    before launch is reasonable. This could range from a simple comment on the
    launch bug up through preemptively flipping the Launch-Security flag to
    "No", depending on how the conversation goes.

    If you determine that a particular feature has no security implications at
    all, head over to the launch bug and flip the Launch-Security flag to NA,
    along with a comment to that effect.

    At the end of your triage shift, send a report to
    [security-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/security-dev)
    with a short description of what you looked at, and how it went (the
    [net-dev@ triage
    thread](https://groups.google.com/a/chromium.org/d/msg/net-dev/zhd-eJLjGi0/9Z-83U6vCAAJ)
    is a great model for this).

## Hello, feature owner!

This triage rotation is not meant as an approval step. Lack of comments from the
security team on an "Intent" thread should not be interpreted as blanket
approval. It's meant simply as a mechanism to get more eyes on features earlier
in the process, and will hopefully speed up the process of getting those flags
flipped on your important launches.
