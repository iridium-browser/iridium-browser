---
breadcrumbs:
- - /Home
  - Chromium
page_name: ice-cream-sandwich-support-deprecation-faq
title: Ice Cream Sandwich Support Deprecation
---

## What is happening?

Chrome 42 will be the last supported release of Chrome for Android on all Ice
Cream Sandwich (Android 4.0) devices. Ice Cream Sandwich (ICS) users will
receive no further updates to Chrome on Android.

## Why the change?

While the number of Ice Cream Sandwich devices is shrinking, supporting them in
terms of engineering effort and technical complexity is increasingly difficult
over time. Each new feature or web capability that’s added to Chrome must be
built and tested for ICS. Often workarounds and special cases have to be added
specifically for ICS, and that adds code complexity, slows performance, and
increases development time.

The number of ICS devices is now sufficiently small that we can better serve our
users by phasing out support for earlier devices and focusing on making Chrome
better for the vast majority of users on more modern devices.

## What does this mean for users?

Users on Ice Cream Sandwich can continue to use Chrome, but will not receive
updates beyond Chrome 42.

## How many devices will be affected?

Ice Cream Sandwich makes up a small percentage of devices with a recent version
of Chrome for Android. Over the past year we’ve seen a thirty percent decrease
in the overall number of users using Chrome on Ice Cream Sandwich.

## When is the last Chrome release for Ice Cream Sandwich devices scheduled?

Chrome 42 will be the last update on Ice Cream Sandwich, and is expected to be
released in mid-April 2015. Chrome 43 will no longer support ICS and is expected
to be released toward the end of May 2015.

## Will there be any further Chrome updates or security patches for ICS devices?

We will continue to issue patches while Chrome 42 is the most recent Chrome
version. Once Chrome 43 is released, we don’t plan to issue further updates for
ICS devices.

## What will happen to the code supporting ICS?

We will be removing the code for ICS from trunk after the branch point for
Chrome 43.

## Are there other Android versions affected?

No. This only affects Ice Cream Sandwich.

## How can I handle developing for two versions of Chrome? What should I tell my users?

We encourage all developers irrespective of browser platform to consider
progressive enhancement when it comes to building apps and sites. By using
progressive enhancement you will be able to offer a good experience for as many
users as possible and in the case of the platform’s capabilities you will be
able to detect which features you will be able to support and which you might
need to polyfill.

## Where can I ask more questions or leave feedback?

For end users, Chrome has an extensive feedback reporting tool (accessible from
the Chrome menu).

For developers, the
[chromium-discuss](https://groups.google.com/a/chromium.org/forum/#!forum/chromium-discuss)
mailing list is a good place to start if you have specific issues with Ice Cream
Sandwich (note it is not a general support channel), or reach out to
@[ChromiumDev](https://twitter.com/ChromiumDev) on Twitter.
