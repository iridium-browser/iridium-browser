---
breadcrumbs:
- - /developers
  - For Developers
page_name: meet-the-web-platform-companion
title: Meet the Web Platform Companion Guide
---

# **Last updated 3/13/12**

The web platform is composed of a large collection of interoperable standards
that represent a solid foundation for building powerful apps. New standards are
born on the cutting edge, where various organizations experiment with new
capabilities. Those capabilities that capture the imagination of the broader
community may eventually become a formal part of the web platform.
The demos in the [Meet the Web Platform
videos](http://www.youtube.com/watch?v=3i4dtgh3ym0&list=PL0207724E1C9C22A6&feature=plpp_play_all)
show a few examples of some of the cool stuff you can do on the cutting edge of
the web platform. Not all of these capabilities have been standardized yet, some
only work in Chrome today, and some are still actively being implemented in
Chrome. This guide serves as a companion to the videos, providing more context
on each demo and the technology that powers them.

## Building on Foundations

### Web Components

*   Implementation status: Shadow DOM, a core foundational piece of Web
            Components, is available in Chrome 19+ behind the Shadow DOM flag.
            Other bits will be implemented in the near future.
*   Cross-browser support: Active interest demonstrated by Microsoft and
            Mozilla
*   Standardization status: All specifications under Web Components
            umbrella are being developed in the [W3C WebApps Working
            Group](http://www.w3.org/2008/webapps/)
*   Learn more:
    *   A comprehensive [introduction to Web
                components](http://dvcs.w3.org/hg/webcomponents/raw-file/tip/explainer/index.html)
    *   A readable introduction to [Shadow
                DOM](http://glazkov.com/2011/01/14/what-the-heck-is-shadow-dom/)
    *   Follow the [Web Components Google+
                page](https://plus.google.com/u/1/103330502635338602217/posts)
                to stay up to date
    *   W3C Editor’s Draft of the [Shadow DOM
                specification](http://dvcs.w3.org/hg/webcomponents/raw-file/tip/spec/shadow/index.html)

### Blocks are easy to style, and everything is a block

*   Implementation status: -webkit-filter is enabled in Chrome 19+. 3D
            CSS transformations has been enabled since M16. Video has been
            available for many releases.
*   Cross-browser support: filter is not implemented in other browsers
            yet, but should be soon. 3D transformations are supported in most
            other browsers. Video support is available in all modern browsers
*   Standardization status: All features are standardized or being
            standardized
*   Learn more:
    *   [CSS Filters specification
                ](http://dvcs.w3.org/hg/FXTF/raw-file/tip/filters/index.html)
    *   [HTML5
                video](http://www.html5rocks.com/en/tutorials/video/basics/)
    *   [3D
                transformations](http://www.html5rocks.com/en/tutorials/3d/css/)
    *   [BeerCamp example](http://2011.beercamp.com/)

### Improved layout primitives

*   Implementation status: The older version of the spec is supported,
            and the improved flexbox spec is being actively implemented in
            Chrome with basic support in Chrome 19.
*   Cross-browser support: Firefox, Safari, Chrome and Internet Explorer
            10 implement the old flexbox spec; updated spec should be
            implemented soon in Chrome and other browsers.
*   Standardization status: Fully standardized
*   Learn more:
    *   The improved [Flexbox
                specification](http://www.w3.org/TR/css3-flexbox/)
    *   [Grid layout specification](http://www.w3.org/TR/css3-layout/)
                (another app layout CSS module that is being actively
                standardized)

### Debugging on mobile

*   Implementation status: Works in Chrome 18+
*   Cross-browser support: N/A
*   Standardization status: N/A
*   Learn more:
    *   [Guide to debugging Chrome for Android
                Beta](http://code.google.com/chrome/mobile/docs/debugging.html)

### World Class Developer Tools

*   Implementation status: Fully implemented
*   Cross-browser support: Other browsers have strong tools as well
*   Standardization status: N/A
*   Learn more:
    *   [Overview of dev tools](http://code.google.com/chrome/devtools/)

### Powerful Text Layout

*   Implementation status: A side build of WebKit from Adobe
            demonstrates CSS exclusions and regions. Work to land that
            functionality in WebKit is ongoing, but doesn't yet work in Chrome.
*   Cross-browser support: No shipping browser supports it yet
*   Standardization status: Actively undergoing standardization at W3C
*   Learn more:
    *   Play around with [Adobe's WebKit build and
                examples](http://labs.adobe.com/technologies/cssregions/)
    *   [CSS Exclusions
                specification](http://dev.w3.org/csswg/css3-exclusions/)
    *   [CSS Regions
                specification](http://dev.w3.org/csswg/css3-regions/) (related
                to Exclusions but not featured in the video)

## Learning from Other Platforms

### Push notifications

*   Implementation status: push notifications for apps on the New Tab
            Page implemented but not yet available to developers. Notification
            center work underway.
*   Cross-browser support: Not yet, as this is part of the Chrome
            extension APIs
*   Standardization status: Not standardized

### Payments

*   Implementation status: Fully implemented
*   Cross-browser support: All modern browsers can use the payments API
*   Standardization status: Not standardized
*   Learn more:
    *   [Payments API
                reference](https://developers.google.com/in-app-payments/docs/samples)
    *   [Bastion](https://chrome.google.com/webstore/detail/oohphhdkahjlioohbalmicpokoefkgid)

### Web Intents

*   Implementation status: Initial functionality available in Chrome 19
            for Chrome Web Store apps.
*   Cross-browser support: No other browsers have implemented web
            intents yet.
*   Standardization status: Being standardized publicly at the W3C
*   Learn more:
    *   [WebIntents.org](http://webintents.org/) overview
    *   [Web Intents Draft
                API](http://dvcs.w3.org/hg/web-intents/raw-file/tip/spec/Overview.html)
    *   Web Intents Task Force [mailing
                list](http://lists.w3.org/Archives/Public/public-web-intents/)
                at the W3C
    *   [Imagemator](http://www.imagemator.com/)
    *   [CloudFilePicker
                app](https://chrome.google.com/webstore/detail/kpeiggegnjmcinljkdmjglpjopdjihff)
                for getting to your Picasa images
    *   [MemeMator
                app](https://chrome.google.com/webstore/detail/lkinojipklbmjkgmmpppmbhlhfpkhmed)
                to edit a photo

## On the Cutting Edge

### WebCam

*   Implementation status: implemented in Chrome 18+, requires enabling
            "Enable MediaStream" in about:flags. Note: as of this writing there
            are no security checks in place for access to webcam yet; do not
            browse untrusted sites with that flag enabled. Security UI will be
            landing in Chrome 19 for all platforms.
*   Cross-browser support: Implemented in Opera and being implemented in
            Firefox
*   Standardization status: Actively being standardized
*   Learn more:
    *   [CAPTURING AUDIO & VIDEO IN
                HTML5](http://www.html5rocks.com/en/tutorials/getusermedia/intro/)
                at HTML5Rocks
    *   [Minimal code
                example](http://webrtc.cloudfoundry.com/get_user_media)
    *   [Neave HTML5 Photobooth App](http://neave.com/webcam/html5/)
    *   [DMV photobooth app](http://dmv.nodejitsu.com/)

### Web Audio

*   Implementation status: Available in Chrome since version 14
*   Cross-browser support: No other browsers yet, although Firefox has a
            separate audio API
*   Standardization status: Being standardized at the W3C
*   Learn more:
    *   [Web Audio
                specification](https://dvcs.w3.org/hg/audio/raw-file/tip/webaudio/specification.html)
    *   [Introductory tutorial to the Web Audio
                API](http://www.html5rocks.com/en/tutorials/webaudio/intro/) at
                HTML5Rocks
    *   [Collection of Web Audio
                articles](http://www.html5rocks.com/tutorials/#webaudio) at
                HTML5Rocks
    *   [Plink](http://labs.dinahmoe.com/plink/) collaborative
                music-making app

### WebGL

*   Implementation status: fully implemented in all recent versions of
            Chrome
*   Cross-browser support: support in nearly all desktop browsers except
            for IE; Safari requires opt-in. Beginning to see support on mobile
            devices (Sony Experia, Opera Mobile)
*   Standardization status: standardized at the Khronos group
*   Learn more:
    *   [WebGL
                tutorial](http://www.html5rocks.com/en/tutorials/webgl/webgl_fundamentals/)
                at HTML5Rocks
    *   [WebGL water example scene](http://madebyevan.com/webgl-water/)
    *   [WebGL wiki](http://khronos.org/webgl/wiki)

### NaCl and GamePad

*   Implementation status: NaCl available for web store apps in Chrome
            14+. GamePad API is in M19+ behind the "Enable Gamepad" flag in
            about:flags.
*   Cross-browser support: NaCl only works in Chrome. GamePad is not
            implemented in other browsers (but will be soon).
*   Standardization status: NaCl is not standardized, but GamePad is
            being actively standardized.
*   Learn more:
    *   [AirMech ](http://www.airme.ch/)(without GamePad support for the
                time being)
    *   Learn more about the [GamePad
                API](https://wiki.mozilla.org/GamepadAPI)
                ([spec](http://dvcs.w3.org/hg/gamepad/raw-file/default/gamepad.html))
    *   Learn more about [Native
                Client](https://developers.google.com/native-client/)
