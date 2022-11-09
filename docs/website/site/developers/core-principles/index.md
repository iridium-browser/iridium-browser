---
breadcrumbs:
- - /developers
  - For Developers
page_name: core-principles
title: Core Principles
---

These are the things that are important to us: these principles shape the
product that we build. Note that the examples given are meant to be illustrative
rather than exhaustive.

**Speed**

Our objective is to make the fastest browser - and we tackle this in a number of
ways.

*   Because the speed of rich web applications is important to us, we
            develop our own JavaScript engine, called V8.
*   Because the speed of page display is important to us, we develop our
            own rendering engine, called Blink. It's incredibly fast.
*   We pay attention to the speed of user interactions - from starting
            the browser to common actions like opening new tabs and windows, and
            switching between them. Noticeable lags are not acceptable.
*   We develop and pay close attention to automated tests for all of
            these things, and run them in combination with new features we
            develop so that we can be sure we're always getting better, not
            worse.

When preparing changelists, we ask ourselves questions like:

*"Does my code make Chrome slower? If it does, how do I fix it?"*

*"Have I created automated performance testing for my feature? Have I created
variations of existing performance tests to measure performance with my feature
enabled?"*

*"What opportunities are there for better performance in my feature? Have I
filed bugs and labeled them with the Performance label?"*

*"How will my code perform when the operating system or hard disk is bogged
down?"*

*"Am I taking care to avoid "blocking file system access" on the UI and IO
threads?"*

Ask yourself these questions before you commit your code. If you make a change
that regresses measured performance, you will be required to fix it or revert.
There are many tools available to help isolate bottlenecks. (LINK to tuning
doc).

**Security**

As users spend more time in the browser and do more sensitive activities there
(banking, tax returns, email) it is increasingly important that we provide the
most secure environment that we can for our users.

We operate with the principle of least privilege. To the extent possible, code
is sandboxed and its privileges to interact with the host system are restricted
as tightly as possible. Untrusted (web) content is intended to never be handled
in privileged code, but instead passed off to sandboxed code for consumption.

We believe in defense in depth. We have protections for our users at multiple
levels. While the sandbox is designed to make sure that malware cannot install
itself onto and read data from your system, we have other components such as
Safe Browsing to help prevent users from reaching malware in the first place.

We believe that security must be usable. Showing the user a certificate's
fingerprint and throwing up our arms asking "what shall we do?" is not solving
the problem at hand. Instead, we strive to provide a more understandable context
in which relevant information (if any) is presented, along with explanations,
encouraging users to take a safe default action.

We believe that users should be safe by default. We should not expect users to
take actions to be secure, rather we should act given the information we have to
keep the user secure. We automatically update users to the latest version, we
set reasonable defaults for security-related features, and we understand that
users are focused on a task, not on security, and act accordingly.

**Stability**

Because browsers are more than just content viewers these days, we do a lot more
in them. It's important that they act more like platforms.

We apply conventional thinking in operating system design to the web browser. A
bug that affects one application should not bring all of them down. With that in
mind, we separate individual tabs into their own processes. We also do this for
plugins.

We try to write as many automated tests as we can, to make sure that the product
is still functioning as intended. We close the tree when the tests fail. We
revert changes that break them. We seek to improve our code coverage in such
tests over time.

We work to get a good sense of the stability of any build before we push it to
our users. We gather data in a couple of ways, such as crash dumps from people
who have opted in to sending them, and our own farm of automated test machines
that run many builds a day against Google's cache of various web sites. We use
the crash stats to determine how we should prioritize engineering work, and
avoid shipping particularly crashy builds to stabler channels.

When writing code, and before sending a changelist for review, we ask ourselves
questions like:

*"How can I write my code to maximize code coverage from unit tests?" ... unit
tests are typically less flaky and easier to debug than UI tests.*

*"Have I written or updated unit tests for the code I've changed?" ... all
non-trivial changes should have a unit test -- the more coverage we can get from
them, the more confident we can be in the stability of a release.*

*"Have I run valgrind while exercising my feature and its tests?"*

*"Have I isolated potentially brittle/untrusted code parsing in a utility
process?" ... for example, web pages, profile migration, theme PNG decoding,
extension parsing, etc. are all done in child processes.*

*"Have I hidden my unfinished experimental feature behind a command line flag so
as not to destabilize the weekly dev channel?"*

**Simplicity**

Google is all about incredibly sophisticated technology, but wrapped in a simple
user experience. We do everything we can to optimize user interactions. The
foremost way in which we've done this is by removing distractions from the
browser itself, and letting page content speak for itself. "Content, not Chrome"
is our mantra.

We tend to avoid complex configuration steps in every aspect of the product,
from installation through options. We take care of keeping the software up to
date so you never need to be concerned about running the latest, safest version
of Chrome.

We try not to interrupt users with questions they are not prepared to answer, or
get in their way with cumbersome modal dialog boxes that they didn't ask for.

We take hints from our host platform, but retain a distinctive aesthetic.

We want the browser to feel like a natural extension of your will. It should
feel fluid and delightful. It's about getting you to the information you need,
not about driving a piece of software.

When writing code and designing features, we ask questions like:

*"Have I talked to UI design group (chrome-ui-leads at google dot com) about my
change?"*

*"Have I made sure all non-trivial work is done off the UI thread, so the UI is
never unresponsive for &gt; 200ms?" - UI jank is the devil. Don't make your
change contribute to it.*

*"Have I done my best to avoid introducing unexpected modal workflows, popups,
questions the user can't answer, superfluous extra options?" - we bend over
backwards to avoid these things.*

*"Have I spent time before the next major release polishing my feature, making
sure it has all appropriate animations, plays nicely with other Chrome features
like themes, etc?" - UI polish is important to us. We want to make sure the UI
looks and feels tight. Think of the interior trim pieces on an expensive car.*
