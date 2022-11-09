---
breadcrumbs:
- - /Home
  - Chromium
page_name: chromium-security
title: Chromium Security
---

The Chromium security team aims to provide Chrome and Chrome OS users with the
most secure platform to navigate the web, and just generally make the Internet a
safer place to hang out. We work on solutions for the [biggest user / ux
security problems](/Home/chromium-security/enamel), drive [secure architecture
design and implementation projects for the Chromium
platform](/Home/chromium-security/guts), [find and help fix security
bugs](/Home/chromium-security/bugs), [help developers to create more secure
apps](/Home/chromium-security/owp), and act as a [general security consulting /
review group](/Home/chromium-security/reviews-and-consulting) for the larger
Chromium project.

To learn more:

*   Read our [core security
            principles](/Home/chromium-security/core-principles), which we try
            to follow in all security work we do.
*   Check out our [security brag
            sheet](/Home/chromium-security/brag-sheet), which lists some of the
            technologies and recognition we're most proud of.
*   Check out some of the work we're doing to [detect and prevent
            security bugs](/Home/chromium-security/bugs), ensure that Chromium
            is [secure by design and resilient to
            exploitation](/Home/chromium-security/guts), and [make security
            easier for users and developers](/Home/chromium-security/enamel).
*   Peruse the [Security
            FAQ](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/faq.md)
            for answers to common questions.
*   Learn about how [Security
            Reviews](/Home/chromium-security/security-reviews) work in Chrome.
*   Check out some of our [Chrome-specific security
            education](/Home/chromium-security/education) documentation.
*   Check out the [PDFium
            Security](/Home/chromium-security/pdfium-security) page, too.
*   Here is the canonical "[prefer secure origins for powerful new
            features](/Home/chromium-security/prefer-secure-origins-for-powerful-new-features)"
            proposal text.
*   Here is the canonical "[Marking HTTP As
            Non-Secure](/Home/chromium-security/marking-http-as-non-secure)"
            proposal text.
*   Have a look at our public [Chrome Security Google Drive
            folder](https://drive.google.com/open?id=0B_KwtdC2J1Q6fjFNRElHUHhmLUlNbktKbFVkRXBlVGp0NkZvTDJvZVRZLXozOVFqTWtzM1E&authuser=0),
            which contains a whole bunch of useful documents as well.
*   We provide [quarterly
            updates](/Home/chromium-security/quarterly-updates) to what we're
            working on, if anything piques your interest get in touch!
*   Find out about [our memory
            safety](/Home/chromium-security/memory-safety) work.

### How can I get involved?

**Find bugs**

One of the quickest ways to get involved is finding and [reporting security
bugs](/Home/chromium-security/reporting-security-bugs). It will get prompt
attention from a [security
sheriff](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/sheriff.md),
be kept private until we coordinate disclosure, and possibly qualify for a cash
reward through our [Vulnerability Rewards
Program](/Home/chromium-security/vulnerability-rewards-program). We occasionally
run security contests outside of our regular reward program (e.g.
[Pwnium2](/Home/chromium-security/pwnium-2),
[Pwnium3](/Home/chromium-security/pwnium-3)) too.

For any issues other than a specific bug, email us at
[security@chromium.org](mailto:security@chromium.org). For non-confidential
discussions, please post to the [technical discussion
forums](/developers/technical-discussion-groups), including the public
[security-dev](https://groups.google.com/a/chromium.org/forum/#!forum/security-dev)
list for technical discussions.

**Become a committer**

We encourage interested parties to work towards [becoming a
committer](/getting-involved/become-a-committer). There are many types of
security related patch that we're excited to collaborate on:

*   Fixes for any security bugs you discover.
*   Implementing or improving security features, including
            security-related web platform features (examples: iframe sandbox,
            XSS auditor, CSP).
*   Implementing or improving security hardening measures (examples:
            defensive checks, allocator improvements, ASLR improvements).

**Become an IPC reviewer**

Bugs in IPC can have nasty consequences, so we take special care to make sure
additions or changes to IPC avoid [common security
pitfalls](/Home/chromium-security/education/security-tips-for-ipc). If you want
to get involved, check out how to become an IPC reviewer
[here](/Home/chromium-security/ipc-security-reviews).

**Join the team**

Access to Chromium security bugs and our team mailing list is restricted, for
obvious reasons. Before applying to join the team, applicants must be committers
and are expected to have made and continue to make active and significant
contributions to Chromium security. You should demonstrate some of the following
before applying:

*   Relevant technical expertise and a history of patches that improve
            Chromium security.
*   A history of identifying and responsibly reporting Chromium security
            vulnerabilities.
*   Other expertise and/or roles that would allow the applicant to
            significantly contribute to Chromium security on a regular basis.
*   \[required\]: Be a committer, and have no personal or professional
            association that is an ethical conflict of interest (e.g. keeping
            vulnerabilities or exploits private, or sharing with parties other
            than the vendor).

To apply for membership, please email
[security@chromium.org](mailto:security@chromium.org).

### How can I get access to Chromium vulnerabilities?

A history of fixed Chromium security bugs is best found via [security notes in
Stable Channel updates on the Google Chrome releases
blog](https://googlechromereleases.blogspot.com/search/label/Stable%20updates).
You can also find fixed, publicly visible Type=Bug-Security bugs in the [issue
tracker](https://crbug.com/) (note: security bugs automatically become publicly
visible 14 weeks after they are fixed). All security bugs are rated according to
our [severity
guidelines](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/security/severity-guidelines.md),
which we keep in line with industry standards.

Advance notice of (fixed) Chromium security vulnerabilities is restricted to
those actively building significantly deployed products based upon Chromium, or
including Chromium as part of bundled software distributions. If you meet the
criteria, and require advanced notice of vulnerabilities, request access via
[security@chromium.org](mailto:security@chromium.org). Your email should explain
your need for access (embedder, Linux distribution, etc.) and your continued
access will require that you follow the terms of list membership.

### There is one simple rule for any party with advance access to security vulnerabilities in Chromium: any details of a vulnerability should be considered confidential and only shared on a need to know basis until such time that the vulnerability is responsibly disclosed by the Chromium project. Additionally, any vulnerabilities in third-party dependencies (e.g. Blink, open source parser libraries, etc.) must be treated with the same consideration. Access will be terminated for any member who fails to comply with this rule in letter or spirit.
