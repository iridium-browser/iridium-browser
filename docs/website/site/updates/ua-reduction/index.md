---
breadcrumbs:
- - /updates
  - updates
page_name: ua-reduction
title: User-Agent Reduction
---

[TOC]

## Updates

May 11th, 2023: the Phase 6 rollout is enabled for 100% of Android clients on M110 and
above via Finch.

April 25th, 2023: the Phase 6 rollout is enabled for 50% of Android clients on M110 and
above via Finch.

April 4th, 2023: the Phase 6 rollout is enabled for 10% of Android clients on M110 and
above via Finch.

March 21st, 2023: the Phase 6 rollout is enabled for 5% of Android clients on M110 and
above via Finch.

February 21st, 2023: the Phase 6 rollout is enabled for 1% of Android clients on M110 and
above via Finch.

February 7th, 2023: the Phase 5 rollout is enabled for 100% of clients on M107 and
above via Finch.

Jan 23rd, 2023: the Phase 5 rollout is enabled for 50% of clients on M107 and
above via Finch.

Jan 9th, 2023: the Phase 5 rollout is enabled for 10% of clients on M107 and
above via Finch.

Dec 7th, 2022: the Phase 5 rollout is enabled for 5% of clients on M107 and
above via Finch.

Nov 3rd, 2022: the Phase 5 rollout is enabled for 1% of clients on M107 and
above via Finch.

June 13th, 2022: the Phase 4 rollout is enabled for 100% of clients on M101 and
above via Finch.

