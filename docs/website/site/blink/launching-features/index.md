---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: launching-features
title: Launching Features
---

If you have concerns about your feature not fitting into this process, or any
other questions, general concerns or discussion of this process, please e-mail
[blink-api-owners-discuss@chromium.org](mailto:blink-api-owners-discuss@chromium.org).
If you'd like to provide feedback on this page or the process it describes,
leave feedback on the [Google Doc
draft](https://docs.google.com/document/d/1R_2V5_rukVM8S5Hg2XKxiEOVRxvB7NqZOI2lfni2YwE/edit?usp=sharing)
of this page.

## Exempt features

You do not need to use this process if your change does not [affect web API
behavior to the point that developers need to be aware of
it](/blink/guidelines/web-exposed). (e.g. no significant behavioral changes, and
no web-facing API changes.) The rest of this document doesn’t apply to this type
of change, although such features might still have to go through a different
launch process. Large projects should have public design docs that are also
shared on blink-dev@chromium.org (or chromium-dev@, for projects that have
significant parts outside of Blink) for feedback (this is also a good way to get
the attention of other relevant leads).

## Frequently asked questions

**Q**: *Do I need any of this if my project is just refactoring or
re-architecting the code? Do the [API owners](/blink/guidelines/api-owners) need
to be involved?*

**A**: No. The API owners oversee the **process** of shipping [web-exposed](/blink/guidelines/web-exposed) API changes. They are not necessarily leads or
overseers of any of the code. Instead, you should get the buy-in of the leads of
the code areas touched by your project. If there may be side effects of your
change, you should follow the "Architectural change" process below. In addition,
such large projects should have public design docs that are also shared on
[blink-dev@chromium.org](mailto:blink-dev@chromium.org) (or
[chromium-dev@chromium.org](mailto:chromium-dev@chromium.org), for projects that
have significant parts outside of third_party/blink) for feedback (this is also
a good way to get the attention of relevant leads you might not have thought
of).

For code-related questions, you can email
[platform-architecture-dev@chromium.org](mailto:platform-architecture-dev@chromium.org)
in addition to blink-dev@ as a catch-all option when the code ownership is not
clear or the feature needs large-scale refactoring changes.

**Q**: *What if I want to add some code to third_party/blink that is for a
non-web-exposed feature? Is that allowed?*

**A**: In general, no. On a case-by-case basis an exception could be given if
there is no other reasonable way to implement the feature. Ask for permission
from leads of the code area as well as the API owners. (The API owners need to
be involved only to help understand if the feature really is not web-exposed;
this can be a very subtle question.)

**Q**: *I am not sure of the right approach for my feature. What should I do?*

**A**: Please reach out to the API owners for help! While they are not
gatekeepers for everything, they are very happy to give advice and unblock your
feature. An email to
[blink-api-owners-discuss@chromium.org](mailto:blink-api-owners-discuss@chromium.org)
is the best way; if a public-facing email is not possible, please email the [API
owners](/blink/guidelines/api-owners) directly.

## The Feature Types

The first thing you will need to do is identify what type of feature you are
building:

### New feature incubation

This is the normal path when we are incubating and
defining new features from scratch - e.g., most
[Fugu](/teams/web-capabilities-fugu) features follow this pattern, and any
feature where we start without an already-defined standard for the feature.
This also covers changes that are extending existing features (e.g.,
defining a new value and behavior for an existing standard feature). This
type of feature has the most associated process steps, as it is charting new
territory.

### Implementation of existing standard

This type of feature has a
lighter-weight “fast track” process, but this process can only be used when
the feature has already been defined in a consensus standards document -
e.g. a W3C Working Group has already agreed on the design, it has already
been merged into a WHATWG standard, or the feature has already been
implemented in another engine.

### Deprecation

Removal of already-shipped features.

### Web developer facing change to existing code

This is generally a public
service announcement- “This is a web-developer-facing change to existing
code without API changes, but you may see side effects.” This may be due to
large-scale code refactoring or rewriting, where the goal is to cause no
behavioral changes (but due to scope of change, side effects are likely), or
this may encompass changes to current code that fix a bug or implement new
changes to the spec without changes to the API shape itself.

### Changing stages

It is possible to change types later in the process - for example, if you start
out implementing an already existing standard, but discover you need to incubate
a new API during the process, you can change the feature type and move back
stages. Note that there are few [strictly required gates to the Chromium
process](/blink/guidelines/api-owners/procedures) (e.g., 3 LGTMs from API owners
on an intent-to-ship) - particularly in the earlier stages we want to encourage
experimentation. However, there are required fields for most stages in the
[ChromeStatus tool](https://www.chromestatus.com/), and it’s important to
consider all the steps listed below; this will maximize your chance of success
on an intent-to-ship and reduce the risk of having to redo parts of your design
or implementation.

## The Chromium process to launch a new feature

### Step 0: Create a ChromeStatus entry, and choose your feature type.

For all types of features, the first step is to create a ChromeStatus entry. Go
to <https://www.chromestatus.com/features>, ensure you are logged in (see the
upper right corner), and click “Add new feature”. If you do no have access to
create a new feature, please send an email to
[webstatus-request@google.com](mailto:webstatus-request@google.com) to request
access. Follow the directions to name your feature and give a short summary, and
select the appropriate feature type.

For Chrome, some launches will require a formal[ Chrome launch
review](https://bugs.chromium.org/p/chromium/issues/entry?template=Chrome+Launch+Feature)
(especially if your feature has security, privacy, legal, or UI implications).
This is the point where you should file a launch bug if this applies to you. We
are working on making this process more open and transparent outside Google.

From this point on, the process changes a little depending on the type of
feature you’re adding.

### For New Feature Incubations

#### Step 1: Incubating: Write up use cases and scenarios in an explainer

Press the "Start" button next to "Start Incubating", fill out the “Motivation”
section with a brief summary, and then write up the use cases and scenarios in
an explainer for the feature (typically hosted in a public personal Github repo
in Markdown form) and hit "Submit".

We have a [program for to provide mentorship for specification
writing](/blink/spec-mentors); If you are a Googler you must file a
[request](https://docs.google.com/forms/d/e/1FAIpQLSfCYE4U13GkZ11rAWBUjOb3Lza-u4m2k3TklQHXe7Zfn3Qo1g/viewform)
for a spec mentor, and ask them to review this early explainer before
proceeding. If you are not a Googler, you are welcome to make use of this but
not required to.

You can then put a link to your public explainer in the feature entry. You will
then need to publish this explainer, and kick off "standards incubation" by
proposing this to a relevant standards venue (frequently, this means you should
[make a WICG proposal](https://github.com/WICG/admin#contributing-new-proposals)
and start socializing the problem with other vendors and developers. Enter a
reference to the public proposal in “Initial Public Proposal” field; if there is
interest in the WICG proposal and it can be moved into the WICG at this point,
do so. The WICG co-chairs can help you.) It’s also a good idea to discuss your
idea with the team/team lead (TL) or area expert in the feature area, prior to
checking in code in the area.

Start sketching out a proposed solution in your (public) explainer, detailing
API design (IDL) in your incubation. If you are a Googler, you may wish to
review this with your specification mentor before proceeding.

#### Step 2: Prototyping

Proceed to the “Start Prototyping” stage in ChromeStatus - this will generate an
“Intent to Prototype” mail for you. Send that email to
[blink-dev](mailto:blink-dev@chromium.org) and start checking in prototype code
to Chromium under a runtime flag. You should do your detailed API design in the
open, in your public repository, and response to feedback filed there. You
should continue pushing for public engagement (from other vendors and web
developers), and to move into WICG, or other incubation venues, if you haven’t
already. During this stage, you should expand on the explainer with a full
design doc (this may also have implementation-specific details), and consider
creating a specification (or writing up a pull request with changes to an
existing spec).

Note that any CLs landing at this stage should be [behind a feature
flag](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/platform/RuntimeEnabledFeatures.md).
Consider adding a[ UseCounter](/blink/platform-predictability/compat-tools) to
track usage of your new feature in the wild, and be sure to write integration
tests for your feature as[
web-platform-tests](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_platform_tests.md)
as you go. Continue to work with your API mentor if there are any design
changes.

Ensure you have an API overview and descriptions for all IDL methods and
properties (these are probably in your specification or explainer, but
developers will need them to try your feature out), and at least a basic sample.

As soon as you have a functional and reasonably complete implementation of your
initial design ready for developers to try out behind a flag, proceed to the
next step.

#### Step 3: Feature Complete behind a feature flag: iteration on design

Once you have a functional and reasonably complete feature implementation
available as a runtime enabled feature, request a
[TAG](https://github.com/w3ctag/design-reviews/issues) review of your design and
proceed to the “Dev Trials” stage in ChromeStatus. This will generate a “Ready
for Trial” email that you should send to
[blink-dev](mailto:blink-dev@chromium.org) to notify the community they can try
out the feature. At this point, you should consider ask other browser vendors
and the web developer community for [signals on their opinion of the
API](https://docs.google.com/document/d/1xkHRXnFS8GDqZi7E0SSbR3a7CZsGScdxPUWBsNgo-oo/edit#heading=h.tgzhprxcmw4u).

This is the main iterating stage of feature development and helps you assess
product-market-fit early on before you corner yourself (does your API address a
problem with meaningful demand? did we get the ergonomics right?). You may wish
to send more than one Ready for Trial emails, if you make substantial changes to
the API shape while iterating. You should work with the TAG to complete their
review and address any issues raised during this stage, and should address any
issues raised by other horizontal reviews (accessibility, privacy,
internationalization, etc.).

#### Step 4: Evaluate readiness to ship

Once you believe you have addressed all major open issues, you should proceed to
the “Evaluating readiness to ship” stage in ChromeStatus. If you haven't already
received [signals on their opinion of the
API](https://docs.google.com/document/d/1xkHRXnFS8GDqZi7E0SSbR3a7CZsGScdxPUWBsNgo-oo/edit#heading=h.tgzhprxcmw4u)
from other browser vendors and the web developer community, now is the time to
pursue getting those signals.

You should also work with the documentation team to ensure your features will be
documented when shipped, and estimate when (in what milestone) you would like to
target shipping.

You should also decide if an Origin Trial would help gather significant data for
your feature.

By this stage, you need to have a complete specification available that matches
what you have implemented. The only exception is if you plan to do an Origin
Trial, and expect that trial feedback could change the shape of the API
significantly or result in it being scrapped altogether. In that case it might
be OK to delay writing a specification until the Origin Trial is underway or
completes. But be careful: writing a good spec can be a lot of work, and
producing a good enough spec to meet the shipping bar can take longer than the
~12-16 weeks of an Origin Trial, so starting the spec-writing process too late
might delay your feature launch.

#### Step 5 (Optional): Origin Trial

If you want to gather data on the usability of your feature that an [Origin
Trial](/blink/origin-trials/running-an-origin-trial) can help collect, proceed
to the “Origin Trial” stage in ChromeStatus and fill out the required fields
detailing what you hope to learn from the origin trial. This will generate an
[Intent to
Experiment](https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit)
mail that you should send to [blink-dev](mailto:blink-dev@chromium.org). After
receiving at least [one LGTM](/blink/guidelines/api-owners/procedures) from the
API owners, you can proceed with your origin trial release. Collect data and
respond to any issues. From here, you may wish to return to Dev Trials, proceed
to Prepare to Ship, or park the feature.

As noted above, a specification is recommended but not required for an Origin
Trial.

Please note that Origin Trials are not exempt from requiring cross-functional
approvals from the Chrome launch review process.

Depending on your feature and your experimentation goals, running an experiment
via Finch on a percentage of the stable or beta populations may be useful
([example](https://groups.google.com/a/chromium.org/g/blink-api-owners-discuss/c/WoQvWPCxKdU/m/e93bxrzwAQAJ)).
In these cases, an Intent to Experiment explaining why this non-typical path is
requested and the corresponding LGTM(s) are still required before proceeding.

#### Step 6: Prepare to Ship

At this point, if you are a Googler you should get a final spec review from your
standards mentor, and discuss options for moving your spec to a final
standardization venue. You should have TAG sign-off on your API design by now,
or have ongoing discussions on the TAG review without any outstanding major
concerns. You should update ChromeStatus with a target milestone for shipping
(and remember to keep this updated, if things change). You should get final
signoff from Documentation, and update for any changes in vendor signals.

Proceed to the “Prepare to Ship” stage in ChromeStatus; this will generate an
[Intent to
Ship](https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit#bookmark=id.w8j30a6lypz0)
mail that you should send to [blink-dev](mailto:blink-dev@chromium.org). This
will spark a conversation with the API owners; address any feedback from them,
and once you get [3 LGTMs from the API
owners](/blink/guidelines/api-owners/procedures), you may enable the feature by
default. You can learn more about the policies and guidelines the API owners
evaluate [here](/blink/guidelines). Requirements for new API owners are
[here](/blink/blink-api-owners-requirements). Email
blink-api-owners-discuss@chromium.org or reach out to one of the[ API
owners](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/API_OWNERS)
if there is no open/unaddressed feedback and you are still blocked on LGTMs
after 5 days.

Once you have shipped your feature, proceed to the "Ship" stage in ChromeStatus.

The approval status of various stages of the intent process is tracked by the
API owners in [this spreadsheet](https://bit.ly/blinkintents).

### Implementations of already-defined consensus-based standards

#### Step 1: Write up use cases and scenarios, start coding

Fill out the “Motivation” section with a brief summary, and then write up the
use cases and scenarios. If this is a large feature, or a combination of
multiple attributes/properties/methods/events, you may wish to do this in a
separate explainer file.

Proceed to the “Start Prototyping” stage in ChromeStatus - this will generate an
“Intent to Prototype” mail for you. Send that email to
[blink-dev](mailto:blink-dev@chromium.org) and start checking in prototype code
to Chromium as [runtime enabled features](/blink/runtime-enabled-features).
Ensure there are adequate
[web-platform-tests](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_platform_tests.md)
for this feature. Also ensure you have an API overview and descriptions for all
IDL methods and properties (this is probably already in the consensus standard
specification, or even on MDN), and at least a basic sample.

As soon as you have a functional and reasonably complete implementation of the
feature ready for developers to try out under a flag, proceed to the next step.

#### Step 2: Feature Complete behind a flag and implementation refinement

If the TAG has not already reviewed the consensus specification, request a
[TAG](https://github.com/w3ctag/design-reviews/issues) review and proceed to the
“Dev Trials” stage in ChromeStatus. This will generate a “Ready for Trial” email
that you should send to [blink-dev](mailto:blink-dev@chromium.org) to notify the
community they can try out the feature.

After you have addressed any issues that the community finds, you should proceed
to the “Evaluating readiness to ship” stage in ChromeStatus. You should also
work with the documentation team to ensure your feature will be documented when
shipped, and estimate when (in what milestone) you would like to target
shipping. You should also decide if an Origin Trial would help gather
significant data for your feature.

#### Step 3 (Optional): Origin Trial

If you want to gather data on the usability of your feature that an [Origin
Trial](/blink/origin-trials/running-an-origin-trial) can help collect, proceed
to the “Origin Trial” stage in ChromeStatus and fill out the required fields
detailing what you hope to learn from the origin trial. This will generate an
[Intent to
Experiment](https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit)
mail that you should send to [blink-dev](mailto:blink-dev@chromium.org). After
receiving at least [one LGTM](/blink/guidelines/api-owners/procedures) from the
API owners, you can proceed with your origin trial release. Collect data and
respond to any issues. From here, you may wish to return to Dev Trials, proceed
to Prepare to Ship, or park the feature.

Please note that origin trials are not exempt from requiring cross-functional
approvals from the Chrome launch review process. Details[
here](/blink/origin-trials/running-an-origin-trial).

If an Origin Trial happens, then there is a [required breaking
period](https://docs.google.com/document/d/1oSlxRwsc8vTUGDGAPU6CaJ8dXRdvCdxvZJGxDp9IC3M/edit#heading=h.r5cdr0aazfpm)
before shipping in step 4. If you wish to skip the breaking period, meaning that
sites participating in the Origin Trial will not see an interruption in support
for the feature between Origin Trial and launch (\*), you may request an
exception. The process to do so is to include this request in your Intent to
Ship email. In the request, you must show clear evidence that developers engaged
with the Origin Trial and that their concerns were taken into account in the
final API design and implementation. LGTMs for the Intent to Ship imply approval
of the request.

(\*) "Not see an interruption" means that if the origin trial ends at milestone
N, and the feature is shipped in milestone N+1, sites opting into the origin
trial will continue to be able to use the feature on Chromium milestone N up to
(and even after, for those users who have not upgraded) N+1 ships.

#### Step 4: Prepare to Ship

You should update ChromeStatus with a target milestone for shipping (and
remember to keep this updated, if things change). You should get final signoff
from Documentation, and update for any changes in vendor signals.

Proceed to the “Prepare to Ship” stage in ChromeStatus; this will generate an
[Intent to
Ship](https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit#bookmark=id.w8j30a6lypz0)
mail that you should send to [blink-dev](mailto:blink-dev@chromium.org). This
will spark a conversation with the API owners; address any feedback from them,
and once you [get 3 LGTMs](/blink/guidelines/api-owners/procedures) from the API
owners, you may enable the feature by default. See[
here](/blink/guidelines/web-platform-changes-guidelines) for the principles the
API owners will apply in evaluating whether the feature is mature enough to
ship. (API owners requirements are listed[
here](/blink/blink-api-owners-requirements) - email
blink-api-owners-discuss@chromium.org or reach out to one of the[ API
owners](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/API_OWNERS)
if no open/unaddressed feedback and you are still blocked on LGTMs after 5
days.)

Once you have shipped your feature, proceed to the "Ship" stage in ChromeStatus.

### Feature deprecations

#### Lessons from the first years of deprecations and removals ([thread](https://groups.google.com/a/chromium.org/forum/#!topic/blink-dev/1wWhVoKWztY))

We should weigh the benefits of removing an API more against the cost it
has. Percent of page views by itself is not the only metric we care about.

The cost of removing an API is not accurately reflected by the UseCounter
for older, widely implemented APIs. It's more likely that there's a
longer-tail of legacy content that we're breaking.

We shouldn't remove APIs that have small value on the path towards a removal
that has significant value. Getting rid of attribute nodes \*is\* valuable
and would benefit the platform. Getting rid of half the attribute node
methods is not. So we should evaluate the usage of all the APIs we need to
remove together in order to get there. Also, if we remove them, we should
remove them all in the same release. Breaking people once is better than
breaking them repeatedly in small ways.

We should be more hesitant to remove older, widely implemented APIs.

For cases where we're particularly concerned about the compatibility hit, we
should do the removal behind a flag so that we can easily re-enable the API
on stable as we don't know the compat hit until the release has been on
stable for a couple weeks.

Enterprise users have additional difficulties reacting to breaking changes,
but we also have additional tools to help them. See[ shipping changes that
are enterprise-friendly](/developers/enterprise-changes) for best practices.

High-usage APIs may require much more work to land successfully. See
[here](https://docs.google.com/document/d/1-_5MagztiYclsMccY4Z66XI465WaasT5I5y2dnhRvoE/edit#heading=h.n49iymp16hl6)
for a good example of how this worked in practice with the deprecation and
removal of the Web Components v0 APIs.

#### Step 1: Write up motivation

Deprecations and removals must have strong reasons, explicitly balanced against
the cost of removal. Deprecations should be clear and actionable for developers.
First, read the [guidelines for deprecating a
feature](https://docs.google.com/a/chromium.org/document/d/1LdqUfUILyzM5WEcOgeAWGupQILKrZHidEXrUxevyi_Y/edit?usp=sharing).
Measure feature usage in the wild. [ Various
tools](/blink/platform-predictability/compat-tools) are available, but typically
this is accomplished by simply adding your feature to the[ UseCounter::Feature
enum](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/core/frame/UseCounter.h&sq=package:chromium&type=cs&q=file:UseCounter.h%20Feature)
and adding MeasureAs=&lt;your enum value&gt; to the feature's IDL definition.

Fill out the “Motivation” section with a brief summary, and then write up the
use cases and scenarios. Proceed to the “Write Up Motivation” stage in
ChromeStatus - this will generate an “Intent to Deprecate and Remove” mail for
you. Send that email to [blink-dev](mailto:blink-dev@chromium.org) and start
checking in your code removing the feature to Chromium as a [runtime enabled
feature](/blink/runtime-enabled-features). Make sure to provide suggested
alternatives to the feature being deprecated. As soon as you have a functional
removal of the feature ready for developers to try out under a flag, proceed to
the next step.

#### Step 2: Dev trial of deprecation

Proceed to the “Dev Trials” stage in ChromeStatus. This will generate a “Ready
for Trial” email that you should send to
[blink-dev](mailto:blink-dev@chromium.org) to notify the community they can try
out the feature deprecation.

At this point, you should also notify developers by adding a deprecation console
message, pointing to the updated status entry in the console message. You should
also continue to measure usage by adding the API to the big switch in[
UseCounter::deprecationMessage](http://src.chromium.org/viewvc/blink/trunk/Source/core/frame/UseCounter.cpp#l120).
Give developers as many milestones as possible to respond to the deprecation.

Instrument your code by either adding DeprecateAs=\[your enum value here\] to
the feature's IDL definition.\* -- See window.performance.webkitGetEntries., or
adding a call to UseCounter::countDeprecation somewhere relevant.

You should work with the documentation team to ensure the community is prepared
for this feature deprecation, and estimate when (in what milestone) you would
like to target shipping. You should also decide (possibly based on data from the
dev trial) if a deprecation trial is going to be necessary to help smooth the
removal of this feature from the web platform.

#### Step 3 (Optional): Deprecation Trial

If you are concerned that there are going to be web developers who need
additional time to fix up their implementations, and will want to delay your
feature deprecation, you can file for a [Deprecation
Trial](https://groups.google.com/a/chromium.org/forum/#!msg/blink-api-owners-discuss/uNWSTCUzIcU/0jyBWgLlDgAJ).
This will let you disable the feature by default, but let developers request an
origin trial token to re-enable the feature, for a limited period of time after
the feature deprecation. You will need to decide how long to keep the
deprecation trial open and enter that milestone in the tool for planning
purposes, and then select “Draft Request for Deprecation Trial email” in
ChromeStatus, and send the resulting “Request for Deprecation Trial” email to
[blink-dev](mailto:blink-dev@chromium.org). After receiving at least [one
LGTM](/blink/guidelines/api-owners/procedures) from the API owners, email
[experimentation-dev@chromium.org](https://groups.google.com/a/chromium.org/forum/#!forum/experimentation-dev)
letting them know you plan to run a deprecation trial, ensure your removal is
integrated with the origin trials framework ([see
details](/blink/origin-trials/running-an-origin-trial#integrate-feature)), and
Googlers can request a trial for their feature at
[go/new-origin-trial](http://goto.google.com/new-origin-trial). Once your
Deprecation Trial is in place, proceed to the next step.

#### Step 4: Prepare to Ship

You should update ChromeStatus with a target milestone for deprecating the
feature (and remember to keep this updated, if things change). You should get
final signoff from the Documentation team.

Proceed to the “Prepare to Ship” stage in ChromeStatus; this will generate an
[Intent to
Ship](https://docs.google.com/document/d/1vlTlsQKThwaX0-lj_iZbVTzyqY7LioqERU8DK3u3XjI/edit#bookmark=id.w8j30a6lypz0)
mail that you should send to [blink-dev](mailto:blink-dev@chromium.org). This
will spark a conversation with the API owners; address any feedback from them,
and once you get [3 LGT](/blink/guidelines/api-owners/procedures)Ms from the API
owners, you may proceed.

#### Step 5: Disable the feature

Disable the feature by default. Update ChromeStatus to either “Disabled” or
“Disabled with Deprecation Trial”.

#### Step 6: Remove Code

If you are running a Deprecation Trial, wait until the Deprecation Trial period
has ended. (If you need to extend the Deprecation Trial, notify
[experimentation-dev@chromium.org](mailto:experimentation-dev@chromium.org) and
click “Generate an Intent to Extend Deprecation Trial” in ChromeStatus and send
the resulting notification to blink-dev.)

Once the feature is no longer available, remove the code (typically waiting a
couple of milestone cycles to insure the code is no longer needed), and set the
ChromeStatus to “Removed.”

If you are unsure of when a feature could be removed, or would like to
discourage usage, you may deprecate a feature without a removal deadline. This
is strongly discouraged and will require significant justification:

* Email blink-dev using the ["Intent to Deprecate" template](https://docs.google.com/a/chromium.org/document/d/1Z7bbuD5ZMzvvLcUs9kAgYzd65ld80d5-p4Cl2a04bV0/edit).

* [1 LGTM](/blink/guidelines/api-owners/procedures) necessary from the API owners

* Must justify why there is no removal date

\* It takes 12-18 weeks to hit Stable once you enable instrumentation. If there
is time pressure you may be able to get permission to merge UseCounter changes
into an existing dev/beta branch, and make provisional decisions based on data
from beta channel.

### Web-developer-facing change to existing code (PSA)

#### Step 1: Write up motivation and implement code

Fill out the “Motivation” section with a brief summary, and proceed to the
“Implementing” stage in ChromeStatus.

#### Step 2: (Optional) Dev trial

If you want to try out this change before shipping it, put your code in Chromium
as [runtime enabled features](/blink/runtime-enabled-features), and set the
status to “Dev Trial” in ChromeStatus. This will generate a “Ready for Trial”
email that you should send to [blink-dev](mailto:blink-dev@chromium.org) to
notify the community they can try out code change.

#### Step 3: Prepare to Ship

You should update ChromeStatus with a target milestone for shipping (and
remember to keep this updated, if things change). Proceed to the “Prepare to
Ship” stage in ChromeStatus; this will generate a “Web-Facing Change PSA” mail
for you. Send that email to [blink-dev](mailto:blink-dev@chromium.org) with the
summary of the code change and the expected milestone.

#### Step 4: (Optional) Finch trial

You may wish to use [Finch](http://go/finch) to increase confidence in the new
code as you deploy it.

#### Step 5: Ship

Ship it, and set the status to “Shipped”.

## Post Launch

After launching a new feature, watch for crashes, regressions caused by your
feature and any substantive spec feedback. Review [UseCounter](/blink/platform-predictability/compat-tools) and other metrics.
Update the intent-to-ship thread and ChromeStatus if non-trivial issues
(like web compatibility or serious design questions) arise. When in doubt,
email blink-dev@ (or blink-api-owners-discuss@ if you prefer a smaller
audience) for advice.

Once a new API is on by default, continue to support other implementations
(for instance, clarifying the spec, improving the tests, fixing bugs, and
updating support status in ChromeStatus) until the feature is broadly
supported and works the same across engines. Remember, your job is to move
the web forward, not simply add features to Chrome.

Review MDN docs for technical accuracy and clarity. Feel free to make edits
directly or reach out to jmedley@.

When you are convinced enabling the feature by default has “stuck”
(typically 2 milestones), remove any unused code including
RuntimeEnabledFeatures entries.

## Related

For an overview, and explanation of motivations, please see:

*   Video presentation: [Intent to explain: Demystifying the Blink
            shipping process (Chrome Dev Summit
            2019)](https://www.youtube.com/watch?v=y3EZx_b-7tk&list=PLNYkxOF6rcIDA1uGhqy45bqlul0VcvKMr&index=17)
*   Blog post: [Intent to Explain: Demystifying the Blink Shipping
            Process](https://blog.chromium.org/2019/11/intent-to-explain-demystifying-blink.html)
