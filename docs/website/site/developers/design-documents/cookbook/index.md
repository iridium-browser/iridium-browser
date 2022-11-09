---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: cookbook
title: Browser Components / Layered Components Cookbook
---

# being the story of how a fresh new component is born, or how a veteran feature is rescued from a tangled web of ugly dependencies and reborn as a shiny component.

## Overview

This document serves as a guide for the process of turning a Chromium feature
into a component or a layered component. The design docs for components and
layered components are assumed as background.

## Motivation and Layering

We’ve started having multiple top-level applications. For one of these (Chrome
for Android), the necessary approach was to #ifdef in //chrome, but we want to
avoid this for future top-level applications.

The //components layer is a place for optional features that products built out
of the Chromium codebase may want to use. Most of the components currently there
are features that previously lived in //chrome but were extracted so that they
could be reused by other top-level applications such as //android_webview.

New features that will get used by more than one top-level application should be
written as components. For top-level applications other than Chrome for Android,
features they wish to reuse should first be extracted into //components. This
document is a cookbook -type guide for how to create a new component, and how to
extract an existing feature from //chrome into a component. The below diagram
shows the place of components in the Chromium hierarchy. Note that the diagram
is over-simplified in that not all components allow dependences on //content;
for example, a component that is shared by iOS will either disallow dependencies
on //content entirely or be in the form of a layered component.

[<img alt="image"
src="/developers/design-documents/cookbook/layercake.png">](/developers/design-documents/cookbook/layercake.png)

## Creating a new Browser Component

