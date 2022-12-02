---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: bugs
title: Security Bugs--
---

Bugs happen. We know this to be as true as other fundamental laws of physics so
long as we have humans writing code to bring new features and improvements to
Chromium. We also know some of these bugs will have security consequences, so we
do a number of things to prevent, identify, and fix Chromium security bugs.

[TOC]

## Security fuzzing

We've build fuzzing infrastructure that automatically and continuously security
["fuzz" test](http://en.wikipedia.org/wiki/Fuzz_testing) Chrome to find new bugs
and help engineers patch and test fixes.
[ClusterFuzz](/Home/chromium-security/bugs/using-clusterfuzz), as the system is
affectionately named, consists of 12000+ cores and fuzzes hundreds of millions
of test cases each day to produce de-duplicated security bugs with small
reproducible test cases. Since it was built (in 2009), ClusterFuzz has helped us
find and fix [roughly two thousand security bugs in
Chromium](https://code.google.com/p/chromium/issues/list?can=1&q=label%3AClusterFuzz+-status%3AWontFix%2CDuplicate+Type%3DBug-Security+&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
and other third party software.

## Vulnerability Response and Remediation

The [security sheriff](/Home/chromium-security/security-sheriff) is a rotating
role that handles all incoming and open security bugs. to all reported security
bugs. We are committed to releasing a fix for any
[critical](/developers/severity-guidelines) security vulnerabilities in [under
60
days](http://googleonlinesecurity.blogspot.com/2010/07/rebooting-responsible-disclosure-focus.html).

## Rewarding Vulnerability Research

We try to reward awesome security research from external folks in a few ways:
[Chromium Vulnerability
Rewards](http://www.chromium.org/Home/chromium-security/vulnerability-rewards-program)is
our ongoing program to reward security bug reports in Chrome and Chrome OS.
**Pwnium** is a contest we run semi-regularly for proof-of-concept Chrome
exploits. Our motivation is simple: we have a big learning opportunity when we
receive full end-to-end exploits. Not only can we fix the bugs, but by studying
the vulnerability and exploit techniques we can enhance our mitigations,
automated testing, and sandboxing. This enables us to better protect our users.

*   [Pwnium
            4](http://blog.chromium.org/2014/01/show-off-your-security-skills.html)
            at CanSecWest in March, 2014.
            [results](https://docs.google.com/presentation/d/1c90yZXNHs7w8oi7uXveEOCx5-8O_NZIxolEKalscuAQ/view).
*   [Pwnium
            3](http://blog.chromium.org/2013/01/show-off-your-security-skills-pwn2own.html)
            at CanSecWest in 2013:
            [results](http://blog.chromium.org/2013/03/pwnium-3-and-pwn2own-results.html)
*   [Pwnium
            2](http://blog.chromium.org/2012/08/announcing-pwnium-2.html) at
            Hack in the Box in 2012:
            [results](http://blog.chromium.org/2012/10/pwnium-2-results-and-wrap-up_10.html)
*   [Pwnium
            1](http://blog.chromium.org/2012/02/pwnium-rewards-for-exploits.html)
            at CanSecWest in 2012: results ([Part
            1](http://blog.chromium.org/2012/05/tale-of-two-pwnies-part-1.html),
            [Part
            2](http://blog.chromium.org/2012/06/tale-of-two-pwnies-part-2.html))

**Pwn2Own** is an independent contest that similarly awards proof-of-concept
exploits. We support these contests with sponsorships.

*   Pwn2Own at CanSecWest 2014:
            [results](https://docs.google.com/presentation/d/1c90yZXNHs7w8oi7uXveEOCx5-8O_NZIxolEKalscuAQ/view).
*   Pwn2Own at PacSec 2013: [Chrome on Android exploit
            writeup](https://docs.google.com/document/d/1tHElG04AJR5OR2Ex-m_Jsmc8S5fAbRB3s4RmTG_PFnw/edit?usp=sharing)
*   Pwn2Own at CanSecWest 2013:
            [results](http://blog.chromium.org/2013/03/pwnium-3-and-pwn2own-results.html),
            MWR labs' write up of their Chrome exploit ([Part
            1](https://labs.mwrinfosecurity.com/blog/2013/04/19/mwr-labs-pwn2own-2013-write-up---webkit-exploit/))
            ([Part
            2](https://labs.mwrinfosecurity.com/blog/2013/09/06/mwr-labs-pwn2own-2013-write-up---kernel-exploit/))

## Presentations

*   [Detect bad-casting at runtime with UndefinedBehavior
            Sanitizer](https://drive.google.com/file/d/0Bxvv8gduedamTEJCUlN6eERtWUE/view?usp=sharing)

## Links

*   [Bug Fix Times
            Statistics](https://docs.google.com/spreadsheets/d/1XyFE36AZFpbPkhu-fQO_Yu8R_eU_7IvsRWSHWXnX7hI/edit#gid=2094956046)
