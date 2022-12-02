---
breadcrumbs:
- - /developers
  - For Developers
page_name: learning-your-way-around-the-code
title: Learning your way around the code
---

**There is lots to learn about the Chromium code base, both at a macro level
(how the processes are laid out, how IPC works, the flow of a URL load), and at
a micro level (code idioms such as smart pointer usage guidelines, message
loops, common threads, threading guidelines, string usage guidelines, etc).**

## Learning to do things the Chromium way

**[Coding style](/developers/coding-style): If you’ve coded elsewhere, the chrome guidelines (and code reviewer comments) might seem strict. For example, extra spaces at the end of lines are forbidden. All comments should be legitimate English sentences, including the ending period. There is a strict 80 column limit (with exceptions for things that can’t possibly be broken up).**

## **A personal learning plan**

Eventually you’ll have your build setup, and want to get to work. In a perfect
world, we would have all the time we needed to read every line of code and
understand it before writing our first line of code. In practice, this is
impossible. So, what can we do? We suggest you develop your own plan for
learning what you need, here are some suggested starting points.
Fortunately for us, Chromium has some top quality [design
docs](/developers/design-documents). While these can go a bit stale (for
instance, when following along, you may find references to files that have been
moved or renamed or refactored out of existence), it is awesome to be able to
comprehend the way that the code fits together overall.

1.  Read the most important developer design docs
    1.  [Multi-process
                architecture](/developers/design-documents/multi-process-architecture)
    2.  [Displaying a web page in
                Chrome](/developers/design-documents/displaying-a-web-page-in-chrome)
    3.  [Inter-process
                communication](/developers/design-documents/inter-process-communication)
    4.  [Threading](/developers/design-documents/threading)
    5.  [Getting around the Chrome source
                code](/developers/how-tos/getting-around-the-chrome-source-code)
2.  See if your group has any docs; there may be some docs that people
            working on the same code will care about while others don’t need to
            know as much detail.
3.  Learn some of the code idioms:
    1.  [Important abstractions and data
                structures](/developers/coding-style/important-abstractions-and-data-structures)
    2.  [Smart pointer guidelines](/developers/smart-pointer-guidelines)
    3.  [Chromium String usage](/developers/chromium-string-usage)
4.  Later, as time permits, skim all the design docs, reading where it
            seems relevant.
5.  Get good at using code search ([or your code browsing tool of
            choice](/developers/code-browsing-in-chromium))
6.  Learn who to ask how the code works ([how to find somebody who knows
            how the code
            works](/developers/finding-somebody-who-knows-how-a-piece-of-code-works))
7.  Debug into the code you need to learn, with a debugger if you can,
            or log statements and grepping if you cannot.
8.  Look at the differences in what you need to understand and you
            currently understand. For instance, if your group does a lot of GUI
            programming, then maybe you can invest time in learning GTK+, Win32,
            or Cocoa programming.

## **Blink**

**Sometimes to make a fix or add a feature to Chromium, the right place to put it is in Blink (formerly WebKit).**
**There is a (2012) [“How Webkit works” slide
deck](https://docs.google.com/presentation/pub?id=1ZRIQbUKw9Tf077odCh66OrrwRIVNLvI_nhLm2Gi__F0).
While Blink has forked, some of this may still be relevant.**

There is also a slide that explains a [basic workflow for WebKit development for
people who are already familiar with Chromium
development](/system/errors/NodeNotFound).