Creating a new component is straightforward. Follow the rules in the [design
document](http://www.chromium.org/developers/design-documents/browser-components),
and the examples already in the //components directory. TL;DR version:

    A component named xyz lives in //components/xyz.

    Code in xyz should be enclosed in namespace xyz.

    There should be a strict //components/xyz/DEPS to enforce that the component
    depends only on lower layers.

        Low-level modules (e.g., //base, //net, potentially //content)

        Other //components, in a strictly tree-shaped graph - no circular
        dependencies.

    A component may depend concretely on lower layers and other components. For
    stuff it needs its embedder to fulfill, it should define a client interface
    that the embedder will implement and provide.

    A component that is used on iOS and also wishes to depend on //content must
    be in the form of a [layered
    component](http://www.chromium.org/developers/design-documents/layered-components-design).

    A component used only by the browser process contains code directly in its
    directory.

    A component used by multiple process types has a directory per process, e.g.

        //components/xyz/browser

        //components/xyz/renderer

        //components/xyz/common

    A component can optionally be a separate dynamic library in the components
    build.

    Create, own and configure the component in the embedder layer, e.g. in
    //chrome/browser.

## Extracting an existing feature to a Browser Component

These are the typical steps to extract a feature:

    Identify a feature you need to extract and reach out to the feature owners
    to inform them of your desire to componentize the feature.

    Create a design document for the componentization and do a file-by-file
    analysis of the //chrome dependencies of the production code of the feature
    (you can leave the componentization of unit tests for a second pass). If the
    feature will be used on iOS, include //content dependencies in your
    analysis.

    From the file analysis, do the following: generate a set of high-level tasks
    involved in the componentization (e.g., inject FooService dependency
    directly instead of obtaining FooService from Profile); if the feature is
    intended for use by iOS, determine whether the component needs to be a
    [layered
    component](http://www.chromium.org/developers/design-documents/layered-components-design);
    and determine whether the feature's componentization is blocked by that of
    other //chrome features (if so, go back to step 2 for the blocking
    features).

    Map the high-level tasks into a bugtree whose leaf nodes are
    individual-CL-granularity bugs. Give all bugs in the bugtree a relevant
    hotlist (e.g., Hotlist-Foo-Component) so that the list can be easily
    searched for and analyzed.

    Create the component (setting it up with the intended final DEPS structure)
    and move some leaf files (i.e., files with few or no problematic
    dependencies) into it. Note that in this first pass you can have the
    component's targets be static libraries.

    Burn down the bugtree, moving files into the component incrementally as
    their problematic dependencies are eliminated.

    Once all production code is componentized, do a second pass componentizing
    unittests (typically relatively straightforward once the production code has
    been componentized).

    Put all the code in the component into the component's namespace (e.g., code
    in //components/foo should be in the "foo" namespace).

    If desired, fix up .gypi files and add export declarations to build it as a
    component

        See examples in other components, e.g.
        //components/webdata/common/webdata_export.h and its uses.

Note that we have tools for moving source files that updates include guards and
updates references to files (#includes and #imports in other source files,
references in .gyp(i) files); see src/tools/git/{move_source_file.py,
mass-rename.py}. We also have a tool for doing search-and-replace across the
codebase using Python regular expression syntax; see src/tools/git/mffr.py.
Finally, we highly encourage the usage of "git cl format" to ease the tedious
process of reformatting that often needs to occur when doing refactorings.

## **Recipes for Breaking //content Dependencies**

## If the component will be shared by iOS and has //content dependencies, then you have two choices:

*   ## Abstract all //content dependencies through the embedder and have
            the component not depend on //content at all.
*   ## Make the component into a [layered
            component](http://www.chromium.org/developers/design-documents/layered-components-design),
            wherein it has a "core/" directory containing code that is shared by
            iOS and cannot depend on //content, and a "content/" directory that
            drives the core code via interactions with //content.

## The first alternative is appropriate when the //content dependencies are not
significant (e.g., dependencies on content::BrowserThread that can be replaced
by having SingleThreadedTaskRunners injected into the component). The second
alternative is necessary when the feature has significant //content
dependencies, e.g. it interacts heavily with content::WebContents.

## For strategies to abstracting //content dependencies, please see
[here](http://www.chromium.org/developers/design-documents/layered-components-technical-approach).
For an example of the structure of a layered component, please see
[here](http://www.chromium.org/developers/design-documents/structure-of-layered-components-within-the-chromium-codebase).

## Recipes for Breaking //chrome Dependencies

Please feel free to add new sections, add clarifications and examples to this
section, etc. If you don't have edit rights to the site, feel free to add
comments at the bottom of the page with additional recipes.

### Dependency Inversion

Dependency inversion is the most fundamental approach to breaking dependencies.
This is the pattern where you switch from object Foo depending concretely on
object Bar, and instead object Foo defines an interface FooDelegate that it
needs fulfilled by any embedder; this interface is then implemented by Bar,
which passes a FooDelegate pointer to itself or its delegate implementation to
Foo.

By using this pattern, it is easy to push knowledge of minor dependencies out of
your component, and possible to remove knowledge of some larger dependencies
(but see the next section).

Examples:

*   [Introduction of
            AutofillManagerDelegate](https://chromiumcodereview.appspot.com/10837363).
*   [Introduction of
            SigninManagerDelegate](https://codereview.chromium.org/14367029).

### Large Dependencies: Do them First

Sometimes you’ll find that the feature you want to extract has a fairly big
dependency on a fairly big chunk of code. Using dependency inversion (see above)
would result in a large client interface or even multiple large client
interfaces.

In these cases, you’ve probably discovered that the component you want isn’t a
“leaf node,” whereas its dependency may be. In these cases you probably want to
componentize this dependency first. Exceptions to this might be where the
dependency would be fulfilled in entirely different ways, rather than in one
common way, by different embedders; if this is the case then a large client
interface may still be appropriate.

Examples:

*   The generic parts of //chrome/browser/webdata needed to be
            componentized before //chrome/browser/autofill could be
            componentized, and the Autofill-specific parts extracted into the
            Autofill component. See //components/webdata and
            //components/autofill/core/browser/webdata.

### Pass Fundamental Objects

It’s very common in //chrome that an “everything” object such as Profile is
passed in to initialize an object or a subsystem, which then turns around and
retrieves just a couple of more fundamental objects from the Profile.

We’ve seen this many times, e.g. a Profile being passed and the only things
retrieved from it being the PrefService associated with the Profile, and the
SequencedTaskRunner for the IO thread.

In cases like this, you should change the code to pass the most fundamental
objects possible, rather than passing more complex “everything” or “bag of
stuff” objects. See also [Law of
Demeter](http://en.wikipedia.org/wiki/Law_of_Demeter).

Examples:

*   [Avoid use of BrowserProcess global by passing
            PrefService](https://chromiumcodereview.appspot.com/15024007) (see
            e.g. signin_manager.h)

### Splitting Objects

Sometimes, you will encounter an object that brings in a lot of dependencies,
but at the same time seems to have functionality that is core to the component.
It might e.g. be an object that has some internal state machine that the
component needs to use, and also initiates or participates in a bunch of UI
interactions.

In cases like this, consider whether the object needs to be split into multiple
objects, where one is the core business logic (which stays with the component)
and another is the part that brings in all the dependencies (this could stay
with the embedder).

Examples:

*   [Removing knowledge of how to migrate specific table types from
            WebDatabase class](https://chromiumcodereview.appspot.com/12518017)
            and [externalizing
            creation](https://chromiumcodereview.appspot.com/12543034).

### Clean up APIs

It's worth noting that the programming interfaces that result from the types of
dependency-breaking operations listed above are not always optimal, in fact they
are often not as nice as what would have resulted from a feature being written
as a component from the start. Once you've finished breaking dependencies, it
can certainly be worth going back, taking a fresh look at the interfaces that
resulted, and cleaning them up.

## Verifying

To verify dependencies, use `tools/checkdeps/checkdeps.py` – to use on a
different repository, such as Blink, specify the `--root` option:

```none
buildtools/checkdeps/checkdeps.py --root third_party/WebKit
```

You can also specify a subdirectory to check:

```none
buildtools/checkdeps/checkdeps.py --root third_party/WebKit Source/bindings
```

There are some wrapper scripts to simplify this, like
`Tools/Scripts/check-blink-deps`.

There are a few wrinkles to be aware of, which result in `checkdeps` *not*
checking particular directories, confusingly returning SUCCESS without any work.
You can specify `--verbose` to check that it actually is doing work.

*   A `skip_child_includes` in a `DEPS` file causes subdirectories to
            not be checked; this is used in the top-level Chromium `DEPS` file
            to not check other repos.
*   The directory needs to look like a source directory, or imports will
            not be checked; concretely, it needs to include `.svn` or `.git/.`
