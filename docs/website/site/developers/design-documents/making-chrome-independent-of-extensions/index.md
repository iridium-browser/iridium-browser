---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: making-chrome-independent-of-extensions
title: Making Chrome Independent of Extensions
---

## **The goal of this work is to make the core Chrome browser code independent of extensions. Direct callsites of extensions code from non-extensions code would be replaced by adding observer-style interfaces onto the non-extensions code and/or abstracting the functionality that extensions provides non-extensions code into delegate interfaces as appropriate. In addition to the general desire for [componentization](http://www.chromium.org/developers/design-documents/browser-components) of [extensions](https://docs.google.com/a/google.com/document/d/1hSwqniJVtk3he1fTl_PW422_48F7ZRQUpXLDPqCmRgk/edit), the concrete motivation for the work is the challenges that Chrome for iOS would face in adopting/extending the Chrome for Android solution to extensions (i.e., build enough extensions-related code that the linker is satisfied).**

**This work is complementary to the
[work](https://docs.google.com/a/google.com/document/d/1hSwqniJVtk3he1fTl_PW422_48F7ZRQUpXLDPqCmRgk/edit)
currently underway to reduce the dependence of extensions on Chrome. There may
be cases where this work is blocked by that work, but there is also much that
can be done toward this goal in parallel with that complementary work. The
components team (most notably, yoz@ and joi@) have been involved in discussions
on this work and are in agreement both on tackling the problem and on the basic
approach.**

## Approach

    ****Take a given class Foo outside of extensions that directly uses extensions-related code.****

    ****Add observer and/or delegate interfaces to Foo that enable moving knowledge of extensions out of Foo.****

    ****Once Foo no longer knows anything about extensions, add a DEPS restriction such that knowledge of extensions cannot be added back to Foo. When a given directory is cleaned of extensions knowledge, this DEPS restriction can be generalized to the directory.****

    ****Goto 1.****

5.  ****When all such knowledge is removed, remove the dependence on
            browser_extensions from the chrome_browser target.****

## FAQ

*   **Sounds great; do you need help?** Yes! Please contact blundell@ or
            stuartmorgan@ re: ways to get involved.
*   **What will happen to the Chrome for Android solution for extensions?

    Android developers (specifically, nileshagrawal@) have indicated that once this solution is complete Android would be happy to be moved to it; at that point the modifications that Android had to make to extensions code would be removed, and ENABLE_EXTENSIONS would serve to signify whether the browser_extensions target was built or not.**

    **Has this proposal been vetted by Chromium eng leadership?** Yes; an
    earlier version of this proposal was developed in conjunction with yoz@ and
    joi@ from the components team, and the proposal in its current form is the
    result of going through the design review process with cpu@ and jam@.

*   **How can I follow along?** **There is a [tracking
            bug](https://code.google.com/p/chromium/issues/detail?id=186422).**
