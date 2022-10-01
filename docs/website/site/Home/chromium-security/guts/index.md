---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: guts
title: Secure Architecture
---

One of our [core security principles](/Home/chromium-security/core-principles)
is, "Design for defense in depth." Some of the things we've done or are working
on to live up to this principle include:

## Background

*   <http://seclab.stanford.edu/websec/chromium/>
*   [A color-by-risk component diagram of
            Chrome](https://docs.google.com/a/chromium.org/drawings/d/1TuECFL9K7J5q5UePJLC-YH3satvb1RrjLRH-tW_VKeE/edit)

## Sandboxing

### Platform-specific sandboxing

*   [Chrome on Windows (sandbox) design and
            implementation](/developers/design-documents/sandbox) and the
            [Sandboxing FAQ (mostly Windows
            specific](/developers/design-documents/sandbox/Sandbox-FAQ))
*   [Chrome on Linux and Chrome OS (sandbox)
            overview](https://code.google.com/p/chromium/wiki/LinuxSandboxing)
            (including the most current [seccomp-bpf
            layer](http://blog.chromium.org/2012/11/a-safer-playground-for-your-linux-and.html))
    *   [bpf_dsl
                presentation](https://drive.google.com/file/d/0B9LSc_-kpOQPVHhvcVBza3NWR0k/view?usp=sharing)
                (Sep 2014)
*   [Chrome on OSX (sandbox)
            overview](/developers/design-documents/sandbox/osx-sandboxing-design)
            and the [second-layer bootstrap
            sandbox](https://docs.google.com/a/chromium.org/document/d/108sr6gBxqdrnzVPsb_4_JbDyW1V4-DRQUC4R8YvM40M/edit#)

### Plugin sandboxing

*   [Flash sandboxing
            (PPAPI)](http://blog.chromium.org/2012/08/the-road-to-safer-more-stable-and.html)

### Site Isolation

We're currently working on using Chrome's sandbox to isolate websites from each
other via the [Site Isolation project](/Home/chromium-security/site-isolation),
which will help to mitigate cross-site information leaks (among other threats)
in the presence of a vulnerability in the renderer process.

## Anti-Exploitation Technologies and Tactics

*   We use industry best practices
            [ASLR](http://en.wikipedia.org/wiki/Address_space_layout_randomization),
            [DEP](http://en.wikipedia.org/wiki/Data_Execution_Prevention), [JIT
            hardening](http://www.matasano.com/research/Attacking_Clientside_JIT_Compilers_Paper.pdf#page=24),
            and
            [SafeSEH](http://msdn.microsoft.com/en-us/library/9a89h429.aspx).
*   We block [out-of-date or unpopular
            plugins](http://support.google.com/chrome/bin/answer.py?hl=en&answer=1181003)
            by default and support work toward [NPAPI
            deprecation](http://blog.chromium.org/2013/09/saying-goodbye-to-our-old-friend-npapi.html).
*   We implement memory hardening features, like [Binding
            Integrity](/Home/chromium-security/binding-integrity).
