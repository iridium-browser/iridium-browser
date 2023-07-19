---
breadcrumbs: []
page_name: developers
title: For Developers
---

<div class="two-column-container">
<div class="column">

#### *See also: docs in the source code - <https://chromium.googlesource.com/chromium/src/+/HEAD/docs/README.md>*

### Start here

*   [Get the Code: Checkout, Build, &
            Run](/developers/how-tos/get-the-code)
*   [Contributing
            code](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/contributing.md)

### How-tos

#### *Note that some of these guides are out-of-date.*

#### Getting the Code

*   [Quick reference](/developers/quick-reference) of common development
            commands.
*   Look at our [Git
            Cookbook](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/git_cookbook.md)
            for a helpful walk-through, or the [Fast Intro to Git
            Internals](/developers/fast-intro-to-git-internals) for a background
            intro to git.
*   [Changelogs for Chromium and Blink](/developers/change-logs).

#### Development Guides

*   Debugging on [Windows](/developers/how-tos/debugging-on-windows),
            [Mac OS X](/developers/how-tos/debugging-on-os-x),
            [Linux](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux/debugging.md)
            and
            [Android](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/android_debugging_instructions.md).
*   [Threading](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/threading_and_tasks.md)
*   [Subtle Threading Bugs and How to Avoid
            Them](/developers/design-documents/threading/suble-threading-bugs-and-patterns-to-avoid-them)
*   [Visual Studio tricks](/developers/how-tos/visualstudio-tricks)
*   [Debugging GPU related
            code](/developers/how-tos/debugging-gpu-related-code)
*   [How to set up Visual Studio debugger
            visualizers](/developers/how-tos/how-to-set-up-visual-studio-debugger-visualizers)
            to make the watch window more convenient
*   [Linux
            Development](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux_development.md)
            tips and porting guide
*   [Mac Development](/developers/how-tos/mac-development)
*   [Generated files](/developers/generated-files)
*   [Chromoting (Chrome Remote Desktop)
            compilation](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/chromoting_build_instructions.md)
*   [Updating module
            dependencies](/developers/how-tos/chromium-modularization)
*   [Editing
            dictionaries](/developers/how-tos/editing-the-spell-checking-dictionaries)
*   Editors Guides
    *   [Atom](/developers/using-atom-as-your-ide)
    *   [Eclipse](/developers/using-eclipse-with-chromium)
    *   [Emacs
                cscope](/developers/how-tos/cscope-emacs-example-linux-setup)
    *   [QtCreator](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/qtcreator.md)
    *   [SlickEdit](/developers/slickedit-editor-notes)
    *   [Sublime
                Text](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/sublime_ide.md)
    *   [Visual Studio
                Code](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/vscode.md)
*   [Learning your way around the
            code](/developers/learning-your-way-around-the-code)
*   [Guide to Important Libraries, Abstractions, and Data
            Structures](/developers/libraries-guide)
    *   [Important Abstractions and Data
                Structures](/developers/coding-style/important-abstractions-and-data-structures)
    *   [Smart Pointer Guidelines](/developers/smart-pointer-guidelines)
    *   [String usage](/developers/chromium-string-usage)
