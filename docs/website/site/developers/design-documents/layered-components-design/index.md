---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: layered-components-design
title: 'Layered Components: Design'
---

## Overview

Chrome for iOS cannot use the content layer as-is due to the restrictions that
Apple places on third-party browsers on iOS. Given this fact, the goal of this
project is to enable iOS’s usage of chrome- and component-level features that:

*   Minimizes pain on Chromium developers, both for iOS and for other
            platforms
*   Enables Chromium developers developing multi-platform features to
            target iOS as well

To that end, we will be performing refactorings to introduce **layered
components**: components that consist of shared code that is content-free
together with a driver based on the content layer and an alternative driver for
iOS that uses an iOS-specific API.

## Motivation

Apple restricts third-party iOS apps to using UIWebView as the
JavaScript/rendering engine and additionally to being single-process. This fact
means that Chrome for iOS:

*   Cannot use the existing content/ implementation, which is based on
            V8/Blink and is multiprocess.
*   Could provide at best a partial alternative implementation of the
            content API due to the black-box nature of UIWebView.

As a result, iOS usage of chrome- and component-level features currently
requires a sea of ifdef’s, code modifications, and code additions. Upstreaming
this surgery would introduce maintainability and readability challenges.

The fact that most Chromium developers have no visibility into iOS’s
restrictions and requirements additionally causes a raised level of difficulty
for developers looking to target feature work to iOS in addition to other
platforms.

## Layered Components

Our approach to the challenges outlined above is to gradually **eliminate
dependence on the content API in code that is used on iOS** (see the below
background for discussion of alternative approaches and how we converged on this
approach).
In determining how to structure this approach within the codebase, we took the
position that DEPS restrictions are easiest for developers to understand and
introduce the least amount of maintainability pain when they are put within a
recognizable structure. We will introduce **layered components**. These
components will live in one of a small number of known locations within the
source tree (e.g., src/components). Each such component will consist of the
following:

*   A core/ subdirectory consisting of code shared by all platforms.
            This code cannot depend on content/, and will contain e.g. core
            model logic of the component.
*   An ios/ subdirectory that contains code that depends on an
            iOS-specific API and drives the shared code for iOS.
*   A content/ subdirectory that contains code that can depend on
            content/ and drives the shared code for all other platforms. For
            example, this subdirectory might contain a WebContentsObserver that
            invokes behavior of the shared code when a WebContents is destroyed.

The technical details of our plan to realize this approach are detailed
[here](/developers/design-documents/layered-components-technical-approach), and
a detailed description of the proposed structure within the Chromium codebase is
detailed
[here](/developers/design-documents/structure-of-layered-components-within-the-chromium-codebase).

## FAQ

*   **Won’t creating this structure introduce extra layering in the
            affected features?** Inevitably it will. However, we have explicit
            goals of (a) minimizing added layering to reduce maintainability
            pain and (b) making decouplings as logical as possible (e.g.,
            decoupling model code from code interacting with the renderer via
            IPC). We plan to work together with feature owners to develop
            agreeable designs.
*   **What is this “iOS-specific API”?** It will be an API that will
            have a similar purpose as the content API (e.g., allow an object to
            know when a webpage has finished loading) but will be much smaller.
            In a sense, it will be a “content API around UIWebView.”
*   **Will there be a wrapper API around the content API and the iOS
            API?** Unknown at this time. The goal is to avoid such an API;
            however, if it turns out that having a thin wrapper API would
            greatly simplify the structure of multiple components, such an API
            might be introduced.
*   **Without a wrapper API, how will you possibly handle
            WebContents/NavigationController/BrowserContext/etc.?** These
            questions will have to be examined on a case-by-case basis. Starting
            from the position that we would like to avoid a wrapper API will
            enable isolating the cases (if any) where the lack of such a wrapper
            API causes significant pain.
*   **What if the shared code of a layered component needs to talk to
            its driver?** We expect that in general communication between the
            shared code of a layered component and its driver will be
            bi-directional. Communication from the shared code to a driver will
            be done via classes that the shared code declares and drivers define
            (the exact fashion in which this will be done will likely have to be
            approached on a case-by-case basis with the goal of minimizing
            boilerplate/code duplication).
*   **Will this work make the Chromium codebase less maintainable as a
            whole?** While the primary motivation of this project is to enable
            upstream code/features to be shared by (and developed for) iOS in
            the most maintainable way, we see several other potential benefits
            that are independent of iOS:
    *   Mitigate the monolithic nature of chrome/browser
    *   Enable simpler unit testing of the code that will be shared
    *   Perform refactorings that increase understandability of the
                impacted features
*   **How large is the scope of this project?** Of the ~80 directories
            under chrome/browser, iOS uses ~30.
*   **I think that approach &lt;X&gt; would be better!** We welcome
            feedback and discussion; please read the background section below as
            context.

## Background: iOS and the Content API

We have examined several approaches to iOS’s usage of content:

1.  iOS uses all the parts of the content API that it needs/can support,
            providing alternative partial implementations of interfaces for
            which it cannot share the existing full-fledged implementation
            (e.g., WebContents).
2.  iOS uses only the parts of the content API for which it can share
            the implementations, refactoring the Chromium codebase to remove and
            disallow usage of problematic parts of the content API (e.g.,
            WebContents) in shared code.
3.  iOS does not use the content API (the approach that we ultimately
            decided on).

The conclusion re: Approach 1 was that the fact that iOS would only be able to
provide a partial implementation of the content API would make it more
challenging for developers to develop against the API: they would have to always
be aware of (a) whether the given piece of code that they were working on was
shared on iOS, and (b) which subsets of the content API were OK to use in code
shared on iOS.
The conclusion re: approach 2 was similar to that re: approach 1, with the added
concern that performing somewhat arbitrary-looking refactorings throughout the
codebase would introduce maintainability and readability problems (initial CLs
targeting this approach bore out these fears).

## Contact

Stuart Morgan (stuartmorgan@) and Colin Blundell (blundell@) are the leads on
this project. Darin Fisher (darin@) and John Abd-El-Malek (jam@) are providing
guidance, direction and approval.
