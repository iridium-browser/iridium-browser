---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: site-isolation
title: Site Isolation Design Document
---

*This design document covers technical information about how Site Isolation is
built. For a general overview of Site Isolation, see
<https://www.chromium.org/Home/chromium-security/site-isolation>.*

## Motivation

Chrome's [multi-process
architecture](/developers/design-documents/multi-process-architecture) provides
many benefits for speed, stability, and security. It allows web pages in
unrelated tabs to run in parallel, and it allows users to continue using the
browser and other tabs when a renderer process crashes. Because the renderer
processes don't require direct access to disk, network, or devices, Chrome can
also run them inside a restricted
[sandbox](/developers/design-documents/sandbox). This [limits the
damage](http://crypto.stanford.edu/websec/chromium/) that attackers can cause if
they exploit a vulnerability in the renderer, including making it difficult for
attackers to access the user's filesystem or devices, as well as privileged
pages (e.g., settings or extensions) and pages in other profiles (e.g.,
Incognito mode).

[<img alt="image"
src="/developers/design-documents/site-isolation/ChromeSiteIsolationProject-arch.png"
height=137
width=320>](/developers/design-documents/site-isolation/ChromeSiteIsolationProject-arch.png)

However, for a long time there was a large opportunity to use Chrome's sandbox
for greater security benefits: isolating web sites from each other. Until
version 67, Chrome made an effort to place pages from different web sites in
different renderer processes when possible, but due to compatibility
constraints, there were many cases in which pages from different sites share a
process (e.g., cross-site iframes). In these cases, we used to rely on the
renderer process to enforce the Same Origin Policy and keep web sites isolated
from each other.

This page describes our "site isolation" efforts to improve Chrome to use
sandboxed renderer processes as a security boundary between web sites, even in
the presence of vulnerabilities in the renderer process. Our goal is to ensure a
renderer process can be limited to contain pages from at most one web site. The
browser process can then restrict that process's access to cookies and other
resources, based on which web sites require dedicated processes.

**Status:** Site Isolation has been enabled by default on desktop platforms (for
all sites) in Chrome 67, and on Android (for sites users log into) in Chrome 77.
This helps defend against speculative side channel attacks (e.g., Spectre), and
(on desktop) against UXSS and fully compromised renderer processes.

[TOC]

## **Threat Model**

For a "one-site-per-process" security policy, we assume that an attacker can
convince the user to visit a page that exploits a vulnerability in the renderer
process, allowing the attacker to run arbitrary code within the sandbox. We also
assume that an attacker may use [speculative side channel
attacks](https://security.googleblog.com/2018/01/todays-cpu-vulnerability-what-you-need.html)
(e.g., Spectre) to read data within a renderer process. We focus on attackers
that want to steal information or abuse privileges granted to other web sites.

Here, we use a precise definition for a **site** that we use as a principal: a
page's site includes the scheme and registered domain name, including the public
suffix, but ignoring subdomains, port, or path. We use sites instead of origins
to avoid breaking compatibility with existing web pages that might modify their
document.domain to communicate across subdomains.

We consider the following threats in scope for the proposed policy (once fully
implemented):

*   **Stealing cross-site cookies and HTML5 stored data.** We can
            prevent a renderer process from receiving cookies or stored data
            from sites other than its own.
*   **Stealing cross-site HTML, XML, and JSON data.** Using MIME type
            and content sniffing, we can prevent a renderer process from loading
            most sensitive cross-site data. We cannot block all cross-site
            resources, however, because images, scripts, and other opaque files
            are permitted across sites.
*   **Stealing saved passwords.** We can prevent a renderer process from
            receiving saved passwords from sites other than its own.
*   **Abusing permissions granted to another site.** We can prevent a
            renderer process from using permissions such as geolocation that the
            user has granted to other sites.
*   **Compromising X-Frame-Options.** We can prevent a renderer process
            from loading cross-site pages in iframes. This allows the browser
            process to decide if a given site can be loaded in an iframe or not
            based on X-Frame-Options or CSP frame-ancestors headers.
*   **Accessing cross-site DOM elements via UXSS bugs.** An attacker
            exploiting a universal cross-site scripting bug in the renderer
            process will not be able to access DOM elements of cross-site pages,
            which will not live in the same renderer process.

We do not expect this policy to mitigate traditional cross-site attacks or
attacks that occur within the page of a victim site, such as XSS, CSRF, XSSI, or
clickjacking.

## Requirements

To support a site-per-process policy in a multi-process web browser, we need to
identify the smallest unit that cannot be split into multiple processes. This is
not actually a single page, but rather a group of documents from the same web
site that have references to each other. Such documents have full script access
to each other's content, and they must run on a single thread, not concurrently.
This group may span multiple frames or tabs, and they may come from multiple
sub-domains of the same site.

[<img alt="image"
src="/developers/design-documents/site-isolation/ChromeSiteIsolationProject-siteinstances.png"
height=150
width=400>](/developers/design-documents/site-isolation/ChromeSiteIsolationProject-siteinstances.png)

The HTML spec refers to this group as a "[unit of related similar-origin
browsing
contexts](http://dev.w3.org/html5/spec/single-page.html#unit-of-related-similar-origin-browsing-contexts)."
In Chrome, we refer to this as a **SiteInstance**. All of the documents within a
SiteInstance may be able to script each other, and we must thus render them in
the same process.

Note that a single tab might be navigated from one web site to another, and thus
it may show different SiteInstances at different times. To support a
site-per-process policy, a browser must be able to swap between renderer
processes for these navigations.

There are also certain JavaScript interactions, such as postMessage() or
close(), that are allowed between windows or frames even when they are showing
documents from different sites. It is necessary to support these limited
interactions between renderer processes.

In addition, top-level documents may contain iframes from different web sites.
These iframes have their own security context and must be rendered in a process
based on their own site, not the site of their parent frame.

Finally, it is important that sensitive cross-site data is not delivered to the
renderer process, even if requested from contexts like image or script tags
which normally work cross-site. This requires identifying which responses to
allow and which to block.

## Project Progression

As described on our [Process
Models](/developers/design-documents/process-models) page, there were originally
several cases in which Chrome (prior to version 67) would place documents from
different sites in the same renderer process. This kept Chrome compatible with
existing web pages with cross-site iframes, at least until Site Isolation was
enabled. Some examples of cross-site pages that shared a process at the time:

*   Cross-site iframes were usually hosted in the same process as their
            parent document.
*   Most renderer-initiated navigations (including link clicks, form
            submissions, and script navigations) were kept within the current
            process even if they cross a site boundary. This is because other
            windows in the same process could attempt to use postMessage or
            similar calls to interact with them.
*   If too many renderer processes had been created, Chrome started to
            reuse existing processes rather than creating new ones. This reduces
            memory overhead.

However, at the time, Chrome already had taken many large steps towards site
isolation. For example, it swapped renderer processes for cross-site navigations
that were initiated in the browser process (such as omnibox navigations or
bookmarks). Cross-origin JavaScript interactions were supported across
processes, such as postMessage() and navigating another window. Chrome
eventually gained support for out-of-process iframes in some contexts.

That early progress allowed us to use a stricter security policy for certain
types of pages, such as privileged WebUI pages (like the Settings page). These
pages are never allowed to share a process with regular web pages, even when
navigating in a single tab. This is generally acceptable from a compatibility
perspective because no scripting is expected between normal pages and WebUI
pages, and because these can never be loaded in subframes of unprivileged pages.

As of Chrome 56, Chrome started using out-of-process iframes to [keep web
content out of privileged extension
processes](https://blog.chromium.org/2017/05/improving-extension-security-with-out.html).

As of Chrome 63, Site Isolation could be
[enabled](https://support.google.com/chrome/answer/7623121?hl=en) as an
[additional mitigation against universal cross-site scripting (UXSS)
vulnerabilities and Spectre](/Home/chromium-security/site-isolation). It was
also available via [enterprise
policy](https://www.blog.google/topics/connected-workspaces/security-enhancements-and-more-enterprise-chrome-browser-customers/).
That initial support was still in progress and had [known issues and
tradeoffs](https://support.google.com/chrome/a/answer/7581529), but helped to
defend against UXSS vulnerabilities and Spectre by putting pages from different
sites in different processes to prevent leaks of cross-site data. The feature
did not yet mitigate attacks with arbitrary remote code execution in the
renderer process; this will later be possible as additional enforcements in the
browser process are completed.

In Chrome 67, Site Isolation was enabled for all sites on desktop platforms
(Windows, Mac, Linux, and ChromeOS), helping to defend against Spectre.

In Chrome 77, Site Isolation was also enabled on Android devices with at least 2
GB of RAM, but only for sites that users log into (to keep memory overhead
lower). By this time, Site Isolation on desktop (i.e., with the full
site-per-process policy) also defended against fully compromised renderer
processes.

## Project Tasks

To support a site-per-process policy in Chrome, we needed to complete the tasks
outlined below. These ensure that cross-site navigations and script interactions
will not break, despite having all pages from different sites in different
processes. The master tracking bug is <https://crbug.com/467770>.

*   **Cross-Process Navigations**
    Any navigation to a different web site requires a process swap in the
    current tab or frame.
    *Status:* Complete. Cross-process navigations are supported in all frames,
    and they are used for many cases: out-of-process iframes, keeping privileged
    WebUI or extension pages isolated from web pages, any cross-site main frame
    navigation, etc.
*   **Cross-Process JavaScript**
    As mentioned above, some window and frame level interactions are allowed
    between pages from different sites. Common examples are postMessage, close,
    focus, blur, and assignments to window.location, notably excluding any
    access to page content. These interactions can generally be made
    asynchronous and can be implemented by passing messages to the appropriate
    renderer process.
    *Status:* Complete. Chrome supports all required interactions, including
    frame placeholders, postMessage, close, closed, focus, blur, and assignments
    to window.location between top-level windows in different processes.
*   **Out-of-Process iframes**
    Iframes have separate security contexts from their parent document, so
    cross-site iframes must be rendered in a different process from their
    parent. It is also important that an iframe that is from the same origin as
    a popup window shares a process with the popup window and not its own parent
    page. We render these out-of-process iframes in a separate RenderFrame
    composited into the correct visual location, much like plugins. This is by
    far the largest requirement for supporting site-per-process, as it involves
    a major architecture change to the Chrome and Blink codebases.
    *Status:* Complete. The first uses of [Out-of-Process
    iframes](/developers/design-documents/oop-iframes) (OOPIFs) launched in
    Chrome 56, isolating extensions from web content. Widespread use for
    cross-site iframes launched in Chrome 67 on desktop, and in Chrome 77 for
    Android. Tracked at <https://crbug.com/99379>.
*   **Cross-Origin Read Blocking**
    While any given site is allowed to request many types of cross-site
    resources (such as scripts and images), the browser process should prevent
    it from receiving cross-site HTML, XML, and JSON data (based on a
    combination of MIME type and content sniffing).
    *Status:* Complete. Our initial [Cross-Site Document Blocking
    Policy](/developers/design-documents/blocking-cross-site-documents) design
    evolved into [Cross-Origin Read Blocking
    (CORB)](/Home/chromium-security/corb-for-developers), with a [CORB
    Explainer](https://chromium.googlesource.com/chromium/src/+/HEAD/services/network/cross_origin_read_blocking_explainer.md).
    This was implemented for non-compromised renderer processes when Site
    Isolation is enabled as of Chrome 63, and for compromised renderers as of
    Chrome 77. The work was tracked at <https://crbug.com/268640>.
*   **Browser Process Enforcements**
    Some of Chrome's security checks are performed in the renderer process. When
    a process is locked to a given site, the browser process can enforce many of
    these checks itself, limiting what a compromised renderer process can
    achieve in an attack. This includes attempts to access site specific stored
    data and permissions, as well as other attempts to lie to the browser
    process.
    *Status:* Most enforcements are in place, though more are tracked in
    <https://crbug.com/786673>.
*   **Improved Renderer Process Limit Policy**
    We have investigated ways to limit number of extra processes Chrome creates
    in Site Isolation modes. One option is to support modes that only isolate a
    set of origins or sites (i.e., --isolate-origins) rather than all sites
    (i.e., --site-per-process). However, we are currently leaning towards
    isolating all sites given our current findings. Note that a page from one
    site may reuse a process that has already been used for that same site, and
    we aggressively reuse processes in this way for subframes when possible.
    Processes will not be reused for cross-site pages.
    *Status:* On desktop, we isolate all sites, allowing process reuse only for
    pages from the same site. On Android, only a subset of sites end up in
    dedicated processes.

## Performance

Monitoring the performance impact of Site Isolation on Chrome has been a
critical part of this effort. Site Isolation can affect performance in several
ways, both positive and negative: some frames may render faster by running in
parallel with the rest of the page, but creating additional renderer processes
also increases memory requirements and may introduce latency on cross-process
navigations. We are taking several steps to minimize the costs incurred, and we
are using metrics collected from actual browsing data to measure the impact.

As mentioned above under "Renderer Process Limit Policy," we investigated one
option for reducing the number of processes by isolating only a subset of web
sites. In such a mode, most web sites would continue to use Chrome's old process
model, while web sites that users are likely to log into would be isolated.
However, we are currently aiming to isolate all sites if possible, based on
current performance measurements, simplicity, and the extra protection it
offers.

Our evaluation is described in our USENIX Security 2019 paper: [Site Isolation:
Process Separation for Web Sites within the
Browser](https://www.usenix.org/conference/usenixsecurity19/presentation/reis).

## How to Enable

See this [Site Isolation Overview page](/Home/chromium-security/site-isolation)
for more details on how to enable or configure Site Isolation. In most cases, no
action is necessary.

## **Development Resources**

Updating Chrome Features:

*   [Feature Update FAQ for Out-of-Process
            iframes](https://docs.google.com/document/d/1Iqe_CzFVA6hyxe7h2bUKusxsjB6frXfdAYLerM3JjPo/edit?usp=sharing)
*   [Features to Update for Out-of-Process
            iframes](https://docs.google.com/document/d/1dCR2aEoBJj_Yqcs6GuM7lUPr0gag77L5OSgDa8bebsI/edit?usp=sharing)

Build Status:

*   Site Isolation FYI bots:
            [Linux](http://build.chromium.org/p/chromium.fyi/builders/Site%20Isolation%20Linux)
            and
            [Windows](http://build.chromium.org/p/chromium.fyi/builders/Site%20Isolation%20Win)
*   Site Isolation try bot:
            [linux_site_isolation](http://build.chromium.org/p/tryserver.chromium.linux/builders/linux_site_isolation)

## 2015 Site Isolation Summit Talks

Talks and discussion from January 2015.

### Site Isolation Overview

[Slides](https://docs.google.com/presentation/d/10HTTK4dsxO5p6FcpEOq8EkuV4yiBx2n6dBki8cqDWyo/edit?usp=sharing)

#### Site Isolation Overview

### Chromium Changes for OOPIF

[Slides](https://docs.google.com/presentation/d/1e25K7BW3etNDm1-lkMltMcCcRDeVwLibBcbACRhqZ1k/edit?usp=sharing)

#### Chromium Changes for OOPIF

### Blink Changes for OOPIF

[Slides](https://docs.google.com/presentation/d/11nrXiuXBTC72E5l_MUtu2eJN6rcW9PtBewDOPPTk9Bc/edit?usp=sharing)

#### Blink Changes for OOPIF

## Discussions/Questions

The mailing list for technical discussions on Site Isolation is
[site-isolation-dev@chromium.org](mailto:site-isolation-dev@chromium.org).
