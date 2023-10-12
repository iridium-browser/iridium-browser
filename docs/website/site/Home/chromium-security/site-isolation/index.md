---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: site-isolation
title: Site Isolation
---

## Overview

Site Isolation is a security feature in Chrome that offers additional protection
against some types of security bugs. It uses Chrome's sandbox to make it harder
for untrustworthy websites to access or steal information from your accounts on
other websites.

Websites are typically not allowed to access each other's data inside the
browser, thanks to code that enforces the Same Origin Policy. Occasionally,
security bugs are found in this code and malicious websites may try to bypass
these rules to attack other websites. The Chrome team aims to fix such bugs as
quickly as possible.

Site Isolation offers an extra line of defense to make such attacks less likely
to succeed. It ensures that pages from different websites are always put into
different processes, each running in a sandbox that limits what the process is
allowed to do. It also makes it possible to block the process from receiving
most types of sensitive data from other sites. As a result, a malicious website
will find it much more difficult to steal data from other sites, even if it can
break some of the rules in its own process.

This protection is made possible by the following changes in Chrome's behavior:

* Cross-site documents are always put into a different process,
  whether the navigation is in the current tab, a new tab, or an
  iframe (i.e., one web page embedded inside another). Note that only
  a subset of sites are isolated on Android, to reduce overhead.