June 10th, 2022: [Enabled the "ReduceUserAgentMinorVersion" feature by default](https://chromiumdash.appspot.com/commit/3fea2402264010af295c8dd87400368680100228)
in the M105 branch.

May 31, 2022: Phase 4 rollout was increased to an even larger percentage of the
Chrome 101 population while we evaluate stability.

May 16, 2022: Phase 4 rollout was increased to a larger percentage of the Chrome
101 population while we evaluate stability.

May 6, 2022: ["Phase 4"](https://www.chromium.org/updates/ua-reduction/#sample-ua-strings-phase-4)
began rolling out to a small percentage of the Chrome 101 stable population while we evaluate
stability.

April 11, 2022: Add a table to map from UA to UA-CH

March 3, 2022: Added frozen Chrome OS platform version to
<code>&lt;unifiedPlatform&gt;</code> value

February 18, 2022: Added information on the platforms for which User-Agent
reduction is applicable, as well as links to resources.

October 12, 2021: Information on using the Origin Trial with third-party embeds
was add to the [blog
post](https://developer.chrome.com/blog/user-agent-reduction-origin-trial/).

September 16, 2021: Chrome milestones were added to reflect
<https://blog.chromium.org/2021/09/user-agent-reduction-origin-trial-and-dates.html>.

May 24, 2021: The chrome://flags#freeze-user-agent flag was renamed to
chrome://flags/#reduce-user-agent in Chrome 93 and the values were updated to
align with the plan below (also testable via --enable-features=ReduceUserAgent).

## Proposed Rollout Plan

**Reduction Preparation**

Phase 1: Warn about accessing navigator.userAgent, navigator.appVersion, and
navigator.platform in DevTools, beginning in M92.

Phase 2: **Chrome 95 to Chrome 100** Launch an Origin Trial for sites to opt
into the final reduced UA string for testing and feedback, for at least 6
months.

**Reduction Rollout**

Phase 3: **Chrome 100** Launch a reverse Origin Trial, for instances where a
site may need more time for migration, for at least 6 months.

Phase 4: **Chrome 101** Ship reduced Chrome MINOR.BUILD.PATCH version numbers
(“0.0.0”). Once rolled-out, the reduced UA string would apply to all page loads
on desktop and mobile OSes that do not opt into the reverse Origin Trial.

Phase 5: **Chrome 107** Begin roll-out of reduced Desktop UA string and related
JS APIs (navigator.userAgent, navigator.appVersion, navigator.platform). Once
rolled-out, the reduced UA string would apply to all page loads on desktop OSes
that do not opt into the reverse Origin Trial.

Phase 6: **Chrome 110** Begin roll-out of reduced Android Mobile (and Tablet) UA
string and related JS APIs. Once rolled-out, the reduced UA string would apply
to all page loads on Android that do not opt into the reverse Origin Trial.

**Reduction Completion**
Phase 7: **Chrome 113** reverse Origin Trial ends and all page loads receive the
reduced UA string and related JS APIs.

## Reduced User Agent String Reference

This reduced format will be available for testing via chrome://flags/#reduce-user-agent in Chrome 93.

### Applicable platforms

User-Agent reduction will be applied to the following platforms: Windows, macOS,
Linux, Chrome OS, and Chrome on Android.  We don't have current plans for
User-Agent Reduction on iOS and Android WebView at this time.

### Unified Format

The unified format that covers all platforms post-UA Reduction looks like so:

<pre><samp>Mozilla/5.0 (<strong>&lt;unifiedPlatform&gt;</strong>) AppleWebKit/537.36 (KHTML, like Gecko)
Chrome/<strong>&lt;majorVersion&gt;</strong>.0.0.0 <strong>&lt;deviceCompat&gt;</strong> Safari/537.36
</samp></pre>

### Desktop

The Chrome Desktop User Agent string currently uses the following format:

<pre><samp>Mozilla/5.0 (<strong>&lt;platform&gt;</strong>; <strong>&lt;oscpu&gt;</strong>) AppleWebKit/537.36 (KHTML, like
Gecko) Chrome/<strong>&lt;majorVersion&gt;.&lt;minorVersion&gt</strong>; Safari/537.36
</samp></pre>

Post UA-Reduction, the new format will be:

<pre><samp>Mozilla/5.0 (<strong>&lt;unifiedPlatform&gt;</strong>) AppleWebKit/537.36 (KHTML, like Gecko)
Chrome/<strong>&lt;majorVersion&gt;</strong>.0.0.0 Safari/537.36
</samp></pre>

### Mobile and Tablet

The Chrome Mobile and Tablet User Agent strings use the following format:

<pre><samp>Mozilla/5.0 (Linux; Android <strong>&lt;androidVersion&gt;</strong>; <strong>&lt;deviceModel&gt;</strong>)
AppleWebKit/537.36 (KHTML, like Gecko)
Chrome/<strong>&lt;majorVersion&gt;.&lt;minorVersion&gt; &lt;deviceCompat&gt;</strong>
Safari/537.36
</samp></pre>

Post UA-Reduction, the new format will be:

<pre><samp>Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko)
Chrome/<strong>&lt;majorVersion&gt;</strong>.0.0.0 <strong>&lt;deviceCompat&gt;</strong> Safari/537.36
</samp></pre>

## Token Reference

<table>
  <tr>
    <td><strong>Tokens</strong>
    <td><strong>Description</strong>
  <tr>
    <td><samp>&lt;androidVersion&gt;</samp>
    <td>Represents Android major version
  <tr>
    <td><samp>&lt;deviceModel&gt;</samp>
    <td>Represents Android device model.
  <tr>
    <td><samp>&lt;minorVersion&gt;</samp>
    <td>Represents the Chrome MINOR.BUILD.PATCH <a href="/developers/version-numbers">version numbers</a>.
  <tr>
    <td><samp>&lt;oscpu&gt;</samp>
    <td>Represents the device operating system and (optionally) CPU architecture.
  <tr>
    <td><samp>&lt;platform&gt;</samp>
    <td>Represents the underlying device platform.
  <tr>
    <th>Post-Reduction Tokens
    <th>
  <tr>
    <td><samp>&lt;deviceCompat&gt;</samp>
    <td>
      Represents device form-factor.
      <p>The possible values are:
      <ul>
        <li><samp>"Mobile"</samp>
        <li><samp>""</samp> (empty string, used by Tablets and Desktop)
      </ul>
  <tr>
    <td><samp>&lt;majorVersion&gt;</samp>
    <td>Represents the Chrome major version.
  <tr>
    <td><samp>&lt;unifiedPlatform&gt;</samp>
    <td>
      The intersection of <samp>&lt;platform&gt;</samp>,
      <samp>&lt;oscpu&gt;</samp>, <samp>&lt;androidVersion&gt;</samp>,
      and <samp>&lt;deviceModel&gt;</samp>, depending on device.
      <p>The possible desktop values* are:
      <ul>
        <li><samp>Windows NT 10.0; Win64; x64</samp>
        <li><samp>Macintosh; Intel Mac OS X 10_15_7</samp>
        <li><samp>X11; Linux x86_64</samp>
        <li><samp>X11; CrOS x86_64 14541.0.0</samp>
        <li><samp>Fuchsia</samp>
      </ul>
      <p>The possible mobile values* are:
      <ul>
        <li><samp>Linux; Android 10; K</samp>
      </ul>
      <p><em>* Note that these strings are literal values; they will
        not update even if a user is on an updated operating system
        or device.</em></p>
</table>

## UA Token to UA-CH Mapping

<table>
  <tr>
    <th>UA string token</th>
    <th>
      HTTP UA-CH token
      (<a href="https://wicg.github.io/ua-client-hints/#http-ua-hints">ref</a>)
    </th>
    <th>
      UA-CH JS API
      (<a href="https://wicg.github.io/ua-client-hints/#interface">ref</a>)
    </th>
  </tr>
  <tr>
    <td>&lt;androidVersion&gt;</td>
    <td><code>Sec-CH-UA-Platform-Version</code></td>
    <td><code>UADataValues.platformVersion</code></td>
  </tr>
  <tr>
    <td>&lt;deviceCompat&gt;</td>
    <td><code>Sec-CH-UA-Mobile</code></td>
    <td><code>NavigatorUAData.mobile</code></td>
  </tr>
  <tr>
    <td>&lt;deviceModel&gt;</td>
    <td><code>Sec-CH-UA-Model</code></td>
    <td><code>UADataValues.model</code></td>
  </tr>
  <tr>
    <td>&lt;majorVersion&gt;</td>
    <td><code>Sec-CH-UA</code></td>
    <td><code>NavigatorUAData.brands</code></td>
  </tr>
  <tr>
    <td>&lt;minorVersion&gt;</td>
    <td><code>Sec-CH-UA-Full-Version-List</code></td>
    <td><code>UADataValues.fullVersionList</code></td>
  </tr>
  <tr>
    <td>&lt;oscpu&gt;</td>
    <td>
      Any combination of the following, as relevant:<br>
      <code>Sec-CH-UA-WoW64</code>, <code>Sec-CH-UA-Arch</code>,
      <code>Sec-CH-UA-Bitness</code>
    </td>
    <td>
      Any combination of the following, as relevant:<br>
      <code>UADataValues.wow64</code>, <code>UADataValues.arch</code>,
      <code>UADataValues.bitness</code>
    </td>
  </tr>
  <tr>
    <td>&lt;unifiedPlatform&gt;</td>
    <td>
      <code>Sec-CH-UA-Platform</code> and
      <code>Sec-CH-UA-Platform-Version</code>
    </td>
    <td>
      <code>UADataValues.platform</code> and
      <code>UADataValues.platformVersion</code>
    </td>
  </tr>
</table>

## Sample UA Strings: Phase 4

In Phase 4 we change the <code>&lt;minorVersion&gt;</code> token to “0.0.0”.

<table>
  <tr>
    <th>
    <th>Desktop (user on Windows 8.1, for example)
  <tr>
    <th>Phase 3 UA
    <td><samp>Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<del>0.1234.56</del> Safari/537.36</samp>
  <tr>
    <th>Phase 4 UA
    <td><samp>Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<ins>0.0.0</ins> Safari/537.36</samp>
</table>

<table>
  <tr>
    <th>
    <th>Mobile (user on Samsung Galaxy, for example)
  <tr>
    <th>Phase 3 UA
    <td><samp>Mozilla/5.0 (Linux; Android 9; SM-A205U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<del>0.1234.56</del> Mobile Safari/537.36</samp>
  <tr>
    <th>Phase 4 UA
    <td><samp>Mozilla/5.0 (Linux; Android 9; SM-A205U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<ins>0.0.0</ins> Mobile Safari/537.36</samp>
</table>

<table>
  <tr>
    <th>
    <th>Tablet (user on Samsung Galaxy, for example)
  <tr>
    <th>Phase 3 UA
    <td><samp>Mozilla/5.0 (Linux; Android 9; SM-T810) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<del>0.1234.56</del> Safari/537.36</samp>
  <tr>
    <th>Phase 4 UA
    <td><samp>Mozilla/5.0 (Linux; Android 9; SM-T810) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<ins>0.0.0</ins> Safari/537.36</samp>
</table>

## Sample UA Strings: Phase 5

In Phase 5 we change the <code>&lt;platform&gt;</code> and
<code>&lt;oscpu&gt;</code> tokens from their
platform-defined values to the relevant <code>&lt;unifiedPlatform&gt;</code>
token value (which will never change).

Note: There may not be user-visible changes here, unless the user was on a lower
version.

Also note that the macOS platform version was already [capped to
10_15_7](https://bugs.chromium.org/p/chromium/issues/detail?id=1175225) in
Chrome 90 for site compatibility reasons.

<table>
  <tr>
    <th>
    <th>Desktop (user on Windows 8.1, for example)
  <tr>
    <th>Phase 4 UA
    <td><samp>Mozilla/5.0 (Windows NT <del>6.3</del>; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Safari/537.36</samp>
  <tr>
    <th>Phase 5 UA
    <td><samp>Mozilla/5.0 (Windows NT <ins>10.0</ins>; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Safari/537.36</samp>
</table>

<table>
  <tr>
    <th>
    <th>Mobile (user on Samsung Galaxy, for example)
  <tr>
    <th>Phase 4 UA
    <td><samp>Mozilla/5.0 (Linux; Android 9; SM-A205U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Mobile Safari/537.36</samp>
  <tr>
    <th>Phase 5 UA
    <td><em>(No changes for Mobile UAs in Phase 5)</em>
</table>

<table>
  <tr>
    <th>
    <th>Tablet (user on Samsung Galaxy, for example)
  <tr>
    <th>Phase 4 UA
    <td><samp>Mozilla/5.0 (Linux; Android 9; SM-T810) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Safari/537.36</samp>
  <tr>
    <th>Phase 5 UA
    <td><em>(No changes for Tablet UAs in Phase 5)</em>
  </tr>
</table>

## Sample UA Strings: Phase 6

In Phase 6, we change the <code>&lt;deviceModel&gt;</code> token to “K” and
change the <code>&lt;androidVersion&gt;</code> token to a static “10” string.

<table>
  <tr>
    <th>
    <th>Desktop (user on Windows 8.1, for example)
  <tr>
    <th>Phase 5 UA
    <td><samp>Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Safari/537.36</samp>
  <tr>
    <th>Phase 6 UA
    <td><em>(No changes for Desktop UAs from Phase 5)</em>
</table>

<table>
  <tr>
    <th>
    <th>Mobile (user on Samsung Galaxy, for example)
  <tr>
    <th>Phase 5 UA
    <td><samp>Mozilla/5.0 (Linux; Android <del>9; SM-A205U</del>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Mobile Safari/537.36</samp>
  <tr>
    <th>Phase 6 UA
    <td><samp>Mozilla/5.0 (Linux; Android <ins>10; K</ins>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Mobile Safari/537.36</samp>
</table>

<table>
  <tr>
    <th>
    <th>Tablet (user on Samsung Galaxy, for example)
  </tr>
  <tr>
    <th>Phase 5 UA
    <td><samp>Mozilla/5.0 (Linux; Android <del>9; SM-T810</del>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Safari/537.36</samp>
  <tr>
    <th>Phase 6 UA
    <td><samp>Mozilla/5.0 (Linux; Android <ins>10; K</ins>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.0.0 Safari/537.36</samp>
</table>

## Sample UA Strings: Final Reduced State

<table>
  <tr>
    <th>
    <th>Desktop (user on Windows 8.1, for example)
  <tr>
    <th>Old UA
    <td><samp>Mozilla/5.0 (Windows NT <del>6.3</del>; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<del>0.1234.56</del> Safari/537.36</samp>
  <tr>
    <th>Final Reduced UA
    <td><samp>Mozilla/5.0 (Windows NT <del>10.0</del>; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<ins>0.0.0</ins> Safari/537.36</samp>
</table>

<table>
  <tr>
    <th>
    <th>Mobile (user on Samsung Galaxy, for example)
  <tr>
    <th>Old UA
    <td><samp>Mozilla/5.0 (Linux; Android <del>9; SM-A205U</del>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<del>0.1234.56</del> Mobile Safari/537.36</samp>
  <tr>
    <th>Final Reduced UA
    <td><samp>Mozilla/5.0 (Linux; Android <ins>10; K</ins>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<ins>0.0.0</ins> Mobile Safari/537.36</samp>
</table>

<table>
  <tr>
    <th>
    <th>Tablet (user on Samsung Galaxy, for example)
  <tr>
    <th>Old UA
    <td><samp>Mozilla/5.0 (Linux; Android <del>9; SM-T810</del>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<del>0.1234.56</del> Safari/537.36</samp>
  <tr>
    <th>Final Reduced UA
    <td><samp>Mozilla/5.0 (Linux; Android <ins>10; K</ins>) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.<ins>0.0.0</ins> Safari/537.36</samp>
</table>

## Reduced navigator.platform values (for all versions)

<table>
  <tr>
    <th>Platform
    <th>Reduced value
  <tr>
    <td>macOS
    <td><samp>MacIntel</samp>
  <tr>
    <td>Windows
    <td><samp>Win32</samp>
  <tr>
    <td>Chrome OS
    <td><samp>Linux x86_64</samp>
  <tr>
    <td>Linux
    <td><samp>Linux x86_64</samp>
  <tr>
    <td>Android
    <td><samp>Linux armv81</samp>
</table>

## Reduced navigator.appVersion values

<code>navigator.appVersion</code> is effectively an alias of <code>navigator.userAgent</code> (it’s
[everything after
“Mozilla/”](https://source.chromium.org/chromium/chromium/src/+/HEAD:third_party/blink/renderer/core/frame/navigator_id.cc;l=56?q=appVersion&ss=chromium)).

## Reduced navigator.userAgent values

To avoid confusion and reduce implementation complexity, we aim to follow the
same plan for <code>navigator.userAgent</code>.

## Alternative: high entropy client hints

All of the information that was contained in the User-Agent string prior to
reduction is available through the high entropy client hints, which are available
by request through the [User-Agent Client Hints](https://github.com/WICG/ua-client-hints)
request headers, as well as the [navigator.userAgentData.getHighEntropyValues](https://developer.mozilla.org/en-US/docs/Web/API/NavigatorUAData/getHighEntropyValues)
Javascript API.

## FAQ

- **What do I do after the reduction deprecation OT ends in May 2023?**

  Be sure to [prepare and test](https://developer.chrome.com/en/docs/privacy-sandbox/user-agent/#prepare-and-test) if you are OK with the [default values](https://web.dev/migrate-to-ua-ch/#are-you-only-using-basic-user-agent-data) provided by the reduced UA and if not then [migrate to User-Agent Client Hints](https://web.dev/migrate-to-ua-ch/).

- <a name="faq-ssp"></a>**I'm an SSP. How can I be ready for UACH?**

  - **Considerations**

    To continue to get accurate values you will need to call the User-Agent Client Hints API (“UA-CH”) which is accessed via JavaScript or HTTP Headers.

    [Open RTB 2.6](https://iabtechlab.com/wp-content/uploads/2022/04/OpenRTB-2-6_FINAL.pdf) introduces a new field for Structured UA in section 3.2.29 which standardized how you would pass it into the bidstream after populating from UA-CH.

    To minimize disruption for ongoing campaigns, we recommend integrating with UA-CH, passing the data in the bidstream following the OpenRTB 2.6 spec, and notifying your DSP partners of this change ensuring they are looking at `device.sua`.

    If you need more time, you can retain access through late May 2023 via the [reduction deprecation trial](https://developer.chrome.com/blog/user-agent-reduction-deprecation-trial/).

  - **Questions to ask your team**
    *	Have you already integrated with User-Agent Client Hints and are passing that data into the bidstream?
    *	Are you passing UA-CH into the bidstream in line with the OpenRTB 2.6 spec? (see section 3.2.29 which describes capturing structured UA within device.sua).
    *	Have you coordinated with your buy-side partners to ensure they are aware of and relying on the new Structured UA fields within your bidstream?

    Answered yes to all the above? Great! You are ready and successfully migrated to UA-CH. If you answered no to any of the prior questions please make sure you review "Actions to take" below to ensure you're ready.

  - **Actions to take**
    * Integrate with UA-CH
	  * See [Migrate to User-Agent Client Hints](https://web.dev/migrate-to-ua-ch/) for recommendations on how to integrate with the API through HTTP Headers or JavaScript.
	  * For an API overview, see [this introductory article](https://developer.chrome.com/articles/user-agent-client-hints/).
    * Updated your bidstream to include UA-CH
	  * We recommend updating your bidstream spec following section 3.2.29 of [Open RTB 2.6 spec](https://iabtechlab.com/wp-content/uploads/2022/04/OpenRTB-2-6_FINAL.pdf).
	  * If there are technical blockers to updating to [Open RTB 2.6](https://iabtechlab.com/wp-content/uploads/2022/04/OpenRTB-2-6_FINAL.pdf), consider following the device.sua data structure recommended in 3.2.29 to include UACH values within the `ext object` to enable continued access to your DSP partners.
    * (Optional) Extend access to the legacy User-Agent string
	  * Companies who need more time to migrate to the UA-CH API can join the [reduction deprecation trial](https://developer.chrome.com/blog/user-agent-reduction-deprecation-trial/) that extends access to the legacy User-Agent string through the end of May 2023.
	  * Once registered, you will need to [update your HTTP Response headers](https://developer.chrome.com/blog/user-agent-reduction-deprecation-trial/#setup) for your production traffic.
	  * While this provides access until May 23, 2023, you should still integrate with UA-CH to continue to have access to accurate values.
    * Raise awareness to your DSP partners
	  * Reach out to your DSP partners and let them know you are passing ua information like device model and OS version in the `device.sua` of OpenRTB 2.6.
	  * If they continue to rely only on the `device.ua`, they will no longer be able to access information like device model or Android OS version.

## Resources

### Education
- What is the [Privacy Sandbox](https://privacysandbox.com/)?
- What are [User Agent Client Hints](https://web.dev/user-agent-client-hints/)?
- What happens to the [User Agent string](https://web.dev/user-agent-client-hints/#what-happens-to-the-user-agent-string)?
- Public GitHub: [UA-CH Explainer](https://github.com/WICG/ua-client-hints)
- Public feedback: [Open issues](https://github.com/WICG/ua-client-hints/issues)
- Public support: [Sandbox-Dev-Support-Repo](https://github.com/GoogleChromeLabs/privacy-sandbox-dev-support/discussions)

### Proposed Timeline
- Full [UA-Reduction proposed rollout plan](https://www.chromium.org/updates/ua-reduction/)
- Key Dates (As of Mar 15th, 2023) ([schedule](https://chromiumdash.appspot.com/schedule))
  - Chrome 100 release (Mar 29th, 2022) (**Deployed**)
    - User Agent Reduction Deprecation Trial launches for instances where a site may need more migration time.
  - Chrome 101 release (Apr 26th, 2022) (**Deployed**)
    - Minor build version is frozen to 0.0.0 and applies to all page loads on desktop or mobile that are not part of the Reverse Origin Trial.
  - Chrome 107 release (Oct 25th, 2022) (**Deployed**)
    - Desktop based UA string is fully reduced for all page loads that are not part of the Reverse Origin Trial.
  - Chrome 110 release (Feb 7th, 2023) (**Deployed**)
    - Android Mobile and Tablet based UA string is fully reduced for all page loads that are not part of the Reduction Deprecation Trial.
- If additional time is needed for migration you may register for the [Reduction Deprecation Trial](https://developer.chrome.com/blog/user-agent-reduction-deprecation-trial/) which will provide access to the full UA string until late May, 2023.



### Migration
- [Migrating to User Agent Client Hints](https://web.dev/migrate-to-ua-ch/) (Full guide)
- [Strategy](https://web.dev/migrate-to-ua-ch/#strategy-on-demand-client-side-javascript-api) for navigator.userAgent users (JS API)
- [Strategy](https://web.dev/migrate-to-ua-ch/#strategy-static-server-side-header) for Static server-side headers
- [Delegating access](https://web.dev/migrate-to-ua-ch/#strategy:-delegating-hints-to-cross-origin-requests) of your high entropy hints to 3P resources
- Handling critical [first request](https://web.dev/migrate-to-ua-ch/#strategy:-server-side-hints-required-on-first-request) client hints beyond the default set
  - The default set of hints includes the browser name with major version, platform, and mobile indicator.

### Testing
- Chrome Browser - Please use chrome://flags/#reduce-user-agent
- Chrome Webdriver (Selenium testing, regression testing)
--enable-features=ReduceUserAgent
- [User-Agent reduction code snippets](https://developer.chrome.com/docs/privacy-sandbox/user-agent/snippets/)
- [Headers &amp; JS API interactive demo](https://web.dev/user-agent-client-hints/#demo)
