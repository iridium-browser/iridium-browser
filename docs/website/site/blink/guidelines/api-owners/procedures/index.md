---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/guidelines
  - Guidelines and Policies
- - /blink/guidelines/api-owners
  - Blink API owners
page_name: procedures
title: Blink API owner procedures
---

[Last updated via unanimous approval: August 3,
2021](https://groups.google.com/a/chromium.org/g/blink-api-owners-discuss/c/zeyQuh6Wk5E)

## LGTM Requirements

* Shipping a change to add, change or remove any substantial aspect of a web-exposed API requires 3 LGTMs.

* Starting or extending an Origin Trial requires 1 LGTM.

* Any non-standard Origin Trial require 3 LGTMs.

* Intents to Prototype do not require any LGTMs.

## Adding and removing API owners

An API owner may be added after 3 LGTMs, plus evidence of the new API owner
meeting the [requirements](/blink/guidelines/api-owners/requirements).

API owners may be removed if they are persistently failing to meet the
requirements as well as to review a reasonable number of the incoming intents.
They may be removed after a vote by an absolute majority of other API owners,
after at least 8 weeks notice (\*) to the API owner to be removed plus
[blink-api-owners-discuss@chromium.org](mailto:blink-api-owners-discuss@chromium.org),
and no formal objection from the API owner to be removed. A formal objection
triggers the majority vote mechanism.

(\*) The 8 week period starts from the point at which the notified API owner can
be reasonably expected to have received the message, and is not on a leave,
vacation etc.

API owners may resign at any time, via email to
[blink-api-owners-discuss@chromium.org](mailto:blink-api-owners-discuss@chromium.org).

## Formal objection

An API owner may formally object to any decision, through written communication
to
[blink-api-owners-discuss@chromium.org](mailto:blink-api-owners-discuss@chromium.org).
This triggers the majority vote mechanism.

## Majority vote

Any API owners decisions that cannot get the required LGTMs will be decided
through a majority vote.

Note: API owners are expected to work via consensus, and try hard not to cause
this situation. For reference: it has never, to date, been needed.

A “majority vote” consists of a formal vote among the API owners, at a
videoconference or in-person meeting attended by a majority of the API owners,
and announced to all API owners at least a week in advance, with option for
absentee votes. The result of this vote decides the issue.

If a vote occurs, the documented final vote (including who voted which way), and
description of the objection raised, must be published publicly on
blink-dev@chromium.org.

## What “LGTMs” means

An LGTM is recorded via email to
[blink-dev@chromium.org](https://groups.google.com/a/chromium.org/g/blink-dev).

**1 LGTM means:** An API owner LGTMed the proposal and no API owner formally objected.

**3 LGTMs mean:** 3 API owners LGTMed the proposal and no API owner formally objected.
