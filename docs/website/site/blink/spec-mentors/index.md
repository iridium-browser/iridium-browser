---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: spec-mentors
title: Chromium Specification Mentors
---

*Quick link:* [mentor request
form](https://docs.google.com/forms/d/e/1FAIpQLSfCYE4U13GkZ11rAWBUjOb3Lza-u4m2k3TklQHXe7Zfn3Qo1g/viewform)

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

## How we work

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

## Can I join?

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