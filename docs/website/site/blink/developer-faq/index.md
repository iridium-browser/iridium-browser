---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: developer-faq
title: Developer FAQ - Why Blink?
---

[« Back to the Blink project page](http://www.chromium.org/blink)

[TOC]

### Why is Chrome spawning a new browser engine?

There are two main reasons why we’re making this change.

    The main reason is that Chromium uses a different multi-process architecture
    from other WebKit-based browsers. So, over the years, supporting multiple
    architectures has led to increasing complexity for both the WebKit and
    Chromium communities, slowing down the collective pace of innovation.

    In addition, this gives us an opportunity to do open-ended investigations
    into other performance improvement strategies. We want web applications to
    be as fast as possible. So for example, we want to make as many of the
    browser’s duties as possible run in parallel, so we can keep the main thread
    free for your application code. We’ve already made significant progress
    here--for example by reducing the impact JavaScript and layout have on page
    scrolling, and making it so an increasing number of CSS animations can run
    at 60fps even while JavaScript is doing some heavy-lifting--but this is just
    the start.

We want to do for networking, rendering and layout what V8 did for JavaScript.
Remember JS engines before V8? We want the same sort of healthy innovation that
benefits all users of the web, on all browsers.

### What sorts of things should I expect from Chrome?

In the Blink [Architectural
Changes](http://www.chromium.org/blink#architectural-changes) section we have
listed a few changes that will improve the speed and stability of the web
platform in Chrome. Meanwhile, there are more improvements whose feasibility and
performance benefits we're excited to investigate:

*   Deliver a speedier DOM and JS engine
    *   Multi-process, security-focused, and faster low-overhead DOM
                bindings to V8
    *   JIT DOM attribute getters in V8. That would allow V8 to access
                div.id, div.firstChild, etc without leaving JIT code. Mozilla is
                also meanwhile trying to[ JIT DOM attribute
                getters](https://bugzilla.mozilla.org/show_bug.cgi?id=747285).
    *   Implement the DOM in JS. This has the potential to make
                JavaScript DOM access dramatically faster, but will involve a
                very large re-write of WebKit’s DOM implementation. IE is
                already doing this and overall, this helps yield faster GC, and
                faster DOM bindings
    *   Support snapshotting in V8. This could allow us to have no
                parse-time overhead and near-instant startup of previously
                loaded pages.
*   Keep the platform secure
    *   Better sandboxing of the compositor thread
    *   [Out-of-process
                iframes](http://www.chromium.org/developers/design-documents/oop-iframes).
                Use renderer processes [as a security
                boundary](http://www.chromium.org/developers/design-documents/site-isolation)
                between cross-site iframes.
*   Refactor for performance
    *   Reduce binding layer overhead. We can make things even faster by
                simplifying the many abstractions in the binding layers that
                have existed to share code between JavaScriptCore and V8 (e.g.
                ScriptState, DOMRequestState, etc).
    *   Improve the performance of style resolution.
    *   Improve utilization of multiple cores.
*   Enable more powerful rendering and layout
    *   Pursue multi-threaded layout
    *   Overhaul style recalculation and selector resolution performance
    *   Stop creating renderers for hidden iframes
    *   Fix old bugs like plugins unloading when they're set to
                display:none.
    *   Allow generated content to be selectable and copy-pasteable.
    *   Rewrite event handling to be more consistent and have fewer bugs
                with focus, mouse up, clicks, etc.
    *   Fire `<iframe>` unload events async to make removeChild faster.

### Is this new browser engine going to fragment the web platform's compatibility more?

We're keenly aware of the compatibility challenges faced by developers today,
and will be collaborating closely with other browser vendors to move the web
forward and preserve the interoperability that has made it a successful
ecosystem. We’ve also put a lot of work over the past few years into reducing
that pain. Take one of the huge successes of the web standards community: the
HTML5 Parser. All major browser engines now share the exact same parsing logic,
which means things like broken markup, &lt;a&gt; tags wrapping block elements,
and other edge cases are all handled consistently across browsers. This
interoperability is important to Chrome and we want to defend it.

Our team regularly contributes to sites that document browser support and
differences like Mozilla's [MDN](https://developer.mozilla.org/en-US/),
[WebPlatform.org](http://docs.webplatform.org/), and [Can I
Use](http://caniuse.com/). The last thing we want is for things to go backwards.

We see testing as the critical piece of web browser interoperability. Chrome
currently shares and runs tests that were authored by Opera, Mozilla, and W3C
Working Groups and we'll be doing a better job of this going forward. Developers
need to be able to rely on Chrome’s implementation of standards, and that’s
something we take very seriously. See the
[Testing](http://www.chromium.org/blink#testing) section for our plans.

### Hold up, isn't more browsers sharing WebKit better for compatibility?

It's important to remember that WebKit is already not a homogenous target for
developers. For example, features like WebGL and IndexedDB are only supported in
some WebKit-based browsers. [Understanding WebKit for
Developers](http://paulirish.com/2013/webkit-for-developers/) helps explain the
details, like why `<video>`, fonts and 3D transforms implementations vary across
WebKit browsers.

Today Firefox uses the Gecko engine, which isn’t based on WebKit, yet the two
have a high level of compatibility. We’re adopting a similar approach to Mozilla
by having a distinct yet compatible open-source engine. We will also continue to
have open bug tracking and [implementation status](http://chromestatus.com/) so
you can see and contribute to what we’re working on at any time.

From a short-term perspective, monocultures seem good for developer
productivity. From the long-term perspective, however, monocultures inevitably
lead to stagnation. It is our firm belief that more options in rendering engines
will lead to more innovation and a healthier web ecosystem.

### How does this affect web standards?

Bringing a new browser engine into the world increases diversity. Though that in
itself isn't our goal, it has the beneficial effect of ensuring that multiple
interoperable implementations of accepted standards exist. Each engine will
approach the same problem from a different direction, meaning that web
developers can be more confident in the performance and security characteristics
of the end result. It also makes it less likely that one implementation's quirks
become de facto standards, which is good for the open web at large.

### Will we see a `-chrome-` vendor prefix now?

We’ve seen how the proliferation of vendor prefixes has caused pain for
developers and we don't want to exacerbate this. As of today, Chrome is adopting
a policy on vendor prefixes, one that is similar to [Mozilla's recently
announced
policy](http://lists.w3.org/Archives/Public/public-webapps/2012OctDec/0731.html).

In short: we won't use vendor prefixes for new features. Instead, we’ll expose a
single setting (in `about:flags`) to enable experimental DOM/CSS features for
you to see what's coming, play around, and provide feedback, much as we do today
with [the “Experimental WebKit
Features”/"](https://plus.google.com/+GoogleChromeDevelopers/posts/ffDPaPAMkMZ)==Enable
experimental Web Platform features"==[
flag](https://plus.google.com/+GoogleChromeDevelopers/posts/ffDPaPAMkMZ). Only
when we're ready to see these features ship to stable will they be enabled by
default in the dev/canary channels.

For legacy vendor-prefixed features, we will continue to use the `-webkit-`
prefix because renaming all these prefixes to something else would cause
developers unnecessary pain. We've [started looking
into](https://plus.google.com/+GoogleChromeDevelopers/posts/Rh1aMkzucgV) real
world usage of HTML5 and CSS3 features and hope to use data like this to better
inform how we can responsibly deprecate prefixed properties and APIs. As for any
non-standard features that we inherited (like `-webkit-box-reflect`), over time
we hope to either help standardize or deprecate them on a case-by-case basis.

### So we have an even more fragmented mobile WebKit story?

We really don’t want that; time spent papering over differences is time not
spent building your apps’ features. We’re focusing our attention on making
Chrome for Android the best possible mobile browser. So you should expect the
same compatibility, rapid release schedule and super high JS and DOM performance
that you get in desktop Chrome.

Your site or app's success on the mobile web is dependent on the mobile browsers
it runs on. We want to see that entire mobile web platform keeps pace with, and
even anticipates, the ambitions of your app. Opera is already shipping a beta of
their Chromium-based browser which has features and capabilities very similar to
what's in Chrome on Android.

### What's stopping Chrome from shipping proprietary features?

Our goal is to drive innovation and improve the compatible, open web platform,
not to add a ton of features and break compatibility with other browsers. We're
introducing strong developer-facing policies on [adding new
features](http://www.chromium.org/blink#new-features), the [use of vendor
prefixes](http://www.chromium.org/blink#vendor-prefixes), and [when a feature
should be considered stable enough to
ship](http://www.chromium.org/blink#compatibility). This codifies our policy on
thoughtfully augmenting the platform, and as transparency is a core principle of
Blink, we hope this process is equally visible to you. The [Chromium Feature
Dashboard](http://www.chromestatus.com/features) we recently introduced offers a
view of the standards and implementation status of many of our implemented and
planned features.

Please feel free to watch the development of Blink via
[Gitiles](https://chromium.googlesource.com/chromium/blink), follow along on the
[blink-dev mailing
list](https://groups.google.com/a/chromium.org/group/blink-dev), and join
`#blink` on Freenode.

We know that the introduction of a new rendering engine can have significant
implications for the web. In the coming months we hope to earn the respect of
the broader open web community by letting our actions speak louder than words.

### Is this just a ruse to land Google-developed technologies?

Nope, not at all! We're instituting [strong guidelines on new
features](http://www.chromium.org/blink#new-features) that emphasize standards,
interoperability, and transparency. We expect to hold all new shipping features
that affect web developers on the open web up to the same level of scrutiny.
Technologies and standards developed primarily within Google will be held to the
same guidelines as others.

### What should we expect to see from Chrome and Blink in the next 12 months? What about the long term?

Our main short-term aim is to improve performance, compatibility and stability
for all the platforms where we ship Chrome. In the long term we hope to
significantly improve Chrome and inspire innovation among all the browser
manufacturers. We will be increasing our investment in conformance tests (shared
with W3C working groups) as part of our commitment to being good citizens of the
open web.

### Is this going to be open source?

Yes, of course. [Chromium is already open-source](http://www.chromium.org/Home)
and Blink is part of that project. Transparency is one of our core principles.
[Developing Blink](http://www.chromium.org/blink#participating) covers this in
detail.

### Opera recently announced they adopted Chromium for their browsers. What's their plan?

Opera will be adopting Blink, as mentioned by [Bruce Lawson on his
blog](http://www.brucelawson.co.uk/2013/hello-blink/).

### Why is this is good for me as a web developer?

One of the keys to improving users’ experience is to give developers the tools,
features, compatibility and performance they need to get the most out of the
platform. Although the move is borne out of architectural necessity, it also
allows us to prioritize the features that you need to build the next generation
of apps, on both mobile and desktop. Similar to the introduction of V8, we hope
this will spur innovation and you can and should expect the whole web platform
to benefit.

Our ambitions are high and we continue to need the feedback and contributions
that have made Chrome the browser it is today. You should also expect improved
transparency in Blink's development processes, so getting involved will be
easier than ever. Please, review the [Chromium Feature
Dashboard](http://www.chromestatus.com/features), experiment with future
features in Dev/Canary and [file any bugs](http://crbug.com/) you find.

~ FAQ authored by Paul Irish and Paul Lewis on the Chrome Developer Relations
team

[<img alt="image"
src="/blink/developer-faq/paulhead.jpg.1365009992996.png">](/blink/developer-faq/paulhead.jpg.1365009992996.png)[<img
alt="image"
src="/blink/developer-faq/lewishead.jpg.1365009990294.png">](/blink/developer-faq/lewishead.jpg.1365009990294.png)

# Q&A Video

After the Blink announcement, the developer community submitted hundreds of
questions and votes on Google Moderator ([goo.gl/Uu0qV](http://goo.gl/Uu0qV))
that sought answers to your tough questions about Blink.

Engineering leads Darin Fisher and Eric Seidel are joined by PM Alex Komoroske
and Developer Advocate Paul Irish

Below are the top-voted questions, along with timecodes you can click (will open
in a new window):
[1:12](http://www.youtube.com/watch?v=TlJob8K_OwE#t=1m12s) What will be the
relationship between the WebKit and Blink codebases going forward?
[2:42](http://www.youtube.com/watch?v=TlJob8K_OwE#t=2m42s) When will Blink ship
on the Chrome channels Canary/Beta/Stable?
[3:25](http://www.youtube.com/watch?v=TlJob8K_OwE#t=3m25s) How does the plan for
transitioning the WebKit integrated in Android to Blink look like?
[4:59](http://www.youtube.com/watch?v=TlJob8K_OwE#t=4m59s) Can you elaborate on
the idea of moving the DOM into JavaScript?
[6:40](http://www.youtube.com/watch?v=TlJob8K_OwE#t=6m40s) Can you elaborate on
the idea of "removing obscure parts of the DOM and make backwards incompatible
changesthat benefit performance or remove complexity"?
[8:35](http://www.youtube.com/watch?v=TlJob8K_OwE#t=8m35s) How will Blink
responsibly deprecate prefixed CSS properties?
[9:30](http://www.youtube.com/watch?v=TlJob8K_OwE#t=9m30s) What will prevent the
same collaborative development difficulties that have hampered Webkit emerging
in Blink, as it gains more contributors and is ported to more platforms?
[12:35](http://www.youtube.com/watch?v=TlJob8K_OwE#t=12m35s) Will changes to
Blink be contributed back to the WebKit project?
[13:34](http://www.youtube.com/watch?v=TlJob8K_OwE#t=13m34s) Google said
problems living with the WebKit2 multi-process model was a prime reason to
create Blink, but Apple engineers say they asked to integrate Chromium's
multi-process into WebKit prior to creating WebKit2, and were refused. What
gives?
[16:46](http://www.youtube.com/watch?v=TlJob8K_OwE#t=16m46s) Is the plan to
shift Android's &lt;webview&gt; implementation over to Blink as well?
[17:26](http://www.youtube.com/watch?v=TlJob8K_OwE#t=17m26s) Will blink be able
to support multiple scripting languages? E.g. Dart.
[19:34](http://www.youtube.com/watch?v=TlJob8K_OwE#t=19m34s) How will affect
other browsers that have adopted WebKit?
[20:44](http://www.youtube.com/watch?v=TlJob8K_OwE#t=20m44s) Does this means
Google stops contributions to WebKit?
[21:31](http://www.youtube.com/watch?v=TlJob8K_OwE#t=21m31s) What Open Source
license will Blink have? Will it continue to support the H.264 video codec?
[22:11](http://www.youtube.com/watch?v=TlJob8K_OwE#t=22m11s) Any user-agent
string changes?
[23:38](http://www.youtube.com/watch?v=TlJob8K_OwE#t=23m38s) When we'll be able
to test first versions of Blink in Chromium?
[24:15](http://www.youtube.com/watch?v=TlJob8K_OwE#t=24m15s) How can developers
follow Blink's development?
[25:40](http://www.youtube.com/watch?v=TlJob8K_OwE#t=25m40s) What is
[chromestatus.com](http://chromestatus.com/) about?
[26:40](http://www.youtube.com/watch?v=TlJob8K_OwE#t=26m40s) How will this
impact Dart language's progress?
[27:13](http://www.youtube.com/watch?v=TlJob8K_OwE#t=27m13s) Will this be a
direct competitor against Mozilla's new engine?
[29:03](http://www.youtube.com/watch?v=TlJob8K_OwE#t=29m03s) When will all
existing vendor prefixes in Blink be phased out?
[30:20](http://www.youtube.com/watch?v=TlJob8K_OwE#t=30m20s) Will you support
-blink-text-decoration: blink? ;)
