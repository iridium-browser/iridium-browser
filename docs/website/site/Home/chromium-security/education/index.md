---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: education
title: Education
---

Security is a core principle and shared responsibility for everyone contributing
to Chromium. Here are some docs that can help an engineer get ramped up to
Chrome-specific security best practices, pitfalls, or relevant background. Send
your comments, questions, or additional security education needs to
security-dev@chromium.org

*   [IPC Security
            Tips](/Home/chromium-security/education/security-tips-for-ipc), a
            thrilling read about how to avoid introducing an IPC vulnerability
            and feature in the next
            [Pwnium](http://blog.chromium.org/2012/05/tale-of-two-pwnies-part-1.html)
            contest.
*   Security tips for avoiding common vulnerabilities and abuse vectors
            when [developing extensions and
            apps](/Home/chromium-security/education/security-tips-for-crx-and-apps).
*   [Everything you wanted to know about TLS/SSL in
            Chrome](/Home/chromium-security/education/tls)
*   If you are implementing a Chrome Extension/App API, read the
            [security guidelines for Chrome Extension & App API
            developers](https://docs.google.com/document/d/1RamP4-HJ7GAJY3yv2ju2cK50K9GhOsydJN6KIO81das/pub).
*   Do not implement your own allocator. Custom allocators are a major
            source of [security
            vulnerabilities](https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=437).
            Chrome's existing allocators (e.g.
            [Tcmalloc](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/tcmalloc/chromium/src/&q=tcmalloc&sq=package:chromium),
            [PartitionAlloc](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/wtf/PartitionAlloc.h&q=PartitionAlloc&sq=package:chromium&type=cs&l=1))
            and resillient to security issues. If you absolutely need to
            implement some form of custom allocator, make sure to get a thorough
            review from the security team.
*   When manipulating buffers in trusted memory, do not implement your
            own code for handling [integer
            overflows](http://en.wikipedia.org/wiki/Integer_overflow#Security_ramifications),
            truncations, or other integral boundary conditions. Instead use
            [base/numerics](https://code.google.com/p/chromium/codesearch#chromium/src/base/numerics/&ct=rc&cd=1&q=base/numerics&sq=package:chromium)
            templates which are already used in several parts of Chrome. The
            following refernces are also good resources on integer security
            issues:
    *   CERT C++ [Secure Coding
                Standard](https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=637).
    *   Mark Dowd, John McDonald, Justin Schuh, [The Art of Software
                Security Assessment: Identifying and Preventing Software
                Vulnerabilities](http://www.amazon.com/Art-Software-Security-Assessment-Vulnerabilities/dp/0321444426/ref=sr_1_1?s=books&ie=UTF8&qid=1402949525&sr=1-1&keywords=The+Art+of+Software+Security+Assessment).
    *   Robert C. Seacord, [Secure Coding in C and
                C++](http://www.amazon.com/Secure-Coding-Robert-C-Seacord/dp/0321335724).