*   [//base Newsletters](/developers/base-newsletters)
*   [Android WebView](/developers/androidwebview)
*   [GitHub Collaboration](/developers/github-collaboration)

See also: All [How-tos](/developers/how-tos).

### Blink

*   [Blink Project](/blink)
    *   [DOM Team](/teams/dom-team)
    *   [Binding Team](/teams/binding-team)
    *   [Layout Team](/teams/layout-team)
    *   [Memory Team](/blink/memory-team)
    *   [Paint Team](/teams/paint-team)
    *   [Style Team](/teams/style-team)
    *   [Animation Team](/teams/animations)
    *   [Input Team](/teams/input-dev)
*   [Running and Debugging the Blink web tests (pka layout
            tests)](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_tests.md)
*   [Blink Sheriffing](/blink/sheriffing)
*   [Web IDL interfaces](/developers/web-idl-interfaces)
*   [Class Diagram: Blink Core to Chrome
            Browser](/developers/class-diagram-webkit-webcore-to-chrome-browser)
*   [Rebaselining
            Tool](http://code.google.com/p/chromium/wiki/RebaseliningTool)
*   [How repaint
            works](https://docs.google.com/a/chromium.org/document/d/1jxbw-g65ox8BVtPUZajcTvzqNcm5fFnxdi4wbKq-QlY/edit)
*   [Phases of
            Rendering](https://docs.google.com/a/chromium.org/document/d/1UkxPz9GDQXLBZcbw5OeUQpk1VIq_BKhm6BGvWJ5mKdU/edit)
*   [Web Platform
            Tests](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_platform_tests.md)
*   [Baseline computation and some line layout
            docs](https://docs.google.com/a/chromium.org/document/d/1OP49xbB-D7A0qKNAwFTOfbDL-1dYxu74Jp38ZKAS6kk/edit)
*   [Fast Text Autosizer](http://tinyurl.com/fasttextautosizer)

### Testing and Infrastructure

*   [Tests](/developers/testing)
    *   [Tour of the Chromium Continuous Integration
                Console](/developers/testing/chromium-build-infrastructure/tour-of-the-chromium-buildbot)
    *   [WebKit Layout Tests](/developers/testing/webkit-layout-tests)
    *   [Flakiness Dashboard
                HOWTO](/developers/testing/flakiness-dashboard)
    *   [Frame Rate Test](/developers/testing/frame-rate-test)
    *   [GPU Testing](/developers/testing/gpu-testing)
    *   [GPU Recipe](/system/errors/NodeNotFound)
    *   [WebGL Conformance
                Tests](/developers/testing/webgl-conformance-tests)
    *   [Blink, Testing, and the W3C](/blink/blink-testing-and-the-w3c)
    *   [The JSON Results
                format](/developers/the-json-test-results-format)
*   [Browser Tests](/developers/testing/browser-tests)
*   [Handling a failing
            test](/developers/tree-sheriffs/handling-a-failing-test)
*   [Running Chrome
            tests](http://code.google.com/p/chromium/wiki/RunningChromeUITests)
*   [Reliability Tests](/system/errors/NodeNotFound)
*   [Using Valgrind](/system/errors/NodeNotFound)
*   [Page Heap for Chrome](/developers/testing/page-heap-for-chrome)
*   [Establishing Blame for Memory usage via
            Memory_Watcher](/developers/memory_watcher)
*   [GPU Rendering
            Benchmarks](/developers/design-documents/rendering-benchmarks)
*   [Infra
            documentation](https://chromium.googlesource.com/infra/infra/+/HEAD/doc/index.md)
*   [Contacting a
            Trooper](https://chromium.googlesource.com/infra/infra/+/HEAD/doc/users/contacting_troopers.md)
            

### Performance

*   [Adding Performance
            Tests](/developers/testing/adding-performance-tests)
*   [Telemetry: Performance testing framework](/developers/telemetry)
    *   [Cluster Telemetry](https://sites.google.com/corp/google.com/cluster-telemetry/home): Run
                benchmarks against the top 10k web pages (Googlers-only)
*   [Memory](/Home/memory)
*   [Profiling Tools](/developers/profiling-chromium-and-webkit):
    *   [Thread and Task Profiling and
                Tracking](/developers/threaded-task-tracking) (about:profiler)
        Also allows diagnosing per-task heap usage and churn if Chrome runs with
        "--enable-heap-profiling=task-profiler".
    *   [Tracing tool](/developers/how-tos/trace-event-profiling-tool)
                (about:tracing)
    *   [Deep Memory Profiler](/developers/deep-memory-profiler)
    *   Investigating [Windows Binary
                Sizes](/developers/windows-binary-sizes)
    *   Windows-specific issues can be profiled with
                [UIforETW](https://randomascii.wordpress.com/2015/09/01/xperf-basics-recording-a-trace-the-ultimate-easy-way/)
    *   [Leak Detection](/developers/leak-detection)
*   [Perf
            Sheriffing](http://www.chromium.org/developers/tree-sheriffs/perf-sheriffs)

### Sync

*   [Sync](/developers/design-documents/sync)

### **Diagnostics**

*   [Diagnostics](/developers/diagnostics)

### Documentation hosted in / generated by source code

*   [depot_tools](http://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html)
*   [C++ use in Chromium](http://chromium-cpp.appspot.com/)
*   [GN](https://gn.googlesource.com/gn/): Meta-build system that
            generates NinjaBuild files; Intended to be GYP replacement.
*   [MB](https://chromium.googlesource.com/chromium/src/+/HEAD/tools/mb#):
            Meta-build wrapper around both GN and GYP.
*   [Chrome
            Infra](https://chromium.googlesource.com/infra/infra/+/HEAD/doc/index.md)

</div>
<div class="column">

### Practices

*   [Core Product Principles](/developers/core-principles)
    *   [No Hidden
                Preferences](/developers/core-principles/no-hidden-preferences)
*   [Contributing code](/developers/contributing-code)
    *   [Coding style](/developers/coding-style)
        *   [C++ Dos and
                    Don'ts](/developers/coding-style/cpp-dos-and-donts)
        *   [Cocoa Dos and
                    Don'ts](/developers/coding-style/cocoa-dos-and-donts)
        *   [Web Development Style
                    Guide](/developers/web-development-style-guide)
        *   [Java](/developers/coding-style/java)
        *   [Jinja](/developers/jinja)
    *   [Becoming a Committer](/getting-involved/become-a-committer)
    *   [Gerrit Guide (Googler/Non-Googler)](/developers/gerrit-guide)
    *   [Create experimental branches to work
                on](/developers/experimental-branches)
    *   [Committer's
                responsibility](/developers/committers-responsibility)
    *   [OWNERS
                Files](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/code_reviews.md)
    *   [Try server
                usage](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/infra/trybot_usage.md)
    *   [Commit queue](/developers/testing/commit-queue)
    *   [Tips for minimizing code review lag across
                timezones](/developers/contributing-code/minimizing-review-lag-across-time-zones)
*   [Starting to work on a new web platform
            feature](/blink/launching-features)
*   [Filing bugs](/for-testers/bug-reporting-guidelines)
    *   [Severity Guidelines for Security
                Issues](/developers/severity-guidelines)
*   [Network bug
            triage](/developers/design-documents/network-stack/network-bug-triage)
*   [GPU bug
            triage](https://docs.google.com/document/d/1Sr1rUl2a5_RBCkLtxfx4qE-xUIJfYraISdSz_I6Ft38/edit#heading=h.vo10gbuchnj4)
*   [Ticket milestone punting](/system/errors/NodeNotFound)
*   [Tree Sheriffs](/developers/tree-sheriffs)
*   [Useful extensions for developers](/developers/useful-extensions)
*   [Adding 3rd-party libraries](/developers/adding-3rd-party-libraries)
*   [Shipping changes that are
            enterprise-friendly](/developers/enterprise-changes)

Design documents

*   [Getting around the source code
            directories](/developers/how-tos/getting-around-the-chrome-source-code)
*   [Tech Talks: Videos & Presentations](/developers/tech-talk-videos)
*   [Engineering design docs](/developers/design-documents)
*   [User experience design docs](/user-experience)
*   *Sharing design documents on Google drive: share on Chromium domain*
    If on private domain, share with self@chromium.org, then log in with
    self@chromium.org, click "Shared with Me", right-click "Make a copy", then
    set the permissions: "Public on the web" or "Anyone with the link",
    generally "Can comment". It a good idea to then mark your local copy
    *(PRIVATE)* and only edit the public copy.

### Communication

*   [General discussion groups](/developers/discussion-groups)
*   [Technical discussion
            groups](/developers/technical-discussion-groups)
*   [Slack](/developers/slack)
*   [IRC](/developers/irc)
*   [Development calendar and release info](/developers/calendar)
*   [Common Terms & Techno
            Babble](http://code.google.com/p/chromium/wiki/Glossary)
*   [Public calendar for meetings discussing new
            ideas](/developers/public-calendar-for-meetings-discussing-new-ideas)
*   Questions or problems with your Chromium account? Email
            [accounts@chromium.org](mailto:accounts@chromium.org).

### Status

*   [Status Update Email Best
            Practices](/developers/status-update-email-best-practices)
*   [chromestatus.com](http://chromestatus.com)

Usage statistics

*   [MD5 certificate statistics](/developers/md5-certificate-statistics)

### Graphics

*   [Graphics overview and design
            docs](/developers/design-documents/chromium-graphics)

### External links

*   Waterfalls
    *   [Continuous
                build](http://build.chromium.org/p/chromium/waterfall)
                ([console](http://build.chromium.org/p/chromium/console))
    *   [Memory](http://build.chromium.org/p/chromium.memory/waterfall)
                ([console](http://build.chromium.org/p/chromium.memory/console))
    *   [For Your Information
                build](http://build.chromium.org/p/chromium.fyi/waterfall)
                ([console](http://build.chromium.org/p/chromium.fyi/console))
    *   [Try
                Server](http://build.chromium.org/p/tryserver.chromium.linux/waterfall)
*   [Build Log Archives
            (chromium-build-logs)](http://chromium-build-logs.appspot.com/)
*   [Bug tracker](https://bugs.chromium.org/p/chromium/issues/list)
*   [Code review tool](https://chromium-review.googlesource.com/)
*   [Viewing the
            source](https://chromium.googlesource.com/chromium/src/)
*   [Glossary](http://code.google.com/p/chromium/wiki/Glossary)
            (acronyms, abbreviations, jargon, and technobabble)

</div>
</div>

{% subpages collections.all %}
