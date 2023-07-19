---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/guidelines
  - Guidelines and Policies
page_name: web-platform-changes-guidelines
title: Blink Values in Practice
---

[TOC]

## Introduction

The Blink project's [core values](/blink/guidelines/values) are to promote a
useful and thriving web, while being a good user agent and safeguarding the
openness of the web. Our primary lever for doing this is in deciding what
features to ship or unship to our users, which benefits our users directly.

Unlike many other platforms, the web has multiple implementations, which is
useful for maintaining its openness. Blink developers try their best to work
with those other implementations to ship a consensus feature all at
approximately the same time. When we can do this, the web platform moves forward
smoothly.

Other times, the priorities and values of the various implementations don't
align so quickly.
The Chromium project has been blessed with a large ecosystem of active
contributors. As such, we can take on the responsibility of proving out features
in our engine first, trialing them with users of Blink-based browsers ahead of
general deployment across all engines and to more web users. If other engines do
then deploy the new features, this increases the interoperable surface area of
the web, which improves the experience of those users we do not directly serve.

For this to work, we have to ship each feature in a way that incorporates broad
feedback and makes it easy for other engines to implement if and when they
decide the feature is valuable for their users.

This document describes how the Blink project uses and evaluates the
requirements in our [launch process](/blink/launching-features) to ensure that
new features put our values into practice.

## Finding balance

For all browser developers, there is an inherent tension between moving the web
forward and preserving interoperability and compatibility. On the one hand, the
web platform API surface must evolve to stay relevant. On the other hand, the
web's primary strength is its reach, which is largely a function of
interoperability. And by definition, when any browser ships a new feature, the
API change is not yet interoperable. So we need to balance some key risks while
we improve the web platform.

<style>
  dfn { font-weight: bold; font-style: normal; }
</style>

