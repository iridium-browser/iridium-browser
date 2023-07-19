---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: spec-mentors
title: Chromium Specification Mentors
---

*Quick link:* [mentor request
form](https://docs.google.com/forms/d/e/1FAIpQLSfCYE4U13GkZ11rAWBUjOb3Lza-u4m2k3TklQHXe7Zfn3Qo1g/viewform)

[toc]

## Introduction

Introducing a new feature to the web platform requires writing a specification,
which is a separate skill set from writing code. It involves API design,
cross-company collaboration, and balancing the needs of the web's various
stakeholders.

Specification mentors apply their experience in this area to ensure that
explainers and specifications put out by the Chromium project are of high
quality and ready to present to the world. For folks new to the standardization
process, mentors can provide guidance and review. And for those who are already
experienced, mentors provide the equivalent of code review: a second perspective
to help raise questions early or spot things you might have missed.

This process aims to improve the quality of explainers and specifications, and
thus uphold the Chromium project's commitment to an open, interoperable, and
well-designed web platform. It should also make the process of launching a new
feature more predictable and less painful for Chromium engineers.

## For feature owners

### Pairing up with your mentor

Before sending an Intent to Prototype, you are encouraged to find a spec mentor
to work with. They can review your explainer, as well as the Intent to Prototype
itself, to make sure your feature is presenting a good face to the world.

For Googlers, a specification mentor is required at this stage. For other
Chromium contributors, you're welcome to reach out if you find one helpful.

To find a specification mentor, you can draw upon your existing contacts (e.g.,
your team lead or coworkers), or you can [fill out our
form](https://docs.google.com/forms/d/e/1FAIpQLSfCYE4U13GkZ11rAWBUjOb3Lza-u4m2k3TklQHXe7Zfn3Qo1g/viewform)
with the relevant information. In the latter case, we will get back to you with
a proposed mentor within 2 business days; we want to make sure your Intent to
Prototype proceeds as quickly as possible.

### What to expect

Your mentor will be available for you to ask questions or ask for reviews
throughout the lifetime of your feature. In particular, if you would like
someone to review your explainer or specification work, you can collaborate with
them directly, e.g. using video calls, GitHub pull request reviews, or Google
Docs.

There are three specific points at which you'll want to request detailed
specification review from your mentor, so that they can help ensure that your
public artifacts are high-quality:

- (Optional) For your explainer and TAG review request, before sending them
  out in the Intent to Prototype process.

- For your explainer and specification, before beginning a [Dev Trial](https://docs.google.com/document/d/1_FDhuZA_C6iY5bop-bjlPl3pFiqu8oFvuK1jzAcyWKU/edit)
  or [Origin Trial](https://github.com/GoogleChrome/OriginTrials), at which
  point these artifacts will be seen by wide review groups and other browser
  vendors.

- For your specification, before sending an Intent to Ship.

You can also call on your mentor to review the Intents themselves, before you
send them off to blink-dev and the scrutiny of the API owners and the wider
world.

Your mentor can optionally reply to the Intent to Prototype/Experiment/Ship
threads with a summary of how the review went. This can help bolster the API
owners' confidence in the explainer or specification. For example:

> "The use cases for this feature make a lot of sense. I raised an issue to
> consider some alternate approaches, and noted a potential privacy risk that
> should be at least discussed, and ideally mitigated. We agreed to keep these
> in mind as the prototyping progresses."

or

> "This feature and the proposed API both look good to ship from my perspective.
> I filed a series of small issues on potential API improvements, which have
> been incorporated. And we tightened up the specification language around the X
> algorithm, which now has extensive web platform tests for the
> previously-ambiguous edge cases."

If all goes well, then by the time you reach the Intent to Ship stage, your
explainer and spec will have been refined through mentorship and review to be
the best they can be. This will put you in a strong position with regard to some
of the most-often-problematic parts of the Intent to Ship, such as the the
Interoperability & Compatibility risks section, and thus smooth the path toward
API OWNER approval.

## For spec mentors
### Can I join?

Yes, please do! Becoming proficient in design reviews is a core engineering
skill, and one of the best ways to do that is to help other Chromium project
members with their explainers and specifications. And it's important for the
health of the Chromium community to have as many engineers as possible who
understand and can work successfully within the standards process.

If you've ever written an explainer or specification before, you've probably
gotten a good amount of feedback from various audiences, both internal and
external. That means you're qualified to help others through the same process,
to pass on what you have learned. You won't be alone: the other mentors are
around to help with anything you're not sure of.

The time commitment for being a specification mentor should be similar to a
commensurate amount of Chromium code reviews, i.e., not that bad.

To join the program and start getting assigned features to help mentor,
subscribe to our mailing list, at
[spec-mentors@chromium.org](https://groups.google.com/a/chromium.org/g/spec-mentors).
When new features come in, feel free to reply that you'd be willing to help;
otherwise, [domenic@chromium.org](mailto:domenic@chromium.org) will assign
features to mentors according to his best judgment.

### How do I mentor?

Your job as a spec mentor is to teach a new person how to get through the
process of writing a specification for a feature being developed in Chromium, or
to improve their knowledge if they've already done it a couple times. You
shouldn't write their specification for them. You don't need to help them
navigate the parts of the launch process that don't intersect with the standards
process. You should try to be aware of their schedule and point out things
they're doing too late.

If you ever don't know the answer to a question, mail the other spec mentors at
[spec-mentors@chromium.org](mailto:spec-mentors@chromium.org).

Here's what to do at some key phases of the spec development process:

#### Starting out

Make sure your feature owner has [created a Chrome Status
entry](https://chromestatus.com/guide/new) and assigned you as their mentor.
Being listed as their mentor will allow you to edit their entry.

#### Reviewing the explainer

Check that the feature's explainer follows the [W3C TAG's guidance for writing
good explainers](https://tag.w3.org/explainers/), especially by focusing on what
problems the feature will solve for end users. Especially in the initial stages,
the feature team should be open to adopting alternative solutions, and the
explainer should make that clear, e.g., by documenting alternatives considered
or calling out areas where better ideas are appreciated. If the feature team
seems too attached to their initial design, you should help coach them to be
more flexible.

#### Picking an incubation venue

Migrating to an incubation venue gives potential external contributors assurance
about how their suggestions will be treated and what protection they have for
using the intellectual property (IP) in the feature's definition. To pick an
incubation venue, think about which standards body the feature is eventually
likely to migrate to. You don't have to be certain at this stage; things can
move around. Each standards body has their own way of doing incubation:

* W3C: See if any existing [community
  group](https://www.w3.org/community/groups/) looks like a good fit. If none
  do, have your feature owner [start off in the
  WICG](https://github.com/WICG/proposals). If they find out later that it would
  help to have a dedicated group, it's easy to ask the [WICG
  chairs](https://wicg.github.io/admin/charter.html#chairs) to migrate an
  incubation.
* WHATWG: The WHATWG doesn't have an incubation process. Use the W3C's
  incubation process.
* IETF: The IETF doesn't have specific venues set up for incubation, but you get
  similar IP protection by [publishing an idea as an Internet Draft to the
  datatracker](https://datatracker.ietf.org/submit/). You can host this draft
  either in a personal repository or in the WICG. You invite people to comment
  on your proposal by emailing a relevant [working
  group](https://datatracker.ietf.org/wg/)'s list.
* TC39: TC39's [stage 0](https://tc39.es/process-document/) is their incubation
  stage.

#### Reviewing the specification

You should do a complete review of the specification between the ["dev trials"
stage](/blink/launching-features/#dev-trials) and when the team plans to send
their [Intent to Ship
email](/blink/launching-features/#new-feature-prepare-to-ship), help the team
fix any problems you find, and be ready to summarize the specification's quality
in the I2S thread or privately to the API owners. The API owners will use your
summary in order to inform their decisions about any tradeoffs between schedule
pressure and spec quality, but you aren't responsible for making those
decisions.

If you're unsure about any of this review, email for help on the
[spec-mentors@chromium.org](mailto:spec-mentors@chromium.org) list, and another
mentor will be happy to help you.

Look for the following things, in addition to using your general good judgement
and experience with writing specifications:

* The specification should be clear enough that someone not familiar with any
  specific implementation of the feature could write a compatible new
  implementation from the specification.

* The specification and API design should follow the guidance in:
  * [Infra](https://infra.spec.whatwg.org/)
  * The [Web Platform Design Principles](https://w3ctag.github.io/design-principles/)
  * Our [informal document of collected API
    guidance](https://docs.google.com/document/d/1xOzSO7CqD9AkGHAM9a_vViaY6oT7ApnwoZodYI0DNP0/edit#).
    Feel free to add useful tips to this document as you notice them.

* The specification should define all the new terms it introduces and link to
  all the terms it uses.

* The behavior should be specified using algorithms that are approximately as
  rigorous as existing features of the web platform. External references from
  W3C and WHATWG specifications should follow the [W3C's normative references
  guidelines](https://www.w3.org/2013/09/normative-references).

* If the specification proposes a change to another web standard, it should
  [label that change as a monkeypatch and organize it so that it will be easy to
  merge into the upstream specification in the
  future](https://www.w3.org/TR/design-principles/#cant-avoid-monkey-patching).
  * By the time of the [Intent to Ship
  email](/blink/launching-features/#new-feature-prepare-to-ship), there should be
  an issue on the upstream spec proposing to merge the change.

* If the specification you're reviewing calls into
  [Fetch](https://fetch.spec.whatwg.org/), check that it follows the guidance in
  the [Fetch section on using it from other
  standards](https://fetch.spec.whatwg.org/#fetch-elsewhere), and if there's
  anything you're not sure of, ask one of the Fetch editors for help.

You're not expected to discover ergonomic issues or aspects of the proposed
feature that could make it "bad for the Web", but if you do notice such
problems, you should discuss them with the feature team and consider escalating
to the [API owners](/blink/guidelines/api-owners). It's also not your
responsibility to decide whether a feature should ship despite spec quality
issues; just to ensure the decision-makers are aware of any problems.
