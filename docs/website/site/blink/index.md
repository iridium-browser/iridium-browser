---
breadcrumbs: []
page_name: blink
title: Blink (Rendering Engine)
---

[TOC]

## Mission

Make the Web the premier platform for experiencing the world’s information and
deliver the world’s best implementation of the Web platform.

## What is Blink?

Blink is the name of the [rendering
engine](https://en.wikipedia.org/wiki/Web_browser_engine) used by Chromium and
particularly refers to the code living under
***[src/third_party/blink](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/).***

## Participating

[Chromium](http://chromium.org) is an
[inclusive](https://chromium.googlesource.com/chromium/src/+/HEAD/CODE_OF_CONDUCT.md)
open-source community that values fostering a supportive culture.

### Discussions

We value transparency and open collaboration. Our goal is for everyone to be
able to participate, regardless of organizational affiliation. There are several
areas where developer discussions take place:

*   [blink-dev@chromium.org](https://groups.google.com/a/chromium.org/group/blink-dev/topics)
            is the general list for discussions relevant to the design and
            implementation of web platform features.
*   Technical Discussion Groups -
            [Groups](/developers/technical-discussion-groups) dedicated to the
            discussion of specific areas of the codebase.
*   Slack (#blink): We hang out on the #blink channel on
            [chromium.slack.com](https://chromium.slack.com) to have quick,
            informal discussions and answer questions.

### Reporting Issues

We use Chromium's [issue
tracker](https://bugs.chromium.org/p/chromium/issues/list) (aka
[crbug.com](http://crbug.com)). Web Platform issues live under components in
[Blink](https://bugs.chromium.org/p/chromium/issues/list?q=component%3Ablink&can=2)
and
[Internals](https://bugs.chromium.org/p/chromium/issues/list?q=component%3Ainternals&can=2).

### Tracking features

For developers interested in tracking new features, there are several dedicated
channels for staying up-to-date:

*   [Beta Blog Posts](https://blog.chromium.org/search/label/beta): For
            each new Chrome Beta release (~every six weeks), the Chrome team
            publishes [blog posts](https://blog.chromium.org/search/label/beta)
            outlining the changes to the web platform and the Chrome Apps &
            Extensions APIs.
*   Chrome Developer Relations: Chrome DevRel posts about new features
            on [web.dev](https://web.dev/), Twitter
            ([@ChromiumDev](https://twitter.com/ChromiumDev)), and
            [YouTube](https://www.youtube.com/user/ChromeDevelopers/) (Google
            Chrome Developers channel).
*   [chromestatus.com](https://www.chromestatus.com/): A dashboard where
            we track new feature development.
*   [bit.ly/blinkintents](https://bit.ly/blinkintents): A Google
            Spreadsheet that lists all ["Intent"
            threads](/blink#TOC-Web-Platform-Changes:-Process) and their
            approval status.

## Developing

### Learning about Blink Development

Blink is implemented on top of an abstract platform and thus cannot be run by
itself. The [Chromium Content module](/developers/content-module) provides the
implementation of this abstract platform required for running Blink.

*   [How Blink
            works](https://docs.google.com/document/d/1aitSOucL0VHZa9Z2vbRJSyAIsAz24kX8LFByQ5xQnUg)
            is a high-level overview of Blink architecture.
*   [Chromium Content module](/developers/content-module) - Details on
            the abstract platform required to run Blink.
*   [Getting Started with Blink
            Debugging](/blink/getting-started-with-blink-debugging) - Best
            practices on how to debug Blink (using Content Shell).
*   [Chromium Development](/developers) - Guides and best practices for
            Chromium development
*   YouTube ([Chrome
            University](https://www.youtube.com/playlist?list=PLNYkxOF6rcICgS7eFJrGDhMBwWtdTgzpx)
            Playlist) - Introductory lessons that cover the fundamental concepts
            for Chromium development.

### Committing and reviewing code

Blink follows all standard [Chromium](/developers) practices, including those on
[contributing
code](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/contributing.md)
and [becoming a committer](/getting-involved/become-a-committer). Code living in
*[src/third_party/blink](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/)*
follows the [Blink Coding Style Guidelines](/blink/coding-style).

### Launching and Removing Features

*   [How we think about making changes to the
            platform](/blink/guidelines/web-platform-changes-guidelines)
*   [Launching a Web Platform Feature](/blink/launching-features)
*   [Removing a Web Platform Feature](/blink/deprecating-features)
*   (Video) [Intent to Explain: Demystifying the Blink shipping
            process](https://www.youtube.com/watch?v=y3EZx_b-7tk)

### Page Directory

{% subpages collections.all %}
