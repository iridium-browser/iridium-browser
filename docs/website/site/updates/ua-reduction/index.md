---
breadcrumbs:
- - /updates
  - updates
page_name: ua-reduction
title: User-Agent Reduction
---

[TOC]

## Updates

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

## Resources

The following sites show snippets and allow developers to preview what the
reduced User-Agent string will look like on different platforms:
- [User-Agent Reduction Snippets](https://developer.chrome.com/docs/privacy-sandbox/user-agent/snippets/)
- [User-Agent Reduction Interactive Demo](https://reduced-ua.glitch.me/)
