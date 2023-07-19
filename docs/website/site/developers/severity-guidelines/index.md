---
breadcrumbs:
- - /developers
  - For Developers
page_name: severity-guidelines
title: Severity Guidelines for Security Issues
---

### **Do not edit — for historical reference only**

The canonical version of this page is now in the [Chromium source tree](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/severity-guidelines.md).

### ---

### Vendors shipping products based on Chromium might wish to rate the severity of security issues in the products they release. This document contains guidelines for how to rate these issues. Check out our [security release management page](/Home/chromium-security/security-release-management) for guidance on how to release fixes based on severity.

### Any significant mitigating factors, such as unusual or additional user interaction, or running Chrome with a specific command line flag or non-default feature enabled, may reduce an issue’s severity by one or more levels. Also note that most crashes do not indicate vulnerabilities. Chromium is designed to crash in a controlled manner (e.g., with a __debugBreak) when memory is exhausted or in other exceptional circumstances.

### Critical severity

Critical severity issues allow an attacker run arbitrary code on the underlying
platform with the user's privileges in the normal course of browsing.

They are normally assigned priority Pri-0 and assigned to the current stable
milestone (or earliest milestone affected). For critical severity bugs,
[SheriffBot](/issue-tracking/autotriage) will automatically assign the
milestone.

For critical vulnerabilities, we aim to deploy the patch to all Chrome users in
under 30 days. Critical vulnerability details may be made public in 60 days, in
accordance with Google's general [vulnerability disclosure
recommendations](https://security.googleblog.com/2010/07/rebooting-responsible-disclosure-focus.html),
or [faster (7 days) if there is evidence of active
exploitation](https://security.googleblog.com/2013/05/disclosure-timeline-for-vulnerabilities.html).

Example bugs:

    Memory corruption in the browser process controllable by a malicious web
    site
    ([564501](https://bugs.chromium.org/p/chromium/issues/detail?id=564501)).

    Exploit chains made up of multiple bugs that can lead to code execution
    outside of the sandbox
    ([416449](https://bugs.chromium.org/p/chromium/issues/detail?id=416449)).
    Note that the individual bugs that make up the chain will have lower
    severity ratings.

### High severity

High severity vulnerabilities allow an attacker to execute code in the context
of, or otherwise impersonate other origins. Bugs which would normally be
critical severity with unusual mitigating factors may be rated as high severity.
For example, renderer sandbox escapes fall into this category as their impact is
that of a critical severity bug, but they require the precondition of a
compromised renderer.

They are normally assigned priority Pri-1 and assigned to the current stable
milestone (or earliest milestone affected). For high severity bugs,
[SheriffBot](/issue-tracking/autotriage) will automatically assign the
milestone.

For high severity vulnerabilities, we aim to deploy the patch to all Chrome
users in under 60 days.

Example bugs:

    A bug that allows full circumvention of the same origin policy. Universal
    XSS bugs fall into this category, as they allow script execution in the
    context of an arbitrary origin
    ([534923](https://bugs.chromium.org/p/chromium/issues/detail?id=534923)).

    A bug that allows arbitrary code execution within the confines of the
    sandbox, such as renderer or GPU process memory corruption
    ([570427](https://bugs.chromium.org/p/chromium/issues/detail?id=570427),
    [468936](https://bugs.chromium.org/p/chromium/issues/detail?id=468936)).

    Complete control over the apparent origin in the omnibox
    ([76666](https://bugs.chromium.org/p/chromium/issues/detail?id=76666)).

    Memory corruption in the browser process that can only be triggered from a
    compromised renderer, leading to a sandbox escape
    ([469152](https://bugs.chromium.org/p/chromium/issues/detail?id=469152)).

    Kernel memory corruption that could be used as a sandbox escape from a
    compromised renderer
    ([377392](https://bugs.chromium.org/p/chromium/issues/detail?id=377392)).

    Memory corruption in the browser process that requires specific user
    interaction, such as granting a permission
    ([455735](https://bugs.chromium.org/p/chromium/issues/detail?id=455735)).

### Medium severity

Medium severity bugs allow attackers to read or modify limited amounts of
information, or which are not harmful on their own but potentially harmful when
combined with other bugs. This includes information leaks that could be useful
in potential memory corruption exploits, or exposure of sensitive user
information that an attacker can exfiltrate. Bugs that would normally rated at a
higher severity level with unusual mitigating factors may be rated as medium
severity.

They are normally assigned priority Pri-1 and assigned to the current stable
milestone (or earliest milestone affected). If the fix seems too complicated to
merge to the current stable milestone, assign it to the next stable milestone.

Example bugs:

    An out-of-bounds read in a renderer process
    ([281480](https://bugs.chromium.org/p/chromium/issues/detail?id=281480)).

    An uninitialized memory read in the browser process where the values are
    passed to a compromised renderer via IPC
    ([469151](https://bugs.chromium.org/p/chromium/issues/detail?id=469151)).

    Memory corruption that requires a specific extension to be installed
    ([313743](https://bugs.chromium.org/p/chromium/issues/detail?id=313743)).

    An HSTS bypass
    ([461481](https://bugs.chromium.org/p/chromium/issues/detail?id=461481)).

    A bypass of the same origin policy for pages that meet several preconditions
    ([419383](https://bugs.chromium.org/p/chromium/issues/detail?id=419383)).

    A bug that allows web content to tamper with trusted browser UI
    ([550047](https://bugs.chromium.org/p/chromium/issues/detail?id=550047)).

    A bug that reduces the effectiveness of the sandbox
    ([338538](https://bugs.chromium.org/p/chromium/issues/detail?id=338538)).

    A bug that allows arbitrary pages to bypass security interstitials
    ([540949](https://bugs.chromium.org/p/chromium/issues/detail?id=540949)).

    A bug that allows an a

    ttacker to reliably read or infer browsing history
    ([381808](https://bugs.chromium.org/p/chromium/issues/detail?id=381808)).

    An address bar spoof where only certain URLs can be displayed, or with other
    mitigating factors
    ([265221](https://bugs.chromium.org/p/chromium/issues/detail?id=265221)).

    Memory corruption in a renderer process that requires specific user
    interaction, such as dragging an object
    ([303772](https://bugs.chromium.org/p/chromium/issues/detail?id=303772)).

### Low severity

Low severity vulnerabilities are usually bugs that would normally be a higher
severity, but which have extreme mitigating factors or highly limited scope.

They are normally assigned priority Pri-2. Milestones can be assigned to low
severity bugs on a case-by-case basis, but they are not normally merged to
stable or beta branches.

Example bugs:

    Bypass requirement for a user gesture
    ([256057](https://bugs.chromium.org/p/chromium/issues/detail?id=256057)).

    Partial CSP bypass
    ([534570](https://bugs.chromium.org/p/chromium/issues/detail?id=534570)).

    A limited extension permission bypass
    ([169632](https://bugs.chromium.org/p/chromium/issues/detail?id=169632)).

    An uncontrolled single-byte out-of-bounds read
    ([128163](https://bugs.chromium.org/p/chromium/issues/detail?id=128163)).

The [security FAQ](/Home/chromium-security/security-faq) covers many of the
cases that we do not consider to be security bugs, such as [denial of
service](/Home/chromium-security/security-faq).
