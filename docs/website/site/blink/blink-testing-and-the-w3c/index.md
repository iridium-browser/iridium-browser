---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: blink-testing-and-the-w3c
title: Blink, Testing, and the W3C
---

*Blink is currently running a subset of the W3C's tests automatically, as
documented in [Importing the W3C tests to Blink](/blink/importing-the-w3c-tests)
.*

*This document is left here for historical purposes and to document some of the design and process tradeoffs involved.*

> *Document owner: [dpranke@chromium.org](mailto:dpranke@chromium.org)*

## Overview

The W3C has defined a large number of tests used for conformance testing.

Blink has inherited from WebKit a large number of tests that are used for
regression testing. Many of these tests (mostly the so-called "pixel tests")
require manual verification of correctness and take a fair amount of work to
keep these tests up-to-date. Many are also not written in a way that can be
easily used by other browser vendors.

Blink does not currently (4/2013) regularly import and run the W3C's tests, but
we have done some ad-hoc partial imports of test suites in the past into our
existing regression test suites. Some of these previously imported W3C tests are
therefore out of date and/or redundant.

The Blink team [has stated](/blink/#testing) that any new features being
developed should be accompanied by conformance tests and that we should be
working with the W3C on these tests. We believe that having a comprehensive,
vendor-neutral set of tests for the web platform is good for the web as a whole.

Thus, there are several interrelated problems:

    *The W3C has a lot of tests that aren't being run regularly (by us, at
    least).*

    *We have redundant and/or out of date tests.*

    *We have a lot of tests that are hard to maintain.*

    *We have a lot of tests we could potentially share with the W3C.*

    *We do not have a great sense of the coverage of either or both sets of
    tests.*

    *Neither we nor the W3C have smoothly running development processes to make
    these two sets of tests converge where possible.*

I will also note that although I used the words "conformance" and "regression"
above, they don't necessarily mean much difference in practice. You can, and
probably should, use conformance tests for regression testing.

## Ideal state

We should work together with the W3C to resolve these problems as much as
possible. The ideal end state might look something like:

    *All of the W3C’s tests would be in a single location or repository, clearly
    organized by some scheme that identified which features each test targeted,
    and what the state of the test was (submitted, accepted/approved, etc.)*

    *There would be some sort of process for minimizing or eliminating redundant
    tests.*

    *The test suite coverage would be comprehensive, and portable (it would be
    well understood how to write tests that ran in the major browsers).*

    *The processes for submitting new tests, getting changes made to existing
    tests, and downloading the latest test suites and integrating them into our
    builds would be well defined and practiced.*

    *The tests could be run and evaluated automatically (i.e., tests would not
    require manual review to see if you passed or failed).*

    *The number of chromium-/blink- specific layout tests would be kept to a
    minimum; if the test suites were comprehensive, arguably we’d have little
    need for custom tests except for features that weren’t standards-track. We
    would have a well-polished process for removing our tests when they were
    made redundant.*

## Current state

Today, as I currently understand it, none of these things are true, although
some are closer than others. Item-by-item:

    *Tests are scattered across multiple locations.*

        **Most of them can be found somewhere on the W3C’s mercurial repository
        (dvcs.w3.org/hg), and are mirrored to w3c-test.org .**

        **Some tests are also being moved to Github. The W3C has claimed a goal
        of moving everything to Github by the end of 2013.**

        **Different WGs follow different naming conventions, although I think
        things are fairly consistent for newer tests.**

    *I have no idea how people figure out if there are redundant tests in the
    W3C suites. My guess is that reviewers and spec editors make their best
    efforts here.*

    *I have no real idea how comprehensive the existing test suites are, nor how
    many of them run on Blink as-is (let alone pass).*

        **At a rough guess, there’s somewhere between 13,000 and 18,000 existing
        tests in various stages of approval.**

        **I don’t yet really know how many reftests really are portable and
        aren’t subject to diffs resulting from rounding errors and other things
        that don’t really affect correctness.**

        **On a related note, it’s hard to say how many of the tests are “good”,
        in the sense that they are maintainable, clear, focused, fast, etc.**

    *I think the processes for submitting new tests are roughly defined for most
    groups at this point, but I don’t know how broadly shared they are between
    WGs. We in Blink are at least not very familiar with the processes.*

    *Many of the older tests (e.g., CSS1, CSS2.1) were written assuming someone
    would look at them individually to decide if they were passing.*

        **We currently run those as pixel tests, but it would be difficult to
        say with confidence how many of them we were “passing” (due to the way
        Blink manages passing vs. expected results).**

        **There may be some tests that require interactive (manual) execution,
        as well as manual review. I don’t know if we are interested in them at
        all (I’m not, personally).**

    *I have no idea how many of our existing regression tests are redundant, nor
    how many of them could be reused by other vendors.*

**Work items**

So, this leaves us with the following (unprioritized) set of potential work
items:

    *Pull all of the existing tests down and see:*

        **How many of them we already have?**

        **How many of them are ref tests or are otherwise self-contained (i.e.,
        we don’t need to generate -expected results separately and review them
        for correctness)?**

        **What sort of coverage do we have? How much overlap with existing Blink
        tests do we have?**

        **How well do they integrate into the existing harnesses we have?**

    *Help organize the W3C repos.*

    *Work on tools and processes for submitting new tests, getting tests
    approved, and pulling them down to run, etc.*

    *Work to identify areas missing coverage.*

    *Work to write tests for those areas.*

    *Formalize tracking the test suites for new features we’re working on and
    ensuring the test suites are accepted (I think we do this today in a fairly
    ad-hoc and informal way). We should at least be regularly running our own
    tests!*

    *Work on the w3c’s test harness and figure out if it needs more features to
    be able to get better test coverage and/or work more portably.*

    *Publish our test results regularly somehow.*

    *Perhaps help publish the results of other implementations (Helping to build
    a common dashboard or something; presumably we don’t want to be publishing
    the results from other vendors ourselves).*

This is all a fair amount of work, but there are too many unknowns to be able to
really give a resource estimate (at least I wouldn’t want to).

## Q2 2013 goals

We on Blink have a Q2 2013 goal of at least the following:

    *Develop tools to import some of the larger test suites from the W3C, if not
    all of them. (with some [help from
    Adobe](https://lists.webkit.org/pipermail/webkit-dev/2013-March/024055.html))*

    *Import and run the tests in an automated manner as part of our existing
    regression testing processes (the layout tests). This will not include any
    tests requiring manual verification of correctness (i.e., no tests requiring
    new platform-specific baselines).*

    *Provide at least an initial set of feedback to the W3C on what did and
    didn't work (including which tests are and aren't passing), including
    enhancement requests for their infrastructure and processes if we find our
    needs aren't currently met.*

This should give us a pretty good sense of where we're at and how much work the
remaining open problems may be.

Note that if we get this working smoothly, we can hopefully extend these
processes to other standards bodies as need be (ECMA, Khronos, etc.)
