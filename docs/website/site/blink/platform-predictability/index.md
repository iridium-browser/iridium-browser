---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: platform-predictability
title: Web Platform Predictability
---

Web developers say a big problem building for the web is all the surprises they
hit where something doesn't work the way they expected. The Chromium project
(along with other browsers and standards organizations) has long invested
heavily in reducing these problems. But we've lacked a coordinated effort to
track and improve the whole area.

The Web Platform Predictability project is an effort to make developing for the
web more predictable, and as a result more enjoyable, cost-effective and
competitive with native mobile platforms. In particular, we intend to focus
primarily on problems caused by:

*   Different browsers / platforms behaving differently
            (interoperability)
*   Behavior changing from one version of a browser to another
            (compatibility)
*   APIs with surprising side effects or subtle behavior details
            (footguns)

For details see:

*   [BlinkOn 7: Platform
            Predictability](https://docs.google.com/presentation/d/1pfu-wAxbkVN41Zgg9P3ln9tJB9AwKh9T3btyWvd17Rk/edit),
            and
            [web-platform-tests](https://docs.google.com/presentation/d/1s2Dick89wvJsuNJb4ia3pPt84NtMv8rZr0E_GFXJLrk/edit#slide=id.p)
*   Chrome Dev Summit: [Predictability for the
            Web](http://www.slideshare.net/robnyman/predictability-for-the-web/)
            talk ([video](https://youtu.be/meAl-s77DuA))
*   [Platform Predictability vision
            document](https://drive.google.com/open?id=1jx--r4elUfTP2EGo27UPGncLTAIW3ExF7N2JL7sjki4)
*   [BlinkOn 6: Web Platform
            Predictability](https://docs.google.com/presentation/d/1umK4QkfCvzicHVJKLNo2yDRyWSqQEamavW9QVFmugNY/edit)
            talk
            ([video](https://www.youtube.com/watch?v=ipfPyM-Kwyk&feature=youtu.be))
*   [Predictability mailing
            list](https://groups.google.com/a/chromium.org/forum/#!forum/platform-predictability)
*   [Ecosystem Infrastructure team
            charter](https://docs.google.com/document/d/1MgcisuMnvh3z6QNIjDSvRbt4uoNtmI_cljcQkGXzNQ8/edit)
            and [mailing
            list](https://groups.google.com/a/chromium.org/forum/#!forum/ecosystem-infra):
            the chromium team driving most of the interop-related work

## The difficulties of web development

*   [The Double-Edged Sword of the
            Web](https://ponyfoo.com/articles/double-edged-sword-web)
*   [The Fucking Open
            Web](https://hueniverse.com/2016/06/08/the-fucking-open-web/)
*   [10 things I learned from reading (and writing) the PouchDB
            source](https://pouchdb.com/2014/10/26/10-things-i-learned-from-reading-and-writing-the-pouchdb-source.html)
*   [(Web) Development Sucks, and It's Not Getting Any
            Better](http://blog.dantup.com/2014/05/web-development-sucks-and-its-not-getting-any-better/)
*   [Make the Web Work For
            Everyone](https://hacks.mozilla.org/2016/07/make-the-web-work-for-everyone/)

## Advice for Blink developers

*   Blink's mission is to make the web better, fight the temptation to
            focus narrowly on just making Chrome great!
*   Prefer [web-platform-test changes in your
            CLs](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_platform_tests.md)
            over web_tests changes where practical
*   What matters to web developers is eliminating surprises, so focus on
            [pragmatic paths to
            interop](https://docs.google.com/document/d/1LSuLWJDP02rlC9bOlidL6DzBV5kSkV5bW5Pled8HGC8/edit)
            that maximize web compat. If 3+ engines violate a spec in the same
            way, the bug is probably in the spec (especially when changing the
            engines would break websites). Specs are much easier to change than
            multiple implementations!
*   For breaking changes, reach out to
            platform-predictability@chromium.org for advice or read the [Blink
            principles of web compatibility](https://bit.ly/blink-compat)
*   Prioritize bugs in your areas that reflect real developer pain - eg.
            [those most
            starred](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component:Blink&sort=-stars&colspec=ID%20Stars%20Pri%20Status%20Component%20Opened%20Summary),
            [recent
            regressions](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=type%3Dbug-regression+pri%3D0%2C1+component%3Ablink+opened-after%3Atoday-60&colspec=ID+Pri+Mstone+ReleaseBlock+OS+Area+Feature+Status+Owner+Summary&x=m&y=releaseblock&cells=tiles).
*   Understand how your feature behaves on [all major
            browsers](https://browserstack.com/), file bugs for differences (on
            specs, other engines and/or blink).
*   Make sure new APIs have feedback from several real world web
            developers.
*   Get to know your peers on other engines, you'll be surprised how
            well your goals and interests are aligned!

## Relevant issue trackers

*   Chromium Hotlist-Interop: key bugs that impact interoperability
    *   Useful queries:
                [untriaged](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Hotlist%3DInterop+status%3Duntriaged%2Cunconfirmed&sort=&groupby=&colspec=ID+Pri+Component+Status+Owner+Summary+Modified&nobtn=Update),
                [high
                priority](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Hotlist%3DInterop+pri%3D0%2C1&colspec=ID+Pri+Component+Status+Owner+Summary+Modified&x=m&y=releaseblock&cells=ids),
                [most
                starred](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Hotlist=Interop&sort=-stars&colspec=ID%20Stars%20Pri%20Component%20Status%20Owner%20Summary%20Modified)
    *   [New Hotlist-Interop
                bug](https://bugs.chromium.org/p/chromium/issues/entry?template=Defect%20report%20from%20user&labels=Type-Bug,Pri-2,Cr-Blink,Hotlist-Interop)
*   [Chromium
            Blink&gt;Infra&gt;Ecosystem](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3ABlink%3EInfra%3EEcosystem+&colspec=ID+Pri+Component+Status+Owner+Summary+Modified&x=m&y=releaseblock&cells=ids):
            bugs for chromium infrastructure and tooling to promote
            interoperability
*   [Chromium
            Internals&gt;FeatureControl](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=component%3AInternals%3EFeatureControl+&colspec=ID+Pri+Component+Status+Owner+Summary+Modified&x=m&y=releaseblock&cells=ids):
            bugs related to how web platform features are enabled and measured
            (e.g. webexposed tests, UseCounters)
*   [Chromium bugs filed by other browser
            vendors](https://bugs.chromium.org/p/chromium/issues/list?can=2&q=reporter:microsoft.com,mozilla,webkit.org,apple.com&sort=-opened+-modified&colspec=ID%20Reporter%20Pri%20Component%20Status%20Owner%20Summary%20Opened%20Modified)
*   [GitHub issues for
            Chromestatus.com](https://github.com/GoogleChrome/chromium-dashboard/issues)

## Useful tools

*   [Compat analysis tools](/blink/platform-predictability/compat-tools)
*   Cross browser testing: [BrowserStack](https://www.browserstack.com),
            [Sauce Labs](https://saucelabs.com/)

## Other Resources

*   [web-platform-tests](https://github.com/w3c/web-platform-tests/)
*   [Webcompat.com](http://webcompat.com/)
*   [BlinkOn5: Interoperability Case
            Studies](https://docs.google.com/presentation/d/1pOZ8ppcxEsJ6N8KfnfrI0EXwPEvHwg3BHyxzXXw8lRE)
            ([video](https://www.youtube.com/watch?v=a3-zFbwsoEs))
*   [W3C Web Platform Incubation Community
            Group](https://www.w3.org/community/wicg/)
*   [The elastic band of
            compatibility](https://plus.google.com/+AlexKomoroske/posts/WNvcmeTFhzx)
