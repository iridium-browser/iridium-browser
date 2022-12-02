---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: layered-components-technical-approach
title: 'Layered Components: Technical Approach'
---

This document assumes that the reader has read the [high-level
design](http://www.chromium.org/developers/design-documents/layered-components-design)
of layered components, and presents strategies for realizing that design.

# Choosing Features to Refactor

We will aim to tackle simpler “leaf” features first. By tackling these features
first, we can wrestle with the basic issues and establish basic
structure/patterns in a simpler context. As we move to more complex features,
the extra complexities that they introduce will thus be easier to isolate and
focus on.

# Dealing with Upstreaming’s Incremental Nature

For the foreseeable future, Chrome for iOS will be in a state where some code is
upstream and some code is downstream. This fact creates two challenges:

    Upstream code needs to call code that is not yet upstreamed. This case might
    occur because of challenging dependency chains that need to be addressed
    incrementally.

    Not-yet-upstreamed code calls code that is upstream. This case causes
    problems because it makes Chrome for iOS vulnerable to bustage during
    merges.

The first case will be handled by introducing an API that allows the downstream
codebase to inject functionality into upstream features via embedder objects
(precisely following the example of ContentClient and friends).

The second case will be handled by introducing an API via which downstream iOS
code consumes upstream Chromium code. This “consumer API” will live upstream in
directories that are owned by iOS engineers together with unittests of the API
that specify the expected semantics. Thus, downstream usage of these APIs will
be visible to all Chromium engineers (likely there will be a policy of TBR’ing
an API OWNER on a change of a consumer API implementation but getting a full
review from an OWNER on a change of a consumer API definition).

# Refactoring a Feature

    Reach out to the feature owner(s) to make them aware that we are tackling
    this feature.

        They should already have the general context as we will have publicized
        the project on chromium.org/chromium-dev.

    Move the feature to its desired final location and lock down DEPS rules on
    it (put in the desired DEPS restrictions and then allow inclusions of all
    existing violations, together with a note to talk to one of us before adding
    to the list) in order to stop the bleeding on the feature while we work on
    it.

    Gain sufficient expertise in the feature to develop a design for refactoring

        Discussion with feature owners

        Strong desire against designs that require introducing ifdef’ing

        Strong desire against designs that require wrapper API around content
        layer and iOS embed layer

            Note that the two above points might well come into conflict at
            times, wherein consultation with darin@ and jam@ will likely be in
            order to decide on best way of proceeding

    Iterate on the design with feature owners

        If there are aspects of debate (e.g., the feature owner thinks that
        having some ifdef’s would be better than introducing a new layer, or it
        seems that introducing a specific wrapper api around content would
        greatly simplify the structure), bring in darin@/jam@ for discussion

    Turn the finalized feature-level design into CLs

    Once the feature is in its final form, introduce iOS consumer API around
    downstream usage of the feature to prevent downstream bustage while
    upstreaming of the clients of the feature is still ongoing

# Approaches to Challenges Encountered During Refactoring

### Laying the Groundwork via a Driver

Most features both receive incoming information from the content layer (e.g.,
IPC or notifications) and send out information to or query the content layer
(e.g., sending IPC, asking BrowserContext if it is off the record). To enable
abstracting these interactions, the general pattern is to introduce a FooDriver
interface. This interface will live in the core code of the component, and an
instance of it will be injected into this core code on instantiation. The
content-based and iOS drivers of the component will each have a concrete
subclass of the FooDriver interface.

Example: <https://codereview.chromium.org/16286020/>

### Abstracting WebContentsObserver

You can make the content-based FooDriver implementation be a
WebContentsObserver. This class can contain references to the core classes that
were previously themselves WebContentsObservers and call (potentially new)
appropriate public APIs that those core classes expose in response to observing
events on the given WebContents.

Example: <https://codereview.chromium.org/16286020/>

The iOS FooDriver implementation, by contrast, can observe WebState (iOS's
equivalent to WebContents).

**### Abstracting WebContentsUserData**

The content-based FooDriver implementation can itself be a WebContentsUserData,
and can own the core classes that were previously themselves WebContentsUserData
objects. Thus, the lifetime of these core classes will still be scoped to the
lifetime of the WebContents.

Example: <https://codereview.chromium.org/17225008/>

The iOS FooDriver implementation, by contrast, can be a WebStateUserData.

### Abstracting Listening to Notifications and IPC Reception

The approach to these is similar to that for WebContentsObserver: have the
content-based FooDriver implementation listen for the notifications/IPC, and
then invoke appropriate calls on the core classes (potentially creating new
public APIs on these core classes in order to do so).

Example of abstracting listening for notifications:
<https://codereview.chromium.org/17893010/>

Example of abstracting IPC reception:
<https://codereview.chromium.org/17382007/>

The iOS implementation of the feature will likely just call the new public APIs
on the core classes directly.

### Abstracting IPC Sending

Expose methods on the FooDriver interface that the core code can call where it
was previously directly sending IPC. The content-based FooDriver implementation
will implement these calls by sending IPC.

Example: <https://codereview.chromium.org/17572015/>

The iOS FooDriver will implement these methods via an iOS-specific flow.

**# General Strategies to Consider When Doing Refactoring**

    **If a class Foo gets a WebContents via a callback to another class (e.g.,
    FooDelegate-&gt;GetWebContents()), consider just having FooDelegate expose
    the API from WebContents that Foo uses. FooDelegate will likely already have
    (or be made to have) platform-specific implementations.**

    **To minimize duplication of header files, consider avoiding subclassing:
    Instead of having an interface FooDelegate.h with e.g. a content
    implementation FooDelegateContentImpl.h and FooDelegateContentImpl.cc, just
    have FooDelegate.h declare the concrete class and then have
    FoodDelegateContent.cc and FooDelegateIOS.cc.**

        **One question this raises is of declaring platform-specific variables
        in FooDelegate.h. This problem can of course be solved by if-defing, but
        we are looking to avoid if-defing in almost all cases. In some cases, it
        can be solved without ifdefing by the [Pimpl
        paradigm](http://en.wikipedia.org/wiki/Opaque_pointer).**

    **Alternative to introducing a wrapper API around the content layer and the
    iOS embed layer would be introducing typedef’s**

        **This would be a usage of ifdef’s but would be contained to a single
        location**

    **Difficult case arises when shared classes currently take in a WebContents
    instance (or similarly problematic class) in their constructor. These are
    the cases where we’ll have to consider very carefully to devise solutions
    that avoid the need for either a wrapper API or ifdef’ing. To put it a
    different way, if we have to introduce either a wrapper API or ifdef’ing, it
    will likely be due to these kinds of cases.**

**# Case Studies / Examples to Follow**

*   [components/autofill](/developers/design-documents/layered-components-technical-approach/making-autofill-into-a-layered-component)
