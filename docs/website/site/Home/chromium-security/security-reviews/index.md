---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: security-reviews
title: Chrome Security Reviews
---

All launches and major changes to Chrome undergo a security review.

Please note that filing a launch bug requires an @google.com account. For
non-Google/open source contributors, find a Google PM who can help you with your
launch. (If you don't know whom to ask, ask on chromium-dev@chromium.org).

The Chrome Security Team used to ask engineers and PMs to provide the same
information over and over again for incremental launches. We also had a hard
time keeping on top of incremental changes and we weren't really using the
cumulative review data to give us insight into the ongoing engineering practices
across Chrome.

This process aims to address these issues and make the review process simpler
and faster for everyone. If you have further questions, ping adetaylor@.

## How does the security review process work?

The full story is here: [New Chrome Security Review
Plan](https://docs.google.com/document/d/11bnI_H0_Hg_o1mILjjH28PHB98bpSC4STh_OXLwv1gA/edit).

TL;DR: File a launch bug, and the security team will see it on their dashboard.
Make sure to link to a design document in the launch bug. The security
reviewer(s) will look to the design doc first, and will probably comment and ask
questions in the document.

If your project is especially tricky or large, it's best to reach out to
security@chromium.org (for public stuff; preferable) or
chrome-security@google.com (for Google confidential stuff) well ahead of time.

## FAQ

#### Q: Why do we do security reviews?

Great question, and thanks for asking it! Security reviews serve three main
purposes:

    Provides an opportunity to educate and advise on potential security issues.
    Chrome is one of the hardest pieces of software in the world to secure:
    millions of lines of security-critical code, rendering untrusted content
    from all corners of the web, sitting on over 1 billion devices... that makes
    it an attractive target for the bad guys! Everyone in Chrome is responsible
    for security, and we're here to help point out and avoid security pitfalls.

    It’s a second pair of (expert) eyes looking for security holes. An attacker
    needs to only find one security bug in your project to exploit, while you
    have the job of defending your entire codebase. Not fair, right? We want an
    engineer on Chrome to look over your project with an eye on security, before
    someone with different motives takes a look when the project lands.

    Gives the security team an overview of what is happening in Chrome. Good
    security advice is tailored to a team, but great security advice also
    considers the strategic direction of the entire product area. When we can
    keep tabs on developments and product trajectories across teams, it
    (hopefully) allows us to provide meaningful advice and recommendations,
    sometimes even before you approach us and ask for it!

#### Q: Is there anything security reviews are NOT?

Unfortunately, security reviews don’t mean that you can stop caring about
security. Your team is still accountable and responsible for ensuring that your
code is free of security bugs. Security reviews won’t catch all bugs, but they
certainly do help to make sure your security practices are sound.

#### Q: Are Chrome and Chrome OS using the same security review process?

Yes, but with one important difference: for Chrome OS, kerrnel@ is the main
point of contact. To file a Chrome OS feature survey, please follow the steps at
go/cros-security-review.

#### Q: I have other questions. How can I get in contact?

The best address for us is security@chromium.org, or if you’re a Googler you can
reach us at chrome-security@google.com for Chrome stuff,
chromeos-security@google.com for Chrome OS stuff.