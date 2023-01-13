---
breadcrumbs:
- - /audio-video
  - Audio/Video
- - /audio-video/autoplay
  - Autoplay
page_name: autoplay-policy-design-rationale
title: Autoplay Policy Design Rationale
---

This page captures the first principles and design rationale for the autoplay
policy in Chrome.

#### Problem

Browsers have historically been poor at helping the user manage sound, and
unwanted noise is the primary reason that users don’t want autoplay. However,
not all autoplay is unwanted, and a significant portion of blocked autoplays in
Chrome are subsequently played by the user.

#### Vision

A core mission of Chrome is simplicity and ideally Chrome would accurately allow
audio playback when a user wants it, and prevent surprises when they don’t. A
long-term goal is for fewer users to block audio at the OS level, meaning that
more users will hear sound overall. It’s unlikely that Chrome can be 100%
accurate at meeting user expectations, but a reasonable vision is:

    For 99% of users, Chrome will be 95% accurate at predicting when a user
    wants an audible playback before a user gesture on a page.

    In cases where Chrome is inaccurate (for example, while learning the user’s
    preferences), 99% of the time the user will control sound by interacting
    with the page.

    When the page does not allow the user to control sound, or when a user more
    generally has different requirements than what Chrome can provide, Chrome
    will provide the user control over their browsing experience. Direct
    interaction with Chrome to manage sound will be infrequent.

Realization of this vision would result in the following user experience:

    In 95% of browsing sessions, audio will match user expectations.

    In 4.95% of browsing sessions, users will interact with the page to control
    audio.

    In 0.05% of browsing sessions, users will either terminate the session or
    will use Chrome to control audio.

#### Core Principles

The following core principles were used to design Chrome’s autoplay policy.

Note that these principles do not apply when the user has made an explicit
choice in Chrome to control sound for a site. For example, if a user has muted a
site, these principles do not apply.

1. User Gesture Should Allow Sound

If users ask for sound, they should get it. A core user expectation is that
hitting ‘play’ (e.g. on a media element or a game) will enable audio. But any
HTML element could be a ‘play’ button and Chrome can’t tell the difference.
Thus, Chrome must assume that any user gesture should allow sound playback on
the page.

2. Sites Can’t Control The Policy

There are incentives for sites to play video and audio regardless of user
expectations, and sites will leverage any API they can to play audio on page
load. The site cannot be a steward of the user’s audio experience before a user
gesture, and cannot override Chrome’s policy choices. For this reason, the site
cannot override Chrome’s choices or control when autoplay is allowed.

3. The Site Must Know when Audio or Autoplay is Blocked

When the policy blocks autoplay for audible media, site developers may want to
show a different user experience (e.g. click-to-play), apply different logic
(e.g. choosing muted ads), and at the very least will want to update any visual
indication of audio playback state (e.g. a volume status indicator). For this
reason, if autoplay with audio is blocked the site must know that it is blocked.

4. The Site Must Be Allowed To Request the User Gesture

A user gesture is on the page, and the site is thus best positioned to present a
UX that explains the impact of a user gesture to enable sound, give a place to
click or tap, and apply logic to that user gesture. Alternatives where Chrome
enables sound arbitrarily will be more confusing for the user. For this reason,
the site (not Chrome) is in the best position to help the user play media with
sound.

5. The Site Needs Predictable Behavior

When sound is allowed, developers need predictable control over audio playback
state, to ensure that the right elements are playing, the UX is in sync, and the
timing of audio playback is aligned with other elements on the page.
Alternatives where Chrome enables sound or plays media arbitrarily may break
sites or cause a poor user experience. For this reason, the site must be
responsible for starting playback or enabling audio when it is allowed.

6. User-Initiated Audio Playback is a Moderately Strong Signal to Chrome

Sustained, audible playback as a result of a user gesture is a moderately strong
signal that audio was desired. This signal is not perfect (e.g. a site could
hijack a click and play audio on a system that has no speakers). However, Chrome
will use regular, sustained playback as a signal of user intent.

7. Chrome Must Learn And Adapt to User Preferences

A key learning from early MEI analysis was that there is a wide range of user
preferences for audio playback. This is true not only across different sites,
but even within the same site where one user might find audio annoying, but
another will expect it to work automatically. Chrome cannot rely upon a static
list of ‘acceptable’ sites and must learn and adapt to individual user
preferences. As such, for each individual user Chrome will learn on which sites
autoplay is expected, and where it is not. All sites will be subjected to the
same rules; there are no sites where autoplay is always expected by all users.

#### Design Implications

The above principles drive the following design conclusions:

Chrome Can’t Prompt To Enable Audio

Sites will generally present a UX that allows the user to enable audio. If
Chrome adds its own prompt, this will lead to user confusion about where to
click, and additional friction in cases where users end up clicking twice.
(Note: A more subtle UX such as a play/pause button in the Omnibox may still be
acceptable, especially if the logic which detects when to display it is designed
to avoid confusion.)

Chrome Can’t Use Tab Muting For Automatic Control

Automatic management of sound through tab muting wouldn’t allow the site to
detect that they were muted, and wouldn’t allow them to manage user
expectations. Further, it would prevent the site from prompting for a user
gesture and managing user expectations.

The Site (not Chrome) Must Enable Audio

If Chrome enables audio, it may appear random to the developer, meaning that the
site UX gets out of sync, and developers may lose control over which audio
elements are playing. Further, the developer may (in response to a user gesture)
wish to perform tasks before enabling sound. The developer, not Chrome, must be
responsible for enabling sound after a user gesture.

Blocking, Not Muting, for Chrome’s Default Behavior

Blocking audio is a predictable, active signal that audio is not allowed. Mute
state introduces unpredictability for developers, because it requires a query
after audio has started. In cases where audio playback triggers a visual
display, developers may not be able to prevent a brief flash of visuals before
they can make the query and disable the visuals.

Further, (1) the use of mute state is inconsistent with existing policies for
audio management (e.g. in other browsers, on mobile, or in IFrames), (2) not all
audio APIs have provisions for muting, (3) for APIs that do have mute
attributes, making them dynamically controlled by Chrome overloads their
behavior, and (4) it consumes more CPU power to have a running (muted) stream
than a stopped one.

Not like pop-up blocker

Sound management will not eliminate sound, nor will it eliminate autoplay (which
will be allowed when Chrome believes the user wants it). The frequency of
scenarios where a user wants autoplay will not decrease over time. This puts
autoplay in a different category than other types of interventions, where the
behavior being blocked is expected to fall into disuse, reducing the frequency
of Chrome-based prompts.

Further, it is expected that over time most sites will monitor their attempts to
autoplay and manage the user experience when it is not allowed. This will over
time reduce the need for any type of Chrome-based prompt to enable blocked
autoplay.
