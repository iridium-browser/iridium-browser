---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/telemetry
  - 'Telemetry: Introduction'
page_name: telemetry-feature-guidelines
title: 'Telemetry: Feature Guidelines'
---

*This page describes some general guidelines for both reviewers and authors.
When justified, there may always be exceptions to these guidelines. But authors
be forewarned, you may not get an lgtm on patches that don't adhere to these.*

## Backwards compatibility

## All benchmarks and unittests must work for all Chromes from the current stable version to tip of tree. Where new Telemetry features rely on new Chrome features, the benchmark should gracefully degrade. Backwards compatibility fallbacks may be cleaned up after they are no longer necessary for the stable channel.

## Cross platform support

All features must be generalized to have a reasonable path to support on every
platform where Chrome runs (Android, CrOS, Linux, Mac, Windows and soon iOS).

We aim to take advantage of the best support on each platform, not conform to
the lowest common denominator across platforms. So some APIs may indicate that a
give functionality is not supported for the platform.

Since it isn't always feasible to implement all platforms in a single patch,
some platforms may raise a NotImplementedError. In this case, a crbug should be
filed for the platform implementation and reasonable effort should be made to
implement or find an owner.

## Dependencies

Telemetry must work for our builders and chromium developers without requiring a
compilation or manual installation/configuration. This means assuming no more
than a Chrome checkout and python 2.7.

In cases where native code is necessary, we [check in prebuilt
binaries](/developers/telemetry/upload_to_cloud_storage) for all relevant
platforms to cloud storage. In cases where installation/configuration is
required, Telemetry scripts it for the user.

There are also module boundaries where dependencies can only go in one
direction. These include, but are not limited to:

*   tools/telemetry cannot depend on tools/perf or content/test/gpu
*   telemetry.core cannot depend on telemetry.page

## Style

Code must adhere to the [Chromium Style Guide](/developers/coding-style) first
and the [Google Python Style
Guide](http://google-styleguide.googlecode.com/svn/trunk/pyguide.html) second.
We also borrow naming principles from the [Cocoa Coding
Guidelines](https://developer.apple.com/library/mac/documentation/Cocoa/Conceptual/CodingGuidelines/Articles/NamingBasics.html).

## Unittests

All features, large or small, must have unittest coverage. We welcome new
unittests, especially where there's no current coverage! Your patch/feature will
be much easier to accept should you decide to first bring some test coverage to
the existing code, and then add your new feature.

## Error handling

Be particularly careful when executing JavaScript or executing external
applications. If the JavaScript call hangs or the application has unexpected
behavior or output, we end up with TimeoutErrors that are hard to diagnose. For
applications, always check whether the application launched correctly, whether
it's in a consistent state, whether it produced the expected output, and whether
it produced the expected exit code. For JavaScript calls, try to catch
TimeoutErrors and print additional diagnostic information.
