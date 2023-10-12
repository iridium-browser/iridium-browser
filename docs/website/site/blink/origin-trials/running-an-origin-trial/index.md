---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
- - /blink/origin-trials
  - Origin Trials
page_name: running-an-origin-trial
title: Running an Origin Trial
---

*For the full context on origin trials, please see the
[explainer](https://github.com/GoogleChrome/OriginTrials/blob/gh-pages/explainer.md).
This is the feature author guide for Blink contributors.*

Here, we describe what is involved in running an origin trials experiment for a
new browser feature. Most importantly, origin trials are integrated into the
[launch process for new web platform features](/blink/launching-features). You
should be following that overall process (maybe you ended up here from that
page).

[TOC]

## Should you run an origin trial?

Origin trials are intended to be used to ensure we design the best possible
features by getting feedback from developers before the standard is finalized.
They may also be used to prove developer interest in a feature proposal that is
otherwise undesired due to an expected lack of interest. Typically, you should
have a specific hypothesis that you wish to test by collecting data.

If you answer “yes” to any of the following questions, you should consider
running an origin trial (see caveat in \[1\]).

*   Is there disagreement about how well this API satisfies its intended
            use case?
*   Are you unsure about what API shape will be the most ergonomic in
            real world scenarios?
*   Is it hard to quantify performance gains without testing on real
            world sites?
*   Is there a reason that this API needs to be deployed to real users,
            rather than behind a flag, for data to be meaningful?

\[1\] Origin trials should be run for a specific reason. These questions are
guidance to identifying that reason. However, there is still debate about the
right reasons, so the guidance may change. You can join the conversation in
[this
doc](https://docs.google.com/document/d/1oSlxRwsc8vTUGDGAPU6CaJ8dXRdvCdxvZJGxDp9IC3M/edit#heading=h.eeiog2sf7oht).

*If you're planning to run an origin trial please contact the origin trials team
(OT team) to quickly talk over your feature and the reason for running the
trial.* You can email
[origin-trials-support@google.com](mailto:origin-trials-support@google.com).

## How do origin trials work in Chrome?

The framework will enable features at runtime, on a per-execution-context basis
(practically, this will be per-document or per-worker). Features are disabled by
default, and only be enabled if a properly signed token, scoped to the origin
that it is being presented on, and scoped to the specific feature name, is
present in either:

*   an HTTP Origin-Trial header in the server response,
*   an HTML &lt;META&gt; tag in the document's head, or
*   (for Dedicated Workers only) the HTTP response or document head of
            the parent document.

The logic for enabling includes a check of your [runtime feature
flag](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/platform/RuntimeEnabledFeatures.md)
(even if the origin trials framework isn't being used). This means you can
easily test your feature locally, even without any trial tokens.
Origin trials are being enabled in documents (for both inline and external
scripts), and in service, shared, and dedicated workers. (Note that for service
workers and shared workers, HTTP headers are the only way to enable trials.
Dedicated workers will also inherit any trials enabled by their parent
document).
If an experiment gets out of hand (*way* too popular to be an experiment
anymore, for instance), we’ll be able to turn it off remotely, for all origins.
Similarly, if there turns out to be major problems with the implementation of
the framework itself, we’ll be able to turn it off completely, and disable all
trials. (Hopefully we don’t have to do that, but we're still in the early stages
of origin trials, and we’re being careful.)

## Is your feature ready to be an origin trial?

Before running an origin trial experiment, your feature needs to be ready for
both web developers and users. Your feature must satisfy the following:

*   Have an explainer for your feature
    *   There needs to be a description of the problems and use cases
                the feature is intended to address.
*   Have a draft spec for your feature
    *   Ideally this would be in the form of an actual spec -- or PR
                against an existing spec -- in the format expected by the
                eventual standards body (although the draft might be hosted
                elsewhere while discussions are ongoing), but a detailed
                explainer laying out all the details may suffice. The level of
                detail needs to be enough so that (a) developers participating
                in the origin trial know how it works, and (b) spec editors for
                the relevant eventual specifications can see exactly how it
                would affect that spec when added.
*   Be approved by the internal Chrome launch review process
    *   Users may be exposed to your feature without opting in, so the
                appropriate measures must be taken for privacy, security, etc.
*   Have a way to remotely disable the feature
    *   Origin trials provides infrastructure to disable a feature (or a
                specific origin), but this only applies to the exposure as an
                origin trial. That means, any interface(s) controlled by the
                trial will be disabled, but it will still be possible to enable
                the feature via its runtime flag. As well, all of the token
                validation/revocation happens in the renderer.
    *   If the previous point is not sufficient for disabling the
                feature, you should implement a kill switch that allows your
                feature to be disabled remotely via Finch.
        *   This can use the existing functionality in
                    PermissionContextBase or base::FeatureList, or be a
                    feature-specific implementation.
    *   Consult the Chrome feature launch process for [guidance for a
                feature
                flag](https://docs.google.com/document/d/1hJ1U8-7DNa7lGfTJWRgSgqQyNnOFO4Ks5Czr1-3--8I/edit#bookmark=id.xgrabupypw8w)
                (Googlers only). If you would launch your feature by default
                with a flag, then you should implement one for the origin trial.
*   Have UMA metrics to track feature usage
    *   You should record usage with
                [UseCounter](https://cs.chromium.org/chromium/src/third_party/blink/renderer/core/frame/use_counter.h),
                as that can be automatically monitored by the origin trials
                infrastructure.
    *   The feature must have a corresponding entry in the enum
                [WebFeature](https://cs.chromium.org/chromium/src/third_party/blink/public/mojom/use_counter/metrics/web_feature.mojom).
    *   For any JavaScript-exposed API, usage can be recorded easily via
                one of the
                [\[Measure\]](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md#Measure_i_m_a_c)
                or
                [\[MeasureAs\]](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/bindings/IDLExtendedAttributes.md#measureas_i_m_a_c)
                IDL attributes.
    *   If not exposed via IDL, the appropriate UseCounter::count\*()
                method can be used directly from your feature implementation.
    *   If it's not feasible to integrate with UseCounter (e.g. usage is
                best tracked outside a renderer, .etc), please contact the OT
                team.
*   Have an established community for discussion of the feature
    *   At a minimum, this should be a WICG group, Github repo, etc.
                Anywhere developers can find discussion or log issues about your
                feature.
    *   The origin trials system will facilitate collecting feedback
                from web developers. However, the goal is to have web developers
                participate in the existing community around the feature.
*   Prepared a blog post/article/landing page introducing the feature
    *   There needs to be single link that will provide details about
                the feature.
    *   Should make it clear how developers provide feedback/log issues
                for your feature.
    *   This could be the README.md in your Github repo, or any other
                page of your choice.
    *   Should include details about availability via origin trials.

## What is the actual process to run an origin trial?

First review the [Blink launch process](/blink/launching-features). Please
contact [origin-trials-support@google.com](mailto:origin-trials-support@google.com)
with any questions about these steps. If you don't have access to any of the
links below, the mailing list can help find someone to guide you.
Running an origin trial requires the following:

*   Make sure your feature is ready to run an origin trial experiment
            ([see
            above](/blink/origin-trials/running-an-origin-trial#is-feature-ready)).
*   Review
            [go/ChromeLaunchProcess](https://goto.google.com/ChromeLaunchProcess)
            and determine what launch approvals you require.
*   Integrate with the origin trials framework ([see
            below](/blink/origin-trials/running-an-origin-trial#integrate-feature)).
*   Send an [Intent to Experiment](/blink/launching-features), via the
            [ChromeStatus entry for your feature](https://chromestatus.com/).
*   Request a trial for your feature at
            [go/new-origin-trial](http://goto.google.com/new-origin-trial).
    *   This will ensure your trial is tracked correctly in
                [go/origin-trials-feature-pipeline](http://goto.google.com/origin-trials-feature-pipeline/).
*   Land the feature in Chrome prior to beta.
*   Engage with external partners or large-scale developers for early
            testing.
    *   Some issues may only found during large-scale use, so should be
                tested even before beta (if possible).
    *   If feasible, this could include doing your own testing within
                the developer's environment (any of test/staging/production).
    *   For an example, see [crbug.com/709211](http://crbug.com/709211).
*   Publish a blog post on
            [developers.google.com/web/updates](http://developers.google.com/web/updates)
            about the feature when it lands beta.
    *   The OT team will add your feature to the [public developer
                console](https://developers.chrome.com/origintrials).
    *   [See
                below](/blink/origin-trials/running-an-origin-trial#add-to-signup-form)
                for more details.
*   Update the feature's entry on
            [chromestatus.com](https://www.chromestatus.com/) to set the status
            to "Origin trial".
*   You can review the developer registrations for your feature (and
            renewals) by following the instructions in the [feature author
            guide](https://goto.google.com/running-an-origin-trial).
    *   In particular, be aware of any registrations that expect
                &gt;10,000 page views per day to be using the feature

Note that these steps are not meant to be sequential. For example, you can
certainly start integrating your feature with origin trials prior to getting
various launch approvals.

### Adding your feature to the public developer console

The configuration of a trial in the [developer
console](https://developers.chrome.com/origintrials) requires basic information,
including some public landing page/documentation link ([see
above](/blink/origin-trials/running-an-origin-trial#is-feature-ready)). In some
cases, this may seem like a chicken-and-egg problem. For example, you may not
want to publish a blog post until the feature is ready for developers. If the
blog post has detailed information on joining the origin trial, it doesn't make
sense to publish and have web developers unable to register. Fortunately, a
trial can be configured in the developer console in advance, separately from
making it available to the public.

Recommended process:

*   Request setup of the trial with an interim documentation link (e.g.
            the README from the GitHub repo).
    *   The OT team can configure the trial in the developer console
                (without making in public), and provide a permanent link to the
                trial detail page.
*   Publish the blog post for the feature when the beta release is
            available.
    *   Include instructions about the origin trial, and a link to
                either (1) the list of available trials or (2) the detail page
                for your trial.
*   Notify the OT team with the final link to the blog post.
    *   The link will be updated in the developer console and recorded
                in
                [go/origin-trials-feature-pipeline](http://goto.google.com/origin-trials-feature-pipeline/).
*   Notify the OT team when the trial is ready for registration.
    *   OT team will activate the trial to make it available to the
                public.

## How long do Origin Trials typically last?

Origin trials can run for up to 6 milestones (~24 weeks), and can be extended
under certain conditions.
See [here](https://www.chromium.org/blink/launching-features/#step-3-optional-origin-trial)
for more details.

## What is the process to extend an origin trial?

Origin trials are not intended as a mechanism for shipping a feature early
without following the full launch process. This is one of the reasons that each
trial has a predefined end date (rather than running indefinitely until the
feature ships). That said, there are situations where it is beneficial to allow
experimentation to continue beyond the planned end date of the trial.

There two general cases where experimentation may continue beyond the planned
end date:

1.  Unexpected delays in releasing
    *   With the 4 week Chrome release cycle, code may not land in the
                intended release, meaning it is not shipped until the subsequent
                release. Alternatively, the code may land in the release, but
                the Chrome stable rollout is delayed, meaning it not
                available/installed until much later than expected. When a trial
                typically runs for 12-16 weeks, such delays can
                significantly impact the availability of the experimental
                feature and the ability to collect sufficient data.
2.  Feature changes or new areas experimentation
    *   As the origin trial progress, you may determine that a feature
                is not ready to be shipped, but do want to continuing
                experimenting. For example, feedback indicates that changes are
                needed to the feature (especially in API surface), but the
                changes would benefit from further feedback. In other cases, the
                hypothesis for the experiment may be proved or disproved, but
                you uncover new hypotheses for experimentation.

Consult with the OT team to figure out if you're in a situation where it makes
sense to continue experimenting. For unexpected delays (1), this generally means
requesting an extension to the trial end date. For feature changes and such (2),
this generally means starting a new origin trial, to follow the previous trial.

In order to be eligible for an extension, you must demonstrate substantial
progress towards meeting the bar for shipping the feature.
See [here](https://www.chromium.org/blink/launching-features/#step-3-optional-origin-trial)
for more details.


### How to setup an extension or continued experiment?

The process is as follows:

*   If desired, email
            [origin-trials-support@google.com](mailto:origin-trials-support@google.com)
            to consult on the appropriate approach.
*   Send an Intent to Extend Origin Trial, via the [ChromeStatus entry
            for your feature](https://chromestatus.com).
*   Wait for 1 LGTM from at least one API owner (similar to the original
            Intent to Experiment)
*   If continuing to experiment via a new trial:
    *   This will officially be a new and separate trial, meaning a
                separate entry in the list of trials, etc.
    *   Request another trial at
                [go/new-origin-trial](http://goto.google.com/new-origin-trial),
                with the appropriate naming to distinguish the old and new
                trial.
    *   Update the integration with the framework - the code must use a
                different trial name in Chromium ([see integration
                below](/blink/origin-trials/running-an-origin-trial#integrate-feature)).
*   If extending the end date of the existing trial:
    *   Notify the OT team of the change at
                [go/extend-origin-trial](http://goto.google.com/extend-origin-trial).
*   Upon approval, the OT team will setup the extension or new trial as
            appropriate.

## How to integrate your feature with the framework?

The integration instructions now live in the Chromium source repo:
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/origin_trials_integration.md>

## Roadmap

All of this may change, as we respond to your feedback about the framework
itself. Please let us know how it works, and what's missing!
To follow the most up-to-date progress and plans, filter in crbug.com for
“[component:Internals&gt;OriginTrials](https://bugs.chromium.org/p/chromium/issues/list?q=component:Internals%3EOriginTrials)”.