* Cross-site data (such as HTML, XML, JSON, and PDF files) is not
  delivered to a web page's process unless the server says it should
  be allowed (using [CORS](https://www.w3.org/TR/cors/)).
* Security checks in the browser process can detect and terminate a
  misbehaving renderer process (only on desktop platforms for the time
  being).

Here, we use a precise definition for a **site**: the scheme and registered
domain name, including the public suffix, but ignoring subdomains, port, or
path. For example, an origin like https://foo.example.com:8080 would have a site
of https://example.com. We use sites instead of origins to avoid breaking
compatibility with existing web pages that might modify their document.domain to
communicate across multiple subdomains of a site.

For more technical information about the protections offered by Site Isolation
and how they are built, please see the [project's design
document](/developers/design-documents/site-isolation).

[TOC]

## Motivation

Web browser security is important: browsers must defend against untrustworthy
web pages that try to attack other sites or access the user's machine. Given the
complexity of the browser, it is necessary to use a "defense in depth" approach
to limit the damage that occurs even if an attacker finds a way around the Same
Origin Policy or other security logic in the browser. As a result, Chrome uses a
sandbox and Site Isolation to try to defend against even powerful attackers
(e.g., who might know about bugs in the browser). This is motivated by several
different types of attacks.

First, compromised renderer processes (also known as "arbitrary code execution"
attacks in the renderer process) need to be explicitly included in a browser’s
security threat model. We assume that determined attackers will be able to find
a way to compromise a renderer process, for several reasons:

* Past experience suggests that potentially exploitable bugs will be
  present in future Chrome releases. There were [10 potentially
  exploitable bugs in renderer components in
  M69](https://bugs.chromium.org/p/chromium/issues/list?can=1&q=Release%3D0-M69%2C1-M69%2C2-M69%2C3-M69+Type%3DBug-Security+Security_Severity%3DHigh%2CCritical+-status%3ADuplicate+label%3Aallpublic+component%3ABlink%2CInternals%3ECompositing%2CInternals%3EImages%3ECodecs%2CInternals%3EMedia%2CInternals%3ESkia%2CInternals%3EWebRTC%2C+-component%3ABlink%3EMedia%3EPictureInPicture%2CBlink%3EPayments%2CBlink%3EStorage%2CInternals%3ECore%2CInternals%3EPrinting%2CInternals%3EStorage%2CMojo%2CServices%3ESync%2CUI%3EBrowser&sort=m&groupby=&colspec=ID+Status+CVE+Security_Severity+Security_Impact+Component+Summary),
  [5 in
  M70](https://bugs.chromium.org/p/chromium/issues/list?sort=m&groupby=&colspec=ID%20Status%20CVE%20Security_Severity%20Security_Impact%20Component%20Summary&q=Release%3D0-M70%2C1-M70%2C2-M70%2C3-M70%20Type%3DBug-Security%20Security_Severity%3DHigh%2CCritical%20-status%3ADuplicate%20label%3Aallpublic%20component%3ABlink%2CInternals%3ECompositing%2CInternals%3EImages%3ECodecs%2CInternals%3EMedia%2CInternals%3ESkia%2CInternals%3EWebRTC%2C%20-component%3ABlink%3EMedia%3EPictureInPicture%2CBlink%3EPayments%2CBlink%3EStorage%2CInternals%3ECore%2CInternals%3EPrinting%2CInternals%3EStorage%2CMojo%2CServices%3ESync%2CUI%3EBrowser&can=1),
  [13 in
  M71](https://bugs.chromium.org/p/chromium/issues/list?sort=m&groupby=&colspec=ID%20Status%20CVE%20Security_Severity%20Security_Impact%20Component%20Summary&q=Release%3D0-M71%2C1-M71%2C2-M71%2C3-M71%20Type%3DBug-Security%20Security_Severity%3DHigh%2CCritical%20-status%3ADuplicate%20label%3Aallpublic%20component%3ABlink%2CInternals%3ECompositing%2CInternals%3EImages%3ECodecs%2CInternals%3EMedia%2CInternals%3ESkia%2CInternals%3EWebRTC%2C%20-component%3ABlink%3EMedia%3EPictureInPicture%2CBlink%3EPayments%2CBlink%3EStorage%2CInternals%3ECore%2CInternals%3EPrinting%2CInternals%3EStorage%2CMojo%2CServices%3ESync%2CUI%3EBrowser&can=1),
  [13 in
  M72](https://bugs.chromium.org/p/chromium/issues/list?sort=m&groupby=&colspec=ID%20Status%20CVE%20Security_Severity%20Security_Impact%20Component%20Summary&q=Release%3D0-M72%2C1-M72%2C2-M72%2C3-M72%20Type%3DBug-Security%20Security_Severity%3DHigh%2CCritical%20-status%3ADuplicate%20label%3Aallpublic%20component%3ABlink%2CInternals%3ECompositing%2CInternals%3EImages%3ECodecs%2CInternals%3EMedia%2CInternals%3ESkia%2CInternals%3EWebRTC%2C%20-component%3ABlink%3EMedia%3EPictureInPicture%2CBlink%3EPayments%2CBlink%3EStorage%2CInternals%3ECore%2CInternals%3EPrinting%2CInternals%3EStorage%2CMojo%2CServices%3ESync%2CUI%3EBrowser&can=1),
  [15 in
  M73](https://bugs.chromium.org/p/chromium/issues/list?sort=m&groupby=&colspec=ID%20Status%20CVE%20Security_Severity%20Security_Impact%20Component%20Summary&q=Release%3D0-M73%2C1-M73%2C2-M73%2C3-M73%20Type%3DBug-Security%20Security_Severity%3DHigh%2CCritical%20-status%3ADuplicate%20label%3Aallpublic%20component%3ABlink%2CInternals%3ECompositing%2CInternals%3EImages%3ECodecs%2CInternals%3EMedia%2CInternals%3ESkia%2CInternals%3EWebRTC%2C%20-component%3ABlink%3EMedia%3EPictureInPicture%2CBlink%3EPayments%2CBlink%3EStorage%2CInternals%3ECore%2CInternals%3EPrinting%2CInternals%3EStorage%2CMojo%2CServices%3ESync%2CUI%3EBrowser&can=1).
  This volume of bugs holds steady despite years of investment into
  developer education, fuzzing, Vulnerability Reward Programs, etc.
  Note that this only includes bugs that are reported to us or are
  found by our team.
* Security bugs can often be made exploitable: even 1-byte buffer
  overruns [can be turned into an
  exploit](https://googleprojectzero.blogspot.com/2014/08/the-poisoned-nul-byte-2014-edition.html).
* Deployed mitigations (like
  [ASLR](http://en.wikipedia.org/wiki/Address_space_layout_randomization)
  or [DEP](http://en.wikipedia.org/wiki/Data_Execution_Prevention))
  are [not always
  effective](https://googleprojectzero.blogspot.com/2019/04/virtually-unlimited-memory-escaping.html).

Second, universal cross-site scripting (UXSS) bugs pose a similar threat.
Security bugs of this form would normally let an attacker bypass the Same Origin
Policy within the renderer process, though they don't give the attacker complete
control over the process. Such UXSS bugs tend to be
[common](https://ai.google/research/pubs/pub48028).

Third, side channel attacks such as [Spectre](https://spectreattack.com/) make
reading arbitrary renderer process memory possible, even without bugs in Chrome.
This poses additional risks to sensitive data in the renderer process, and it
can make exploitation easier.

Chrome's architecture provides additional defenses against these powerful
attacks. Chrome's
[sandbox](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/design/sandbox.md)
helps prevent a compromised renderer process from being able to access arbitrary
local resources (e.g. files, devices). Site Isolation helps protect websites
against attacks from compromised renderer processes, UXSS, and side-channel
attacks like Spectre.

For more background and motivation, see our [Usenix Security 2019 conference
paper](https://www.usenix.org/conference/usenixsecurity19/presentation/reis) on
Site Isolation, and Chrome's [Post-Spectre Threat Model
Re-Think](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/security/side-channel-threat-model.md).

## Current Status

#### Desktop Platforms

Site Isolation was [enabled by
default](https://security.googleblog.com/2018/07/mitigating-spectre-with-site-isolation.html)
for all sites in Chrome 67 on Windows, Mac, Linux, and Chrome OS to help to
defend against attacks that are able to read otherwise inaccessible data within
a process, such as [speculative side-channel attack
techniques](https://security.googleblog.com/2018/01/todays-cpu-vulnerability-what-you-need.html)
like Spectre/Meltdown. As of Chrome 77, Site Isolation also now defends against
fully compromised renderer processes and UXSS bugs on desktop platforms. In M92,
Site Isolation expanded to isolate all extensions from each other as well.  In
M110, Site Isolation support was added for [&lt;webview&gt;
tags](https://developer.chrome.com/docs/extensions/reference/webviewTag/).

#### Android

On Android devices with at least 2 GB of RAM, Site Isolation has been enabled
for sites that users log into since Chrome 77. In Chrome 92, this expanded to
include sites that use third-party login providers (e.g., OAuth) and sites that
adopt Cross-Origin-Opener-Policy headers.

## Related Blog Posts

* [Improving extension security with out-of-process
  iframes](https://blog.chromium.org/2017/05/improving-extension-security-with-out.html)
  \- May 2017
* [Mitigating Spectre with Site Isolation in
  Chrome](https://security.googleblog.com/2018/07/mitigating-spectre-with-site-isolation.html)
  \- July 2018
* [Improving Site Isolation for Stronger Browser
  Security](https://security.googleblog.com/2019/10/improving-site-isolation-for-stronger.html)
  / [Recent Site Isolation
  improvements](https://blog.chromium.org/2019/10/recent-site-isolation-improvements.html)
  \- October 2019
* [Mitigating Side-Channel
  Attacks](https://blog.chromium.org/2021/03/mitigating-side-channel-attacks.html)
  / [A Spectre proof-of-concept for a Spectre-proof
  web](https://security.googleblog.com/2021/03/a-spectre-proof-of-concept-for-spectre.html)
  \- March 2021
* [Protecting more with Site
  Isolation](https://security.googleblog.com/2021/07/protecting-more-with-site-isolation.html)
  / [Privacy and performance, working together in
  Chrome](https://blog.google/products/chrome/privacy-and-performance-working-together-chrome/)
  \- July 2021

## Limitations

* **Sites vs Origins**: Compatibility with document.domain changes
  currently requires us to use sites (e.g., https://example.com)
  rather than origins (e.g., https://foo.example.com) for defining
  process boundaries. This allows multiple origins within a site to
  share the same process.
* **Filtering cross-site data**: Cross-Origin Read Blocking (CORB) is
  a best effort approach that tries to protect as much sensitive
  content as possible, but it is limited by the need to preserve
  compatibility with incorrectly labeled resources (e.g., JavaScript
  files labeled as HTML).
* **Unavailable in some settings**: Site Isolation is not yet
  supported in Android WebView, or on Chrome for Android devices with
  less than 2GB of RAM.

## Tradeoffs

Site Isolation represents a major architecture change for Chrome, so there are
some tradeoffs when enabling it, such as increased memory overhead. The team has
worked hard to minimize this overhead and fix as many functional issues as
possible. A few known issues remain:

For users:

* Higher overall memory use in Chrome. On desktop in Chrome 67, this
  is about 10-13% when isolating all sites with many tabs open. On
  Android in Chrome 77, this is about 3-5% overhead when isolating
  sites that users log into.

For web developers:

* Full-page layout is no longer synchronous, since the frames of a
  page may be spread across multiple processes. This may affect pages
  that change the size of a frame and then send a postMessage to it,
  since the receiving frame may not yet know its new size when
  receiving the message. One workaround is to send the new size in the
  postMessage itself if the receiving frame needs it. As of Chrome 68,
  pages can also work around this by forcing a layout in the sending
  frame before sending the postMessage. See [Site Isolation for web
  developers](https://developers.google.com/web/updates/2018/07/site-isolation)
  for more details.
* Unload handlers may not always run when the tab is closed.
  postMessage might not work from an unload handler
  ([964950](https://crbug.com/964950)).
* When debugging with `--disable-web-security`, it may also be necessary
  to disable Site Isolation (using
  `--disable-features=IsolateOrigins,site-per-process`) to access
  cross-origin frames.

## How to Configure

For most users, no action is required, and Site Isolation is enabled at an
appropriate level based on available resources.

For more advanced cases, it is possible to isolate additional sites and origins
in a few ways. Note that changes to chrome://flags and the command line only
affect the current device, and are not synced to your other instances of Chrome.

### 1) Isolating All Sites (Android)

This mode is already enabled by default for 100% of Chrome users on Windows,
Mac, Linux, and Chrome OS. The instructions below can still be useful on
Android, for users desiring the highest security on devices with sufficient RAM.

This mode ensures that all sites are put into dedicated processes that are not
shared with other sites. It can be enabled in either of the following ways:

* Visit chrome://flags#enable-site-per-process, click Enable, and
  restart. (See also: [help center
  article](https://support.google.com/chrome/answer/7623121). Note
  that this flag is only present on Android. It is missing on other
  platforms, where it is already enabled by default.)

  ![](/Home/chromium-security/site-isolation/site-isolation-flag-1.png)

* Or, use an [Enterprise
  Policy](https://support.google.com/chrome/a/answer/7581529) to
  enable
  [SitePerProcess](/administrators/policy-list-3#SitePerProcess) or
  [SitePerProcessAndroid](/administrators/policy-list-3#SitePerProcessAndroid)
  within your organization.

### 2) Isolating Specific Origins

This mode allows you to provide a list of specific origins that will be given
dedicated processes. On desktop platforms where Site Isolation is already fully
enabled, these origins will be isolated at a finer granularity than their site
(e.g., https://foo.example.com can be isolated from the rest of
https://example.com). On Android, these origins will be isolated in addition to
any other sites already isolated. This can be used to isolate any origins that
need extra protection, such as any that you log into. (Note that wildcards are
supported to isolate all origins underneath a given origin.)

This mode is automatically enabled on Android devices with at least 2 GB of
memory as of Chrome 77, for sites that users log into. This mode can be further
manually configured in any of the following ways:

* In Chrome 77 or later versions: Enable
  chrome://flags/#isolate-origins, provide the list of origins to
  isolate (e.g.
  "https://foo.example.com,https://[*.]corp.example.com"), and
  restart Chrome.

* Use [command line flags](/for-testers/command-line-flags) to start
  Chrome with `--isolate-origins` followed by a comma-separated list of
  origins to isolate. For example:
  `--isolate-origins=https://foo.example.com,https://[*.]corp.example.com`
  Be careful not to include effective top-level domains (e.g., https://co.uk
  or https://appspot.com; see the full list at <https://publicsuffix.org>),
  because these will be ignored.
* Or, use an [Enterprise
  Policy](https://support.google.com/chrome/a/answer/7581529) to
  enable
  [IsolateOrigins](https://chromeenterprise.google/policies/#IsolateOrigins)
  or
  [IsolateOriginsAndroid](https://chromeenterprise.google/policies/#IsolateOriginsAndroid)
  within your organization.

### **3) Isolating All Origins**

This mode is experimental and will break any pages that depend on modifying
document.domain to access a cross-origin but same-site page. It will also
increase Chrome's process count and may affect performance. The benefit is that
every origin will require its own dedicated process, such that two origins from
the same site won't share a process.

This mode can be configured by enabling chrome://flags#strict-origin-isolation

### Diagnosing Issues

If you encounter problems when Site Isolation is enabled, you can try turning it
off by undoing the steps above, to see if the problem goes away.

You can also try opting out of field trials of Site Isolation to diagnose bugs,
by visiting chrome://flags#site-isolation-trial-opt-out, choosing "Disabled (not
recommended)," and restarting.

![](/Home/chromium-security/site-isolation/site-isolation-flag-2.png)

Starting Chrome with the `--disable-site-isolation-trials` flag is equivalent to
the opt-out above.

Note that if Site Isolation has been enabled by enterprise policy, then none of
these options can be used to disable it.

We encourage you to [file bugs](https://goo.gl/XBoKtY) if you encounter problems
when using Site Isolation that go away when disabling it. In the bug report,
please describe the problem and mention whether it is specific to having Site
Isolation enabled.

### Verifying

You can visit chrome://process-internals to see whether a Site Isolation mode is
enabled.

If you would like to test that Site Isolation has been successfully turned on in
practice, you can follow the steps below:

1. Navigate to a website that has cross-site subframes. For example:
   * Navigate to
     <http://csreis.github.io/tests/cross-site-iframe.html>.
   * Click the "Go cross-site (complex page)" button.
   * The main page will now be on the http://csreis.github.io site
     and the subframe will be on the https://chromium.org site.
2. Open Chrome's Task Manager: Chrome Menu -&gt; More tools -&gt; Task
   manager (Shift+Esc).
3. Verify that the main page and the subframe are listed in separate
   rows associated with different processes. For example:
   * Tab: creis.github.io/tests/cross-site-iframe.html - Process ID = 1234
   * Subframe: https://chromium.org - Process ID = 5678

If you see the subframe process in Chrome's Task Manager, then Site Isolation is
correctly enabled. These steps work when using the "Isolating all sites"
approach above (e.g., --site-per-process). They also work when using the
"Isolating certain sites" approach above (e.g., --isolate-origins), as long as
the list of origins provided includes either http://csreis.github.io or
https://chromium.org.

## Recommendations for Web Developers

Site Isolation can help protect sensitive data on your website, but only if
Chrome can distinguish it from other resources which any site is allowed to
request (e.g., images, scripts, etc.). Chrome currently tries to identify URLs
that contain HTML, XML, JSON, and PDF files, based on MIME type and other HTTP
headers. See [Cross-Origin Read Blocking for Web
Developers](/Home/chromium-security/corb-for-developers) for information on how
to ensure that sensitive information on your website will be protected by Site
Isolation.

**We strongly recommend following the guidelines in [Post-Spectre Web
Development](https://www.w3.org/TR/post-spectre-webdev/) to protect content**,
which can help in browsers with and without Site Isolation support. The
[Mitigating Side-Channel
Attacks](https://blog.chromium.org/2021/03/mitigating-side-channel-attacks.html)
blog post provides a good overview of these mechanisms and how they help. For
example, using HTTP response headers such as
[Cross-Origin-Resource-Policy](https://developer.mozilla.org/en-US/docs/Web/HTTP/Cross-Origin_Resource_Policy_(CORP))
and
[Cross-Origin-Opener-Policy](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cross-Origin-Opener-Policy)
can help control which process a resource can load in. Consider also inspecting
the [Sec-Fetch-](https://w3c.github.io/webappsec-fetch-metadata/) request
headers in the HTTP server to identify the source of the request before deciding
how to handle a request.

See also [Site Isolation for web
developers](https://developers.google.com/web/updates/2018/07/site-isolation)
for more discussion of how Site Isolation can protect web page content and in
which cases it might affect page behavior.
