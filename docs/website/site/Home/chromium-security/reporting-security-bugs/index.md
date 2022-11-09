---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: reporting-security-bugs
title: Reporting Security Bugs
---

The Chromium project takes security very seriously, but any complex software
project is going to have some vulnerabilities. You can help us make Chromium
more secure by following the guidelines below when filing security bugs against
Chromium. And as an added benefit to you, following these guidelines will
increase both the chance and size of the reward you could receive under the
[Vulnerability Rewards Program](https://g.co/ChromeBugRewards).

[TOC]

### Scope

Do not report physically-local attacks. For example, please do not report that
you can compromise Chromium by installing a malicious DLL on a computer in a
place where Chromium will find it and load it. Similarly, please do not report
password disclosure using the Inspect Element feature to convert the
"\*\*\*\*\*\*\*" into the underlying text.

To understand why, see the [Chromium Security
FAQ.](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/faq.md)

### General Guidelines

These are the criteria that we expect from a good security bug report:

*   Was filed via the [security
            template](https://code.google.com/p/chromium/issues/entry?template=Security%20Bug).
*   Contains a clear and descriptive title.
*   Includes the Chromium/Chrome version number and [release
            channel](/getting-involved/dev-channel).
*   Lists the operating system, version, and service pack level of the
            testing platform.
*   Includes a reproducible example of the bug, such as an attached HTML
            or binary file that triggers the bug when loaded into Chrome.
    *   **Please** make the file as small as possible and remove any
                content not necessary to trigger the vulnerability.
    *   **Please** avoid dependencies on third-party libraries such as
                jQuery or Prototype (which can dramatically complicate the
                process of diagnosing a potential vulnerability).
    *   ***For short** (e.g. 15 lines), text-based reproductions (eg.
                HTML or SVG), please include the reproduction case directly in
                the report text.*
    *   **For larger** or more complex cases, please attach files
                directly to the bug, not in an archive.
    *   If you host a page that demonstrates the bug, **please also
                attach** the files and config needed reproduce locally.
    *   **Note** that a screen capture or video showing the issue is
                largely unnecessary except in the case for security UI issues,
                and is not a substitute for a well-written, reproducible test
                case.
*   Provides a brief description (where appropriate) of the nature of
            the bug along with any additional details required to reproduce it.
*   Avoids unnecessary commentary or hyperbole. (If you choose to
            provide a severity estimate please follow the criteria at [Severity
            Guidelines for Security Issues](/developers/severity-guidelines).)

### Reporting Crash Bugs

In addition to the general guidelines above, we look for the following
information on bugs that trigger browser crashes or sad tabs:

*   Whether the crash is in the browser (application crash) or the
            renderer (sad tab).
*   Paste into the bug description the exception details, register
            state, and relevant portion of the stack trace.
    *   General crash reporting information is available at [Reporting a
                Crash
                Bug](/for-testers/bug-reporting-guidelines/reporting-crash-bug).
    *   Platform specific debugger configuration (including instructions
                on setting up symbols) is available for
                [Windows](/developers/how-tos/debugging), [Mac OS
                X](/developers/debugging-on-os-x), and
                [Linux](https://code.google.com/p/chromium/wiki/LinuxDebugging).
*   If crash reporting is enabled, please provide your [client
            ID](/for-testers/bug-reporting-guidelines/reporting-crash-bug).
*   Please ensure that **all stack traces include symbols**.

#### Signs A Crash Is Not A Security Bug

Generally, we do not consider a denial of service (DoS) issues to be security
vulnerabilities. Examples of these bug classes include: consistent fixed-offset
[NULL pointer
dereferences](https://en.wikipedia.org/wiki/Pointer_(computing)#Null_pointer),
[call stack overflows (stack
exhaustion)](https://blogs.technet.com/b/srd/archive/2009/01/28/stack-overflow-stack-exhaustion-not-the-same-as-stack-buffer-overflow.aspx),
and [out of memory (OOM) errors](https://en.wikipedia.org/wiki/Out_of_memory).
Some of these crashes are valid bugs, and should be reported; however, they are
not security bugs and should be filed through the [normal defect
template](https://code.google.com/p/chromium/issues/entry).

### Going Above and Beyond

While we don't expect security vulnerability reporters to provide us any of the
following information, we certainly find it extremely helpful and would take it
into consideration when determining whether or not a particular report qualified
for a larger award:

*   An extremely clear and well-reduced test case demonstrating the
            vulnerability.
*   An accurate analysis of the exact nature of a crash (e.g.
            identifying the reason for and location of a premature pointer
            deletion in a use-after-free vulnerability).
*   An explanation of a broader vulnerability or recurring vulnerable
            pattern associated with the reported bug (e.g. noting that a
            vulnerability arises in more than one location due to a common
            misuse pattern on a particular class).
*   Helpful guidance for fixing the vulnerability, or providing an
            effective fix as part of the reporting process.
*   Identifying any particularly interesting or impactful issue that
            goes beyond the scope of a single report.
*   Any noteworthy cooperation in the reporting and resolution that goes
            significantly beyond the normal reporting guidelines.
*   We reward more where the reporter [provides a good analysis
            demonstrating probable exploitability](/system/errors/NodeNotFound).

### Bug Visibility

The visibility of security bugs is restricted until a fix has been widely
deployed. For more information see the [Security
FAQ](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/faq.md#TOC-Can-you-please-un-hide-old-security-bugs-).
