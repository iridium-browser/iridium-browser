---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: directory-dependency-in-blink
title: Directory dependency in Blink (Still DRAFT)
---

## NOTE: This plan is not decided yet and is still a work in progress.

## Overview

In 2015 Sep, we merged the Blink repository into the Chromium repository. The
repository merge enabled us to add dependencies from Blink to Chromium and
opened a door for significantly simplifying abstraction layers that had been
needed to connect Blink with Chromium. However, adding random dependencies from
Blink to Chromium will break layering and just mess up the code base. We need a
guideline about it.

Before the repository merge, Blink had the following dependency. Public APIs
were the only way to connect Blink with Chromium's content layer.

[<img alt="image"
src="/blink/directory-dependency-in-blink/before_merge_resized.png">](/blink/directory-dependency-in-blink/before_merge_resized.png)

After the repository merge, we're planning to introduce direct dependencies from
Blink to Chromium (e.g., wtf/ =&gt; base/) to simplify the interactions. To
introduce a new dependency, you need to follow the following guideline.

## Guideline

1.  Send an email to blink-dev@ to propose the dependency. The email
            needs to explain the advantages of introducing the dependency.
2.  Get a consensus on blink-dev@. If people can't get agreement in the
            discussion, public/OWNERS should give advice.
3.  Add the dependency to the following "Allowed dependencies from Blink
            =&gt; Chromium" section.

We do want to avoid introducing random dependencies and breaking layering.
Unless introducing the dependency is going to have a large benefit (e.g., remove
a lot of abstraction classes, remove a lot of code duplication etc), you should
instead consider just using public APIs. After the repository merge, it is
pretty easy to add/remove/modify public APIs because you no longer need
three-sided patches. In particular, remember that:

*   Blink and Chromium have different assumptions and priorities on
            performance, memory and code health. So we may not want to share
            implementations too much. For example, we may want to keep standard
            libraries in wtf/ such as Vectors and Strings until we’re pretty
            sure that it is really a win to replace them with base/
            alternatives.
*   This kind of refactoring requires a substantial amount of
            engineering resources. We should keep in mind the opportunity cost.

## Allowed dependencies from Blink =&gt; Chromium

(Once the intent-to-implement is approved, please add the dependency to the
following list. You can also add a link to a discussion log in blink-dev@,
design documents etc.)

*   ...
*   ...
*   ...

## Discussions

If you want to know more background about the dependency design, see the
following discussions.

*   [Removing the WTF namespace
            (blink-dev@)](https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/B29AVrZy1Z4/discussion)
*   [Proposal for the future dependency model in Blink
            (blink-dev@)](https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/e4y_6y0Nlp8/discussion)
*   [Proposal for a future Blink architecture (design document that
            summarized people's thoughts as of 2015
            May)](https://docs.google.com/document/d/1gwQ1qn3Qx1U_xm9BbwIsJz0GP_05KUXVHhk3PxeNuSw/edit#)
