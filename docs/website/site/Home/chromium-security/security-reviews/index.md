---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: security-reviews
title: Chrome Security Reviews
---

# How to request Chrome Browser security reviews

All launches and major changes to Chrome undergo a security review.

This page aims to provide guidance for those seeking a review.

## Am I in the right place?

Depending on the nature of your change, you might need to go through up to three security
review processes, run by different teams.

These reviews consider different things so don't usually overlap. But nevertheless,
for maximal review speed, please cross-link the reviews so each reviewer can consult with
each other (which will also ensure you get consistent and cohesive answers.)

- For changes to web APIs, you'll likely want the
[Blink intents process](https://www.chromium.org/blink/launching-features/)
- For changes to backend Google services, you'll likely want Google's internal security review
process.
- For changes to Chrome application features, or any other
[non-trivial](http://go/chrome-launch#criteria) (Google-internal, sorry) changes to Chrome,
you're in the right place! Read on for more info.

## How does the Chrome security review process work?

#### Informal pre-review (optional):

If your project is especially tricky or large, please reach out to security@chromium.org
(for public stuff; preferable) or chrome-security@google.com (for Google internal stuff)
well ahead of time for an early design review or feedback on your design doc.

This doesn't mean you will be assigned a reviewer/POC but reviewers will drive by with
feedback. If we provide no feedback via this channel (likely by unfortunate email physics),
please reach out and we will give the doc the attention it deserves!

#### Formal review:

The full story is here:
[go/chrome-security-review-plan](http://go/chrome-security-review-plan)
(Google-internal, sorry).

TL;DR: File a launch entry, with a design document in the launch entry and provide comment
access. The security reviewer(s) (automatically assigned) will look at the design doc first,
and will probably comment and ask questions in the document.

## FAQ

#### Q: Why does Chrome do security reviews?

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

#### Q: Is there anything Chrome security reviews are NOT?

Unfortunately, security reviews don’t mean that you can stop caring about
security. Your team is still accountable and responsible for ensuring that your
code is free of security bugs. Security reviews won’t catch all bugs, but they
certainly do help to make sure your security practices are sound.

#### Q: Are Chrome and Chrome OS using the same security review process?

To file a Chrome OS feature survey, please follow the steps at
[go/cros-security-review](http://go/cros-security-review) (Google-internal, sorry).

#### Q: Do I need to work at Google for the Chrome security review process?

Filing a launch entry requires an @google.com account. For
non-Google/open source contributors, find a Google PM who can help you with your
launch. (If you don't know whom to ask, ask on chromium-dev@chromium.org).

#### Q: I have other questions. How can I get in contact?

The best address for us is security@chromium.org, or if you’re a Googler you can
reach us at chrome-security@google.com for Chrome stuff,
chromeos-security@google.com for Chrome OS stuff.
