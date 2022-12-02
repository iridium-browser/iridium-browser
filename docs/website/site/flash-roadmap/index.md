---
breadcrumbs: []
page_name: flash-roadmap
title: Flash Roadmap
---

[TOC]

## Upcoming Changes

### Flash Support Removed from Chromium (Target: Chrome 88+ - Jan 2021)

#### Summary

Flash support/ capability will be complete removed from Chromium. It will no
longer be possible to enable Flash Player with Enterprise policy in Chrome 88+.

#### **Rationale**

Align with Adobe's [announced
plan](https://blog.adobe.com/en/publish/2017/07/25/adobe-flash-update.html#gs.mglfvc)
to end support.

#### Announcements/ Artifacts

*   Tracking [issue](http://crbug.com/1064647) for the removal of Flash
            Player support.

#### **Notes for Administrators**

*   [How to audit existing Flash
            usage](https://cloud.google.com/blog/products/chrome-enterprise/preparing-your-enterprise-for-flash-deprecation)
*   HARMAN offers a number of commercial [support
            options](https://services.harman.com/partners/adobe) for Flash
            Player beyond 2020. These can be used in complement with Chrome's
            [Legacy Browser Support
            (LBS)](https://support.google.com/chrome/a/answer/9270076?hl=en),
            allowing for both to remain secure and up to date.

### Support for AllowOutdatedPlugins disabled (Target: All Chrome versions - Sept 2021)

#### Summary

It will no longer be possible to enable Flash Player, via Enterprise policy
([AllowOutdatedPlugins](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=AllowOutdatedPlugins)),
in versions of Chrome before Chrome 88 on Windows, Mac, and Linux. ChromeOS will
continue to allow the use of the
[AllowOutdatedPlugins](https://cloud.google.com/docs/chrome-enterprise/policies/?policy=AllowOutdatedPlugins)
policy.

#### Rationale

We strongly encourage Enterprises to migrate away from Flash Player or explore
solutions that leverage LBS to remain on a modern, updated, and secure version
of Chrome.

#### Announcements/ Artifacts

*   [1064657](https://bugs.chromium.org/p/chromium/issues/detail?id=1064657)
            : Update Plugin Meta Data to block Flash Player

#### Notes for Administrators

*   [How to audit existing Flash
            usage](https://cloud.google.com/blog/products/chrome-enterprise/preparing-your-enterprise-for-flash-deprecation)
*   HARMAN offers a number of commercial [support
            options](https://services.harman.com/partners/adobe) for Flash
            Player beyond 2020. These can be used in complement with Chrome's
            [Legacy Browser Support
            (LBS)](https://support.google.com/chrome/a/answer/9270076?hl=en),
            allowing for both to remain secure and up to date.

---

## Shipped Changes

### Plugin Power Savings Mode (Shipped: Chrome 42 - Sept 2015)

#### Summary

Chrome pauses non-essential(1) Flash Content, by replacing the plugin content
with a static image preview and a play button overlayed. Users can re-enable
this content by clicking play.

(1) - Non-essential content being smaller than 300x400 pixels or smaller than
5x5 pixels.

#### Rationale

Limit Flash Playbacks to visible main body content (e.g. video, games, etc...)
and still permit streaming audio services to function.

#### Announcements/ Artifacts

*   [Design
            Doc](https://docs.google.com/document/d/1r4xFSsR4gtjBf1gOP4zHGWIFBV7WWZMgCiAHeepoHVw/edit#%5C)

### Plugin Power Savings Mode - Tiny (Shipped: Chrome 53 - Sept 2016)

#### Summary

A further restriction to Plugin Power Savings Mode that removes the ability to
run 5x5 or smaller content, from a different origin.

#### Rationale

Much of this content (5x5 below) was used for viewability detection (i.e. to see
if an ad was on that page), requiring Chrome to spin up a relatively expensive
(in terms of performance) Flash process in order for the site to infer
viewability.

With the introduction of Intersection Observer in Chrome 51, which added
platform support for this use case, there was no longer a need to continue
granting this exception.

We left an exception for "same origin" 5x5 Flash content, to give smaller sites
(e.g. using things like clipboard access) time to migrate.

#### Announcement/ Artifacts

*   [Intent to
            Implement](https://groups.google.com/a/chromium.org/forum/#!topic/chromium-dev/QL2K4yFVg_U)

### YouTube Embed Re-Writer (Shipped: Chrome 54 - Oct 2016)

#### Summary

Chrome to automatically use the HTML5 content of a YouTube embed when the Flash
one is used.

#### Rationale

This will allow the long tail of websites that never updated to the HTML5 embeds
to no longer require Flash for Chrome users, thus reducing overall usage of
Flash in Chrome.

#### Announcement/ Artifacts

*   [Intent to
            Implement](https://groups.google.com/a/chromium.org/forum/#!msg/chromium-dev/BW8g1iB0jLs/OeWeRCTuBAAJ)
*   [Design
            Doc](https://docs.google.com/document/d/1xHBaX2WhfZVmfeWyLNOX76l9cPiXrQNXqRrUMtlvfks/edit)

### De-couple Flash Player (Shipped: Chrome 54 - Oct 2016)

#### Summary

Chrome to exclusively use the component updater to distribute Flash Player, and
separating it from Chrome's default distribution bundle.

#### Rationale

Enable Chrome to rapidly distribute Flash Player updates, without re-building
the core product, making it easier to match Adobe's monthly release cadence.

This feature was fundamentally technology gated, requiring development of
in-line on-demand Flash component installs, differential component updates, and
building out special serving infrastructure.

#### Announcement/ Artifacts

N/A

### HTML5 By Default (Target: Chrome 55+ - Dec 2016)

#### Summary

Navigator.Plugins() and Navigator.MimeTypes() will only report the presence of
Flash Player if the user has indicated that the domain should execute Flash. If
a site offers an HTML5 experience, this change will make that the primary
experience. We will continue to ship Flash Player with Chrome, and if a site
truly requires Flash, a prompt will appear at the top of the page when the user
first visits that site, giving them the option of allowing it to run for that
site (see the
[proposal](https://docs.google.com/presentation/d/106_KLNJfwb9L-1hVVa4i29aw1YXUy9qFX-Ye4kvJj-4/edit#slide=id.p)
for the mock-ups).

#### Announcements/ Artifacts

*   [Proposal](https://docs.google.com/presentation/d/106_KLNJfwb9L-1hVVa4i29aw1YXUy9qFX-Ye4kvJj-4/edit#slide=id.p)
*   [Design
            Doc](https://docs.google.com/a/chromium.org/document/d/1QD3HQdkBgJea_92f5rars06Bd_SJz5IXjuNHhBuWCcg/edit?usp=sharing)
*   [Intent to
            Implement](https://groups.google.com/a/chromium.org/forum/#!searchin/chromium-dev/HTML5$20by$20default/chromium-dev/0wWoRRhTA_E/__E3jf40OAAJ)
*   [Site Engagement
            Scoring](/developers/design-documents/site-engagement)
*   [Updated Intent to Implement (using Site Engagement
            scoring)](https://groups.google.com/a/chromium.org/d/msg/chromium-dev/ad7Posd6cdI/5EEOduWiCwAJ)

#### Status

HTML5 by default has shipped and we are currently in the process of ramping up
the SEI threshold, per the schedule below.

Currently 87.5% of population have an SEI threshold score of 4 and 12.5% has a
threshold score of 8 (we do this to measure the impact of the threshold change).
By the end of the month we will progress on to the next phase on the ramp and
everyone will be at least at 8.

#### Shipping Schedule

HTML5 by Default was initially rolled out to 1% of Chrome 55 Stable users
(December), followed by a full deployment (i.e. to 100% of users) in Chrome 56
Stable (February).

Flash prompting will only be enabled for sites whose [Site Engagement
Index](/developers/design-documents/site-engagement) (SEI) is below a certain
threshold. For Chrome 55, starting in January 2017 prompts will only appear for
sites where the user’s SEI is less than 1. That threshold will increase to 100
through October 2017, when all Flash sites will require an initial prompt. As a
reminder, users will only be prompted once per site.

Here’s a summary of thresholds and % of users:

<table>
<tr>

<td>Site Engagement Threshold</td>

<td>User % Enabled</td>

</tr>
<tr>

<td>January 2017</td>

<td>1 (Stable 55)</td>

<td>1% (Stable 55), 50% (Beta 56)</td>

</tr>
<tr>

<td>February 2017</td>

<td>2</td>

<td>100% (Stable 56)</td>

</tr>
<tr>

<td>March 2017</td>

<td>4</td>

<td>100%</td>

</tr>
<tr>

<td>April 2017</td>

<td>8</td>

<td>100%</td>

</tr>
<tr>

<td>May 2017</td>

<td>16</td>

<td>100%</td>

</tr>
<tr>

<td>June 2017</td>

<td>32</td>

<td>100%</td>

</tr>
<tr>

<td>July 2017</td>

<td>32</td>

<td>100%</td>

</tr>
<tr>

<td>August 2017</td>

<td>32</td>

<td>100%</td>

</tr>
<tr>

<td>September 2017</td>

<td>64</td>

<td>100%</td>

</tr>
<tr>

<td>October 2017</td>

<td>100</td>

<td>100%</td>

</tr>
</table>

#### Developer Recommendations

Ultimately we recommend migrating towards HTML5 content, however for sites that
still require Flash Player in the interim we recommend presenting users with a
link/ image to "Enable" Flash Player that points to
"<https://get.adobe.com/flashplayer/>." When users click on that link Chrome
will present the necessary UI to enable Flash Player for the site. It will look
something like
[this](https://docs.google.com/presentation/d/106_KLNJfwb9L-1hVVa4i29aw1YXUy9qFX-Ye4kvJj-4/edit#slide=id.g1270f83468_0_12).

> `<a href="https://get.adobe.com/flashplayer">Enable Flash</a>`

> `<a href="https://get.adobe.com/flashplayer"><img border="0" alt="Enable
> Flash" src="enable_flash.gif"/></a>`

### PPS Tiny - Remove Un-sized (0x0 or hidden) content Exceptions (Target: Chrome 59 - June 2017)

#### Summary

We are removing the exception for 0x0 (unsized/hidden) content from Plugin Power
Savings Mode.

#### Rationale

We originally intended on blocking this type of content in the PPS Tiny launch,
however due to a technical oversight we unintentionally left the exception in.
This change brings the implementation in line with what we originally
communicated and intended.

#### Announcements/ Artifacts

N/A

### PPS Tiny - No Same Origin Exceptions (Target: Chrome 60 - Aug 2017)

#### Summary

We are removing the final exception for Plugin Power Savings Mode, which
permitted small (5x5) content, hosted on the same origin, to run.

#### Rationale

The exception was meant to be a temporary relief for smaller developers, for
features that are well now very supported by the web platform (e.g. clipboard
access, audio streams, etc...).

#### Announcements/ Artifacts

*   [Intent to
            Implement](https://groups.google.com/a/chromium.org/d/msg/chromium-dev/Elg7Vhpeb38/zQCvXQ_vAAAJ)

### Remove "Prefer HTML over Flash" from chrome://flags (Target: Chrome 61 - Sept 2017)

#### Summary

Remove the unsupported experimental flag "Prefer HTML over Flash."

#### Rationale

We typically remove feature flags once a feature has launched. However, for this
case, given the impact of the change, we left the flag in for an extended period
to assist developers with debugging the change in behavior. This change will
also help remove the number of similar (but somewhat distinct) control points
for Flash Player and reduce the overall complexity of configuration, leading up
to the settings simplification in the following release.

#### Announcements/ Artifacts

*   Tracking
            [issue](https://bugs.chromium.org/p/chromium/issues/detail?id=709550)

### Unify Flash Settings (Target: Chrome 62 - Oct 2017)

#### Summary

Transition the current Flash settings to a single On/Off toggle. If On, Chrome
run consistently with HTML5 by Default (i.e. prompting users to enable Flash
Player on a per site basis), if Off Flash Player will be blocked. In effect,
this change is removing a UI option to always allow Flash Player to run without
prompting. Users will still be able to manually add wildcard exceptions, which
afford similar capabilities as always allow (albeit in a way that's less obvious
to configure).

Existing users who have set "Always Allow" and Ask (HTML5 by Default) will get
migrated to On. Chrome will not automatically create any wildcard exceptions in
the transition.

#### Rationale

The intent is to simplify the user choice down to a single option, enable Flash
Player (default == enabled), that is easier for users to understand. Power users
will be able to add exceptions (including those with wildcards) explicitly Allow
Flash to run.

#### Announcements/ Artifacts

*   TBD

### Non-Persisted HTML5 by Default (Target: Chrome 69 - September 2018)

#### Summary

Sites using Flash will require explicit permission to run, every time the user
restarts the browser.

#### Rationale

Require affirmative user choice to run Flash Player content, without that choice
persisting across multiple sessions.

#### Announcements/ Artifacts

TBD

### Flash Disabled by Default (Target: Chrome 76+ - July 2019)

#### Summary

Flash will be disabled by default, but can be enabled in Settings at which point
explicit permission is still required for each site when the browser is
restarted.
**Rationale**

Require affirmative user choice to run Flash Player.

#### Announcements/ Artifacts

*   [Flash Deprecation Infobar
            Warning](https://docs.google.com/document/d/1AmICiNrG2Cp2vCuu80Sqqz1EDKlKGgn2m2PUGzdZ6i0/edit)
            - [918428](https://crbug.com/918428)

### Additional warning on Flash Player activation prompt (Target: Chrome 83 - May 2020)

#### Summary

When users go to active Flash Player for an origin/ session we will show an
additional line of text "Flash Player will no longer be supported after December
2020." in the activation prompt. The dialog will also include a (?) icon that
links to blog post, giving additional background and context.

#### Rationale

This change will make the deprecation timing more prominent, particularly in
settings where the Infobar on startup warning might not appear (i.e., policy
configurations that enable Flash Player).

#### Announcements/ Artifacts

*   TBD

### Remove Support for Hostname Wildcards for PluginsAllowedForUrls (Target: Chrome 85 - Aug 2020)

#### Summary

Remove the ability to define Flash Player content settings that use wildcards in
the hostname (e.g., “https://\*” or “https://\[\*.\]mysite.foo”).

#### Rationale

The change requires that administrators to audit their Flash usage and
explicitly add urls that they want to automatically enable Flash Player support
for.

#### Announcements/ Artifacts

TBD

### Remove the Ability for Extensions to Inject Flash Content Settings (Target: Chrome 86 - Oct 2020)

#### Summary

Remove the ability for extensions to inject Flash Player content settings.

#### Rationale

Ensure that all non-policy enabled Flash content requires per session
activation, which coupled with the warning in the activation prompt should help
to increase awareness of the impending change in support.

#### Announcements/ Artifacts

TBD

### Flash Player blocked as "out of date" (Target: All Chrome versions - Jan 2021)

#### Summary

Flash Player will be marked as out of date and will be blocked from loading.

#### Rationale

Align with Adobe's [announced
plan](https://blog.adobe.com/en/publish/2017/07/25/adobe-flash-update.html#gs.mglfvc)
to end support.

#### Announcements/ Artifacts

TBD

### ---

## Relevant Links

### Adobe

*   [Update for Enterprise Customers using Adobe Flash
            Player](https://theblog.adobe.com/update-for-enterprise-adobe-flash-player/)
            (Apr 2020)
*   [Adobe Flash Player EOL General Information
            Page](https://www.adobe.com/products/flashplayer/end-of-life.html)
            (Apr 2020)
*   [Adobe Flash Player EOL Enterprise Information
            Page](https://www.adobe.com/products/flashplayer/enterprise-end-of-life.html)
            (Apr 2020)
*   [Adobe Flash
            Update](https://blog.adobe.com/en/publish/2017/07/25/adobe-flash-update.html#gs.mglfvc)
            (July 2017)
*   [Flash, HTML5 and Open Web
            Standards](https://blogs.adobe.com/conversations/2015/11/flash-html5-and-open-web-standards.html)
            (November 2015)

### Apple

*   [Adobe Announces Flash Distribution and Updates to
            End](https://webkit.org/blog/7839/adobe-announces-flash-distribution-and-updates-to-end/)
            (July 2017)
*   [Next Steps for Legacy
            Plug-ins](https://webkit.org/blog/6589/next-steps-for-legacy-plug-ins/)
            (June 2016)

### Facebook

*   [Flash End of Life Approaching - Options for
            Developers](https://www.facebook.com/fbgaminghome/blog/flash-end-of-life-approaching-options-for-developers)
            (June 2020)
*   [Important Changes to Gameroom and Web Games on
            Facebook](https://www.theverge.com/2020/9/28/21459607/original-farmville-facebook-shut-down-flash-zynga)
            (July 2020)
*   [Migrating Games from Flash to Open Web Standards on
            Facebook](https://developers.facebook.com/blog/post/2017/07/25/Games-Migration-to-Open-Web-Standards/)
            (July 2017)

### Google

*   [Saying goodbye to Flash in
            Chrome](https://www.blog.google/products/chrome/saying-goodbye-flash-chrome/)
            (July 2017)
*   [So long, and thanks for all the
            Flash](https://blog.chromium.org/2017/07/so-long-and-thanks-for-all-flash.html)
            (July 2017)
*   [Admin Essentials: Preparing your enterprise for Flash
            deprecation](https://cloud.google.com/blog/products/chrome-enterprise/preparing-your-enterprise-for-flash-deprecation)
            (May 2020)

### Microsoft

*   [Update on Adobe Flash Player End of
            Support](https://blogs.windows.com/msedgedev/2020/09/04/update-adobe-flash-end-support/)
            (September 4, 2020)
*   [Flash on Windows
            Timeline](https://blogs.windows.com/msedgedev/2017/07/25/flash-on-windows-timeline/)
            (July 2017)
*   [Putting Users in Control of
            Flash](https://blogs.windows.com/msedgedev/2016/04/07/putting-users-in-control-of-flash/)
            (April 2016)
*   [Extending User Control of Flash with
            Click-to-Run](https://blogs.windows.com/msedgedev/2016/12/14/edge-flash-click-run/)
            (December 2016)

### Mozilla

*   [Firefox Roadmap for Flash End of
            Life](https://blog.mozilla.org/futurereleases/2017/07/25/firefox-roadmap-for-flash-end-of-life/)
            (July 2017)
*   [Mozilla Flash
            Roadmap](https://developer.mozilla.org/en-US/docs/Plugins/Roadmap)
*   [NPAPI Plugins in
            Firefox](https://blog.mozilla.org/futurereleases/2015/10/08/npapi-plugins-in-firefox/)
            (October 2015)
*   [Reducing Adobe Flash Usage in
            Firefox](https://blog.mozilla.org/futurereleases/2016/07/20/reducing-adobe-flash-usage-in-firefox/)
            (July 2016)

**NCSC**

*   [Enterprise patching in a post-Flash
            world](https://www.ncsc.gov.uk/blog-post/enterprise-patching-in-a-post-flash-world)
            (Sept 2020)

**Unity**

*   [Unity and creating content in a Post-Flash
            World](https://blogs.unity3d.com/?p=52557) (July 2017)
