---
breadcrumbs:
- - /developers
  - For Developers
page_name: finding-somebody-who-knows-how-a-piece-of-code-works
title: Finding somebody who knows how a piece of code works
---

**It’s much more efficient to ask somebody how something works than to take
weeks to figure it out yourself. So how do you find the person who knows how it
works? Here are some ideas to get you started.**

1.  **Ask your teammates - the folks who have been on the project longer
            than you will have a better idea or may have contacts in other
            areas.**
2.  **Look at the OWNERS files - If you can identify a source file
            involved in what you want to do, find the lowest level OWNERS file
            above it, and email those people. Be aware that they may no longer
            be working on chromium, or on parental leave, so don’t just wait
            forever and keep hoping for an answer if you don’t hear back soon.**
3.  **Sometimes, there isn't one person who knows what you want to find
            out, but you can find people who know parts of the system. For
            instance, no one person knows everything about how chromium starts
            up or shuts down and can tell you what you did wrong in a shutdown
            scenario, but you can find people who know some parts of the
            shutdown to guide you.**
4.  **Use the IRC channel - while not everybody is listening all the
            time, there is always a sheriff listening, and you may get a pointer
            to the right folks to talk to.**
5.  **Use the chromium-dev email alias to ask questions. This is a
            fairly large email list, so use your judgment about what to post,
            and it may take awhile to get a reply. If you do this, it helps to
            ask one crisp question at the top of your mail and then explain in
            detail instead of rambling and asking many questions.**
6.  **Try searching the code. Sure, searching for “Init” or “setup” will
            have about 10^100 hits (at least it will seem that way), but
            searching for something more specific may pay off. Try several
            related terms. For instance, searching for “favicon” will quickly
            take you to the right part of the code, even if you have no idea how
            the code is laid out our what you are looking at when you arrive at
            the right code.**
7.  **Debugging into the code can provide a lot of insight.**