<dfn id="interoperability-risk">Interoperability risk</dfn> is the risk that
browsers will not eventually converge on
an interoperable implementation of the proposed feature. Interoperability cannot
be determined only at a given point in time; since browsers ship features at
different times, there is always a degree of non-interoperability on the web.
Instead, we are concerned with long-term interoperability risk, which we
forecast by observing the public behaviors of others in the web ecosystem, and
we work to minimize via the launch artifacts discussed below. See the [Blink
principles of
interoperability](https://docs.google.com/document/d/1romO1kHzpcwTwPsCzrkNWlh0bBXBwHfUsCt98-CNzkY/edit)
for a more in-depth discussion.

<dfn id="compatibility-risk">Compatibility risk</dfn> is the likelihood that a
change will break existing web
content loaded in Chromium. Compatibility risk is especially common with API
removal, but it is also a factor when adding new features: before shipping, we
need to be as sure as we reasonably can that the feature will not change or
evolve in backward-incompatible ways in the future, as such incompatible changes
cause direct harm to our users. Additionally, there can be cases where even new
features interact with old ones to cause [strange compatibility
issues](https://groups.google.com/a/chromium.org/g/blink-dev/c/BytHPljnifk/m/PiKCn3Ix6IIJ).
See the [Blink principles of web
compatibility](https://docs.google.com/document/d/1RC-pBBvsazYfCNNUSkPqAVpSpNJ96U8trhNkfV0v9fk/edit)
and [Web compat analysis tools](/blink/platform-predictability/compat-tools) for
more on this subject.

In an ideal world, all changes would both dramatically move the web forward, and
involve zero interoperability and compatibility risk. In practice, this is
rarely the case, and a tradeoff needs to be made:

* If a change has low interop/compat risk and significantly moves the web
  forward, Chromium welcomes it. (Example: [shipping CSS
  Grid](https://groups.google.com/a/chromium.org/g/blink-dev/c/hBx1ffTS9CQ/m/TMTigaDIAgAJ).)

* If a change has low interop/compat risk but isn't expected to significantly
  move the web forward, Chromium usually still welcomes it. (Example: [moving
  prefixed event handler properties
  around](https://groups.google.com/a/chromium.org/g/blink-dev/c/4Fidt4JqkTk).)
  Occasionally, Chromium will reject changes in this bucket to avoid technical
  complexity (e.g., removing our implementation of [KV
  Storage](https://www.chromestatus.com/features/6428344899862528)).

* If a change has high interop/compat risk and isn't expected to significantly
  move the web forward, Chromium will usually not welcome it. (Example: [not
  shipping canvas
  supportsContext](https://groups.google.com/a/chromium.org/g/blink-dev/c/n1LP6cE2or4).)

* If a change has high interop/compat risk but is expected to significantly
  move the web forward, Chromium will sometimes welcome it after a careful,
  publicly-considered explanation, accompanied by the artifacts discussed
  below. (Example: [shipping
  WebUSB](https://groups.google.com/a/chromium.org/g/blink-dev/c/KuXx_k2KIis).)

## The role of the API OWNERS

To find this balance and resolve the above tradeoffs on a per-change basis,
Chromium delegates to a trusted group, the [Blink API
OWNERS](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/API_OWNERS),
to decide what web platform features are delivered to our users. Concretely,
they are the final approvers in the [Blink process](/blink/launching-features),
where Chromium contributors present a series of artifacts as part of their
Intent to Prototype/Experiment/Ship.

These artifacts are a key part of the process, and much of the rest of this
document is spent detailing them and why we think they are important enough to
make them requirements before shipping a feature. Because of the amount of
resources committed to driving the web forward through Chromium, we accept that
the bar should be higher for us—we can't expect, time and again, that the burden
of designing new features will be split equally with others. So, we need to do
as much as we can to set up and encourage others to participate.

We have not always done this perfectly in the past, and to be honest, it’s
unlikely we’ll always do it perfectly in the future. But we commit to trying to
work well with others—even when we are implementing features ahead of others.

## Launch artifacts that Chromium values

### Explainers

[Explainers](https://tag.w3.org/explainers) are a proposal's first contact
with the world. Well-written and comprehensive explainers help other interested
parties judge the value of a feature, by presenting the use cases, the research,
and the tradeoffs involved in the design space. Notably, an explainer needs to
assume minimal previous domain knowledge in order to serve these goals well.

In particular, an explainer should present a living record of:

* What problem we are solving.

* Why we think that problem is important to solve.

* What the impact of solving the problem would be.

* Over time, the explainer will develop a specific solution proposal, with
  supporting arguments for why that proposal is the best among alternatives.

* Detailed discussion of any concerns raised by other implementers or by
  wide-review groups, summing up any open questions or conclusions reached.
  (This often ends up in the "FAQ" or "Alternatives considered" sections, or
  in other natural locations like the "Security and privacy considerations".)

Explainers should be documents that other implementers can easily read or skim
to figure out whether they think a feature is worth investing in. If these other
implementers have the time to do so, an explainer repository also serves as an
entry point for them to engage in the early design process. Such early
engagement is excellent news: it increases the chance of the feature launching
to other engines' users, sooner.

### Specifications

A good specification is critical to other implementations being able to
interoperably implement a feature, and to that feature eventually reaching users
of those other implementations. Although our code is open source, and in theory
anyone could implement based on it, good specifications have the following
benefits:

* Specifications operate at a higher level than source code, using
  implementation-agnostic abstractions. Thus, an implementer working on any
  implementation can read a specification and understand how it would map to
  their implementation, separate from the Chromium one.

* Similarly, specifications are reviewable by non-implementers, including web
  developers and groups providing wide review. Writing a specification makes
  it easier to capture the input of such groups.

* Writing a specification crystalizes what the intended behavior is, separate
  from the implemented behavior.

* Specifications provide a mechanism for IPR coverage. The amount of coverage
  varies depending on specification venue, but most venues at the very least
  guarantee that the specification writers' organizations grant a patent license
  to implement the specification. Before shipping, we have to host the
  specification somewhere with at least that minimal guarantee (e.g. a W3C CG,
  TC39, an IETF Individual-Draft, etc.), and we have to offer to migrate it to a
  standards track (e.g. W3C Recommendation, WHATWG Living Standard, Ecma
  Standard, etc.), which would gather more IPR coverage. Beyond that, the choice
  of venue doesn't affect our process.

As such, Chromium developers must ensure that any feature they intend to ship is
backed by a specification (and [tests](#tests)) that is complete and thorough
enough that a second engine can get a good start on implementing the feature
interoperably.

The goal of a web _standard_ is to be clear enough to develop a new
interoperable implementation based just on the standard, but it's impossible to
be this thorough for features that haven't yet been implemented multiple times.
We always expect the second implementers to find mistakes and areas of the
specification that assume the context of the first implementation.

Instead, we require that an experienced spec author be able to read the
specification without finding aspects that would confuse a second implementer.
To help get your specification to this level and check if it's there, we have
the [Chromium Specification Mentors](/blink/spec-mentors) program, which pairs
Chromium developers with a trusted mentor to get a second set of eyes.

Finally, the most important part of specification writing is to keep up active
maintenance of the specification even after Chromium ships the feature. This is
especially true if a second implementer starts implementing and filing bugs
based on their experience.

### Tests

By contributing [web platform tests](https://github.com/web-platform-tests/wpt)
for our proposals, we create a machine-checkable subset of the specification. In
practice, this is essential to achieving interoperable implementations, as well
as ensuring compatibility through preventing regressions. Of note is the
[wpt.fyi](https://wpt.fyi/) dashboard, which enables in-depth comparison of
cross-browser results and a resulting drive toward interoperability.

Additionally, by devoting the Chromium project's resources to writing web
platform tests, we ensure that other implementations have a ready-made test
suite when they go to implement the feature. This is excellent leverage: we can
invest our resources in writing the tests, but they can be used by all future
implementations as well, almost for free.

Chromium developers should strive for high levels of coverage for their
feature's web platform tests suite. Ideally, something like 100% coverage of
both "spec lines" and Chromium code would ensure that Chromium implements the
spec faithfully, and that others will be able to leverage the tests to implement
the spec in the same way. Some sample test suites which approach this goal
include
[url/](https://wpt.fyi/results/url?label=master&label=experimental&aligned),
[streams/](https://wpt.fyi/results/streams?label=master&label=experimental&aligned),
and
[pointerevents/](https://wpt.fyi/results/pointerevents?label=master&label=experimental&aligned).

In practice, this is a hard goal to reach. We can try to approach it by
[annotating specifications](https://tabatkins.github.io/bikeshed/#wpt-element)
with their associated tests, and running the appropriate Chromium [code coverage
tools](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/code_coverage.md).
Another hurdle is that some things are [not
testable](https://github.com/web-platform-tests/wpt/issues?q=is%3Aopen+is%3Aissue+label%3Atype%3Auntestable)
with current infrastructure. We have a dedicated team,
[ecosystem-infra@chromium.org](https://groups.google.com/a/chromium.org/g/ecosystem-infra),
which works to improve our testing infrastructure and mitigate such gaps.

### Web developer and end user feedback

The best evidence for the value of a feature is testimonials or data from users
and web developers. Before launch, these can be gathered in a variety of ways,
including [Dev
Trials](https://docs.google.com/document/d/1_FDhuZA_C6iY5bop-bjlPl3pFiqu8oFvuK1jzAcyWKU/edit),
[Origin Trials](https://github.com/GoogleChrome/OriginTrials), or direct user
and developer engagement in venues like the Chromium or specification issue
tracker. After launch, metrics such as [use
counters](https://www.chromestatus.com/metrics/feature/popularity) can show how
much uptake a feature is getting in the wild.

Collating this information together for easy consumption is an important final
step in the process before shipping. The goal is to present a body of evidence
that makes it easy for other engines to evaluate whether they would like to
bring the proposal to their implementation, or not.

### Browser engine reviews

The Blink process requires most features to ask for [vendor
signals](https://bit.ly/blink-signals) because they can give us some of the most
direct indications of [interoperability risk](#interoperability-risk) and
[specification](#specifications) quality. The feedback we get from the other
vendors comes in several categories, which get different sorts of attention from
the API owners.

* *Spec quality*: When other implementers point out parts of the
  [specification](#specifications) that are ambiguous or under-specified, the
  feature team needs to work to fix those problems promptly. Fortunately, while
  this type of issue takes developer time to fix, it doesn't in itself require
  changes to the implementation and so usually doesn't delay shipping. These
  problems can block the other implementer from doing a complete review, so more
  issues will sometimes come up after the spec's quality is improved.
* *Unimplementable designs*: Other implementers might also tell us that our
  current design is either unimplementable on some operating systems or
  incompatible with some browser architecture choices. These are serious
  problems that can justify rethinking an API even after it's shipped.
* *Ergonomic concerns*: Any reviewer, including other implementers, can notice
  that an API might not work well enough for developers. Our [origin
  trial](/blink/origin-trials) process is designed to catch this class of
  problems before we get to the I2S stage, but it doesn't always. We have to
  evaluate this feedback against any evidence we have, and try to find consensus
  around one of the options. If this feedback comes up *after Chromium has
  shipped* a feature, unless the feedback is obviously correct we can expect it
  to include evidence from the developers who have been able to try the feature
  with live traffic.
* *Values disagreements*: Sometimes other implementers make different
  tradeoffs about which Web features are worth adding. In these cases, we need
  to work to fully explore the disagreement and try to find a compromise
  that could enable all browsers to satisfy the use case. However, if no
  compromise is found, and the feature significantly advances the Web even
  without interoperability, the API owners will often approve it despite the
  other browser engines' opposition.

### Wide review

There are specific groups, usually in the specification ecosystem, which are
dedicated to reviewing feature proposals. The [W3C
TAG](https://github.com/w3ctag/design-reviews/) is one such group, whose reviews
have a formal place in the Blink launch process. But the more such reviews we
can gather, the better.

Other groups to consider are:

* Relevant working groups or specification editors, e.g. the CSSWG for CSS
  feature proposals; the HTML Standard editors for HTML feature proposals;
  etc.

* Cross-functional W3C groups such as the [Internationalization
  WG](https://www.w3.org/International/core/Overview), [Privacy
  IG](https://www.w3.org/Privacy/IG/), or [APA
  WG](https://www.w3.org/2018/08/apa-charter).

* Chromium cross-functional review groups, such as the
  [security](/Home/chromium-security), privacy, permissions, and anti-tracking
  teams.

None of these groups are obligated to respond; many are busy with other reviews
or with their own work. But we always attempt to reach out, and respond to any
concerns or questions raised. And then we work to capture any answers, or
resultant changes, in a permanent location like the specification or explainer.

## Conclusion

Notably, these guidelines do not have an explicit requirement that a feature be
standardized (as opposed to specified), or that a feature have multi-implementer
consensus. Instead, factors like those are inputs into the overall evaluation.
Indeed, all engines implement features ahead of others, or ahead of formal
standardization. (See the "Browser-Specific" section of the [Web API Confluence
Dashboard](https://web-confluence.appspot.com/#!/confluence)). But our process
requires significant up-front investment from Chromium contributors before a
feature is considered ready to ship, to help promote our
[values](/blink/guidelines/values).

Even if other browsers have not yet implemented a feature, these artifacts are
important to the Chromium project. Creating solid explainers, specs, and tests,
and gathering feedback and wide review, can move us toward future
interoperability, and decrease the risk in the eyes of the API OWNERS. Indeed,
even if other browsers are *opposed* to a feature, by taking the steps above, we
can confidently state that we minimized risk, and layed out a well-lit path in
case the other browsers change their evaluation or their priorities in the
future. This shows that in addition to a feature promoting a useful and thriving
web, and being designed well with respect to the priority of constituencies, we
have made a strong effort to ensure the feature upholds the web's open nature as
well.
