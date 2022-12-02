---
breadcrumbs:
- - /audio-video
  - Audio/Video
page_name: autoplay
title: Autoplay
---

***Update (November 22, 2018)***

*For the Web Audio API, the Autoplay Policy will launch in M71.*

***Update (May 15, 2018)***

*The Autoplay Policy launched in M66 Stable for &lt;video&gt; and &lt;audio&gt;
and is effectively blocking roughly half of unwanted media autoplays in Chrome.*

*For the Web Audio API, the autoplay policy will launch in M70. This affects web
games, some WebRTC applications, and other web pages using audio features.
Developers will need to [update their
code](https://developers.google.com/web/updates/2017/09/autoplay-policy-changes#webaudio)
to take advantage of the policy. More detail can be found in the Web Audio API
section below.*

**Summary**
This policy controls when video and audio is allowed to autoplay, and is
designed to meet three primary goals:

*   Provide user control over what content can autoplay
*   Enable legitimate uses of autoplay without complicated workarounds
*   Make progress towards consistent policies across mobile and desktop
            platforms

Under the new policy media content will be allowed to autoplay under the
following conditions:

*   The content is muted, or does not include any audio (video only)
*   The user tapped or clicked somewhere on the site during the browsing
            session
*   On mobile, if the site has been [added to the Home
            Screen](https://developers.google.com/web/updates/2017/02/improved-add-to-home-screen)
            by the user
*   On desktop, if the user has frequently played media on the site,
            according to the [Media Engagement
            Index](https://docs.google.com/document/d/1_278v_plodvgtXSgnEJ0yjZJLg14Ogf-ekAFNymAJoU/edit#heading=h.c1rqulonmckg)

By default embedded IFrames will only be able to play muted or silent videos.
However, if site owners wish for IFrames on their site to be able to play
unmuted content, they may pass the autoplay permissions to the IFrame using
[allow=autoplay](https://github.com/WICG/feature-policy/blob/gh-pages/features.md).
This attribute allows any video contained in the IFrame to play as if it were
hosted on the site.
For a more detailed design and rationale, please click
[here](https://docs.google.com/document/d/1EH7qZatVnTXsBGvQc_53R97Z0xqm6zRblKg3eVmNp30/edit).
**Autoplay blocking**
Around the same time we will be making two additional changes related to
autoplay that will make muted autoplay more reliable. These two changes will
make it possible for sites and advertisers to use muted videos instead of
animated .gifs, which in most cases will reduce overall bandwidth consumption.

*   Removing the block autoplay setting that is currently available on
            Chrome for Android
*   Removing autoplay blocking on mobile when data saver mode is enabled

**Developer Recommendations: &lt;video&gt; and &lt;audio&gt;**

*   **Use autoplay sparingly.** Autoplay can be a powerful engagement
            tool, but it can also annoy users if undesired sound is played or
            they perceive unnecessary resource usage (e.g. data, battery) as the
            result of unwanted video playback.
*   If you do want to use autoplay, consider **starting with muted
            content and let the user unmute** if they are interested in
            exploring more. This technique is being effectively used by numerous
            sites and social networks.
*   Unless there is a specific reason to do so, we recommend **using the
            browser’s native controls for video and audio playback**. This will
            ensure that autoplay policies are properly handled.
*   If you are using custom media controls, **ensure that your website
            functions properly when autoplay is not allowed.** We recommend that
            you always look at the promise returned by the play function to see
            if it was rejected:

> var promise = document.querySelector('video').play();

> if (promise !== undefined) {

> promise.then(_ =&gt; {

> // Autoplay started!

> }).catch(error =&gt; {

> // Autoplay was prevented.

> // Show a "Play" button so that user can start playback.

> });

> }

*   Prompt users to [add your mobile site to the
            homescreen](https://developers.google.com/web/updates/2017/02/improved-add-to-home-screen)
            on Android devices. This will automatically give your application
            unmuted autoplay privileges.

Developer Recommendations: Web Audio API

The Web Audio API will be included in the Autoplay policy with M70 (October
2018). Generally, in Chrome developers can no longer assume that audio is
allowed to play when a user first arrives at a site, and should assume that
playback may be blocked until a user first interacts with the site through a
user activation (a click or a tap). Any attempt to create an audioContext before
that time may result in a suspended audioContext that will have to be explicitly
switched to running after a user activation.

Developers who write games, WebRTC applications, or other websites that use the
Web Audio API should call context.resume() after the first user gesture (e.g. a
click, or tap). For example:

// Resume playback when user interacted with the page.

document.querySelector('button').addEventListener('click', function() {

context.resume().then(() =&gt; {

console.log('Playback resumed successfully');

});

});

Web Audio API developers can detect whether or not autoplay is allowed by
creating a new AudioContext and then checking its state to see whether it is
running (allowed) or suspended (blocked).

Depending upon the site, it may make sense to add additional user interface
elements (such as a ‘play’ button in front of a game, or an ‘unmute’ button in
some other cases), to explicitly capture a user gesture. This can either be done
prior to creating AudioContext, or afterwards with a call to resume() upon
click.

**IFrame embedded content**

Embedded content in a cross-origin IFrame needs to have permission to autoplay
delegated to it, otherwise the audioContext will never be allowed to run.

Developers that host IFrames with content inside them (e.g. game hosting sites)
can enable audio for that content without requiring the underlying content to
change any code, by doing the following:

    If the content is in a cross-origin IFrame, ensure that the IFrame includes
    the attribute allow="autoplay"

    Ensure that before the embedded content loads and runs, the site captures a
    user gesture (e.g. prompt for a click or a tap)

Developers can find more details about specific code changes, and debugging tips
[here](http://developers.google.com/web/updates/2017/09/autoplay-policy-changes).

*Web Audio API FAQs*

    Why is the Web Audio API part of the autoplay policy? Users don’t like to
    click on a link and have sound played automatically that they weren’t
    expecting. The Web Audio API produces sound, so it must be included in the
    autoplay policy to ensure consistency across all web experiences.

    Wait, didn’t you launch the autoplay policy for Web Audio API in M66? Yes,
    briefly, but we reverted the change about a week later. We’re always working
    to improve things for users and developers, but in this case we did not do
    an effective job of communicating the change to developers using the Web
    Audio API. We are moving the launch to October 2018 to give those developers
    more time to prepare. If you develop web games, WebRTC applications, or
    other web experiences with sound please see the developer recommendations.

Release Schedule

<table>
<tr>
<td>September 2017</td>
<td>New autoplay policies announced</td>
<td>Begin collecting Media Engagement Index (MEI) data in M62 Canary and Dev</td>
</tr>
<tr>
<td>December 2017</td>
<td>Site muting available in M64 Beta Autoplay policies available in M65 Canary and Dev</td>
</tr>
<tr>
<td>January 2018</td>
<td>Site muting available in M64 Stable</td>
</tr>
<tr>
<td>April 2018</td>
<td>Autoplay policies are enforced for &lt;video&gt; and &lt;audio&gt; in M66 Stable</td>
</tr>
<tr>
<td>October 2018</td>
<td>Autoplay policies will be enforced for Web Audio API in M70 Stable</td>
</tr>
</table>

**General FAQs**

*   **What is the difference between this and Chrome’s enforcement of
            the [better ad standard](https://www.betterads.org/standards/)?**
            The current better ad standard states that sites cannot embed
            [stand-alone ads with unmuted
            audio](https://www.betterads.org/desktop-auto-playing-video-ad-with-sound/).
            This standard does not apply to content integral to the page (e.g.,
            video of a news article that goes along with the text). The autoplay
            policy is applicable to all video content regardless of its content.
*   **What is the MEI threshold and how will it apply to my site?** We
            are still working on the implementation and don’t have any data yet
            on what the threshold will be or how that will impact individual
            sites. Our general guidance is if your site offers video content
            exclusively, assume that autoplay will work. If it has mixed
            content, assume it won’t unless users interact with the site in some
            way.
*   **Can my site be exempted from the policy?** Unfortunately, Chrome
            cannot provide any exceptions to the autoplay policy.
*   **What counts as "user interaction on the domain?"** Any click on
            the document itself (this excludes scrolling) will count as user
            interaction.
*   **Will autoplay work if a user leaves my site after watching video
            and returns later in a browsing session.** Assuming the MEI
            threshold hasn't been met, autoplay will not work in that context.
            The user interaction requirement only applies to contiguous
            navigations.
*   **Will gestures and/or MEI score apply across tabs/windows?** If the
            user opens a link in a new tab via context menu, yes, the user
            gesture will be counted and the MEI will reflect any views on the
            new tab.
*   **How can I view my own MEI scores?** Navigate to
            chrome://media-engagement to see your personal scores. Individual or
            average scores will not be made available at this time.

More information

[Autoplay policy summary
presentation](https://docs.google.com/presentation/d/1DhW29bTLkDO6JSqp_wLUyByo00nI4krQ9laGQYQEJLU/edit?usp=sharing)

[Autoplay design
document](https://docs.google.com/document/d/1EH7qZatVnTXsBGvQc_53R97Z0xqm6zRblKg3eVmNp30/edit)

[Media engagement index (MEI) design
document](https://docs.google.com/document/d/1_278v_plodvgtXSgnEJ0yjZJLg14Ogf-ekAFNymAJoU/edit)

[Autoplay Policy Changes
(developers.google.com)](https://developers.google.com/web/updates/2017/09/autoplay-policy-changes)

[DOMException: The play() request was interrupted
(developers.google.com)](https://developers.google.com/web/updates/2017/06/play-request-was-interrupted)
